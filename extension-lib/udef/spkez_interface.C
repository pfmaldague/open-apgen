#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//
// Originally, this file contained a rather complex algorithm
// for building interpolation data "on the fly" during modeling.
// This worked pretty well but we now use precomputed resources,
// for which interpolation data are computed once and for all
// when a new trajectory becomes available.  Therefore, there
// is no longer a need for an algorithm that computed interpolation
// data on the fly.
//
// By default, the current implementation of spkez_api now does
// very simple caching: if a call with specific parameters was
// already made in the past, the past returned data are reused
// instead of making a new call to spkez.  This is simple to
// implement and does not require interpolation.
//
// The default can be overridden by invoking udef function
// spkez_control. If caching is turned off, every invocation of
// spkez_api will result in a new call to spkez.
//

#define AU 150000000L // A. U. in km (approximate)

//
// TODO: replace integer constants -159, 10, 399 etc. by
// appropriate symbolic constants defined in the SPICE toolkit.
//

#include "spiceIntfc.H"
#include "aafReader.H"

#define ARG_POINTER args.size()
#define ARG_POP args.pop_front()
#define ARG_STACK slst<TypedValue*>& args
#define WHERE_TO_STOP 0


//
// Caching does not occur if the estimated error is less than
// the following number (which can be set using spkez_control).
// However, the error estimate is very conservative and the
// actual error is significantly smaller.
//
// NOTE: as of today (April 16, 2021) a relative error of
//       0.0001 is hard-coded into the algorithms. This
//       should obviously be changed at some point. In
//       particular, theRelError is not used anywhere in
//       this file.
//
double		spkez_intfc::theRelError = 0.0;
CTime_base 	spkez_intfc::PlanStart;
CTime_base 	spkez_intfc::PlanEnd;

Cstring&	spkez_intfc::theAAFFileName() {
	static Cstring s;
	return s;
}

Cstring&	spkez_intfc::theTCMFileName() {
	static Cstring t;
	return t;
}

Cstring&	spkez_intfc::theJOIFileName() {
	static Cstring j;
	return j;
}

//
// The following combination of static parameters represents the
// "safe" choice: only repeat calls are cached; interpolation is
// not used at all.  Given the current state of the interpolation
// algorithm, it _is_ safe to set repeat_calls_only to false, so
// that interpolation will take place.  However, the efficiency
// gains are modest, and it is my view (Pierre speaking) that it's
// not worth adding the theoretical uncertainty of interpolation
// at the present time.
//
// What _will_ most likely be worth it is to introduce the notion
// of pre-computed resources, not only for purely geometric
// resources but also for GNC resources having to do with attitude
// computations.  The real payoff in terms of overall efficiency
// is to include pre-computed resources for _all_ computational
// hogs.
//
bool spkez_intfc::caching_enabled = true;
bool spkez_intfc::subsystem_initialization_run = false;
bool spkez_intfc::repeat_calls_only = true;
bool spkez_intfc::dump_call_parameters = false;
bool spkez_intfc::check_precomp_res = false;
FILE* spkez_intfc::check_precomp_file = NULL;
bool spkez_intfc::check_errors = false;

vector<double>& spkez_intfc::max_error() {
    static vector<double>	V;
    return V;
}

vector<CTime_base>& spkez_intfc::time_at_max_error() {
    static vector<CTime_base>	C;
    return C;
}

//
// Internal static objects - used for debug-style reporting.
//
int  spkez_intfc::total_count = 0;
int  spkez_intfc::spkez_count = 0;
int  spkez_intfc::interpolated_count = 0;

//
// ZONE OF INFLUENCE
//
static FILE* zone_file = NULL;

CTime_base&	zone_time() {
	static CTime_base T;
	return T;
}

map<int, FILE*>&	aaf_files() {
	static map<int, FILE*> M;
	return M;
}

map<int, Cstring>&	aaf_starts() {
	static map<int, Cstring> N;
	return N;
}

vector<Cstring>&	aaf_indices() {
	static vector<Cstring> V;
	return V;
}

//
// In udef_functions.C:
//
extern TypedValue create_float_list(int n, double* d);

spkez_intfc::data_map&	spkez_intfc::spkez_arg_map() {
	static data_map D;
	return D;
}
vector<spkez_intfc::data_index>& spkez_intfc::spkez_args() {
	static vector<data_index> V;
	return V;
}

//
// The info is the period of the frame in hours,
// if dynamic. If the frame is inertial, the info
// is set to 0.0.
//
map<Cstring, double>& spkez_intfc::frame_info() {
	static map<Cstring, double> FI;
	return FI;
}
vector<spkez_intfc::data_index>& spkez_intfc::master_list_of_spkez_arguments() {
	static vector<data_index> V3;
	return V3;
}
Cstring&	spkez_intfc::AAFname() {
	static Cstring N;
	return N;
}
vector<spkez_intfc::call_data>& spkez_intfc::prev_spkez_call() {
	static vector<call_data> V1;
	return V1;
}
tlist<alpha_int, spkez_intfc::body_state>& spkez_intfc::bodies_of_interest() {
	static tlist<alpha_int, body_state> M;
	return M;
}

Cstring		spkez_intfc::body_state::identify() {
	Cstring s = name_this_object(Key.get_int(), false);
	return s;
}

void spkez_intfc::call_data::interpolate(
				CTime_base T,
				double state[],
				aoString& info) {
    double t_m_t1 = (T - itime).convert_to_double_use_with_caution();
    double delta_t = (ftime - itime).convert_to_double_use_with_caution();

    if(check_errors) {
	char buf[10];
	double s = t_m_t1 / delta_t;
	sprintf(buf, "%5.3lf", s);
	info << " s = " << buf;
    }

    for(int k = 0; k < 3; k++) {
	// velocity index
	int vk = 3 + k;
	double v_av = (fresult[k] - iresult[k]) / delta_t;
	double x = 6.0 * (v_av - 0.5 * (fresult[vk] + iresult[vk])) / (delta_t * delta_t);
	double y = (fresult[vk] - iresult[vk]) / delta_t + x * delta_t;
	state[vk] = iresult[vk] + y * t_m_t1 - x * t_m_t1 * t_m_t1;
	state[k] = iresult[k] + iresult[vk] * t_m_t1
			+ y * t_m_t1 * t_m_t1 * 0.5
			- x * t_m_t1 * t_m_t1 * t_m_t1 / 3.0;
    }
}

double spkez_intfc::estimate_error(
				const TypedValue& a,
				const TypedValue& b) {
    ArrayElement* ae;
    const ListOVal&	lov1 = a.get_array();
    const ListOVal&	lov2 = b.get_array();
    double		position1[3];
    double		position2[3];
    for(int i = 0; i < 3; i++) {
	ae = lov1[i];
	position1[i] = ae->Val().get_double();
	ae = lov2[i];
	position2[i] = ae->Val().get_double();
    }
    return estimate_error(position1, position2);
}

void spkez_intfc::document_error(
				const TypedValue& a,
				const TypedValue& b,
				double	out[]) {
    ArrayElement* ae;
    const ListOVal&	lov1 = a.get_array();
    const ListOVal&	lov2 = b.get_array();
    for(int i = 0; i < 3; i++) {
	ae = lov1[i];
	out[i] = ae->Val().get_double();
	ae = lov2[i];
	out[3 + i] = ae->Val().get_double();
    }
}

Cstring spkez_intfc::cached_spkez_data::identify() {
    Cstring s;
    s << "(" << name_this_object(observer, false)
	<< ", " << name_this_object(target, false)
	<< ", " << ref_frame
	<< ", " << abcorr << ")";
    return s;
}

double spkez_intfc::estimate_error(
				double act[],
				double pred[]) {
	double R2 = 0.0;
	double diff;
	double delta = 0.0;
	for(int k = 0; k < 3; k++) {
	    R2 += act[k] * act[k];
	    diff = act[k] - pred[k];
	    delta += diff * diff;
	}
	double error = sqrt(delta / R2);

	return error;
}

void spkez_api(
	int targ,
	double utc,
	const char *ref,
	const char *abcorr,
	int obs,
	double *atarg,
	double *lt ) {
  char         MGSO_time[1024];
  SpiceDouble  et;

  spkez_intfc::spkez_count++;

  strcpy( MGSO_time, SCET_PRINT_char_ptr( utc ));

  /* Convert UTC epoch to ephemeris seconds past J2000 */
  str2et_c ( MGSO_time , &et );

  //(void)fprintf(stdout,"%f %s %f\n",utc, MGSO_time, et);

  spkez_c  ( targ, et, ref, abcorr, obs, atarg, lt );

  //(void)fprintf(stdout,
  //		"%s,%f,%d,%d,%f,%f,%f,%f,%f,%f\n",
  //		MGSO_time,et,obs,targ,
  //		atarg[0],atarg[1],atarg[2],atarg[3],atarg[4],atarg[5]);

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}

//
// Arguments are PlanStart, PlanEnd, relative_error. Tells
// the spkez subsystem that this run is intended to produce
// spkez interpolation coefficients files (SICF). Such files
// can be used to interpolate spkez over the entire span of
// a plan.
//
apgen::RETURN_STATUS create_geom_subsys_aaf_ap(
			Cstring& errs,
			TypedValue* result,
			ARG_STACK) {
    TypedValue*	n1; // CTime_base start
    TypedValue*	n2; // CTime_base end
    TypedValue*	n3; // double relative_error
    TypedValue*	n4; // array spkez_args
    TypedValue*	n5; // array frame_info
    TypedValue*	n6; // Cstring AAFname
    ListOVal*	params;
    ListOVal*	frame_list;

    if( ARG_POINTER != ( WHERE_TO_STOP + 6) ) {
	errs << "  wrong number of parameters passed to user-defined function "
	    << "create_geom_subsys_aaf; expected 6, got " << ( ARG_POINTER - WHERE_TO_STOP )
	    << "\n";
	return apgen::RETURN_STATUS::FAIL;
    }
    n6 = ARG_POP;
    n5 = ARG_POP;
    n4 = ARG_POP;
    n3 = ARG_POP;
    n2 = ARG_POP;
    n1 = ARG_POP;
    if(!n6->is_string()) {
	errs << "Parameter 6 (AAFname) in create_geom_subsys_aaf is not a string.\n";
	return apgen::RETURN_STATUS::FAIL;
    } else {
	spkez_intfc::AAFname() = n6->get_string();
    }
    if(!n5->is_array()) {
	errs << "Parameter 5 (frame_info) in create_geom_subsys_aaf is not an array.\n";
	return apgen::RETURN_STATUS::FAIL;
    } else {

	//
	// We have to do a little work here.
	//
	frame_list = &n5->get_array();
    }
    if(!n4->is_array()) {
	errs << "Parameter 4 (spkez_args) in create_geom_subsys_aaf is not an array.\n";
	return apgen::RETURN_STATUS::FAIL;
    } else {

	//
	// We have to do a little work here.
	//
	params = &n4->get_array();
    }
    if(!n3->is_numeric()) {
	errs << "Parameter 3 (relative error) in create_geom_subsys_aaf is not a number.\n";
	return apgen::RETURN_STATUS::FAIL;
    } else {
	spkez_intfc::theRelError = n3->get_double();
    }
    if(!n2->is_time()) {
	errs << "Parameter 2 (plan end) in create_geom_subsys_aaf is not a time.\n";
	return apgen::RETURN_STATUS::FAIL;
    } else {
	spkez_intfc::PlanEnd = n2->get_time_or_duration();
    }
    if(!n1->is_time()) {
	errs << "Parameter 1 (plan start) in create_geom_subsys_aaf is not a time.\n";
	return apgen::RETURN_STATUS::FAIL;
    } else {
	spkez_intfc::PlanStart = n1->get_time_or_duration();
    }

    spkez_intfc::caching_enabled = true;
    spkez_intfc::repeat_calls_only = false;
    spkez_intfc::subsystem_initialization_run = true;
    spkez_intfc::clear_cache();

    ArrayElement* ae;

    // debug
    cout << "create_geom_subsys_aaf - storing frame info:\n";

    for(int z = 0; z < frame_list->get_length(); z++) {
	ae = (*frame_list)[z];
	Cstring frame_name = ae->get_key();
	ListOVal* subarray = &ae->Val().get_array();
	ArrayElement* sub_period = subarray->find(Cstring("period (hrs)"));
	double period_in_hours = 0.0;

	if(sub_period->Val().is_string()) {

	    //
	    // Period is infinite
	    //
	    spkez_intfc::frame_info()[frame_name] = 0.0;
	} else {
	    period_in_hours = sub_period->Val().get_double();
	    spkez_intfc::frame_info()[frame_name] = period_in_hours;
	}

	// debug
	cout << "frame " << frame_name << " has period " << period_in_hours << "\n";
    }

    // debug
    cout << "\ncreate_geom_subsys_aaf - storing spkez parameters:\n";

    for(int z = 0; z < params->get_length(); z++) {
	ae = (*params)[z];
	ListOVal* subarray = &ae->Val().get_array();

	//
	// Insert params into the spkez hash table
	//
	spkez_intfc::data_index	one_value;

	one_value.first.first = (*subarray)[0]->Val().get_int();
	one_value.first.second = (*subarray)[1]->Val().get_string();
	one_value.second.first = (*subarray)[2]->Val().get_string();
	one_value.second.second = (*subarray)[3]->Val().get_int();
	spkez_intfc::master_list_of_spkez_arguments().push_back(one_value);

	// debug
	cout << "[" << one_value.first.first << ",\t" << addQuotes(one_value.first.second) << ",\t";
	cout << addQuotes(one_value.second.first) << ",\t" << one_value.second.second << "]\n";
    }

    //
    // At this point, all geometric resources should
    // be computed for the entire duration of the plan.
    //
    // Unlike the incremental algorithm used previously,
    // we are given an explicit list of spkez arguments
    // to prepare for, so we know exactly what to do.
    //
    // Also, because of the particular place of this
    // call in the overall adaptation, we know that
    // the spice kernels have been loaded already. So
    // we have everything we need to evaluate the
    // values of the geometric resources to the desired
    // accuracy.
    //
    try {
	spkez_intfc::generate_precomputed_spkez_AAF();
    } catch(eval_error Err) {
	errs = Err.msg;
	return apgen::RETURN_STATUS::FAIL;
    }

    *result = 0L;
    return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS spkez_control_ap(Cstring& errs, TypedValue* result, ARG_STACK) {
    TypedValue*	n1; // boolean enabled
    TypedValue*	n2; // double relative_error
    TypedValue* n3; // boolean check_errors
    TypedValue*	n4; // boolean repeat_calls_only
    TypedValue*	n5; // boolean dump_spkez_call_parameters
    TypedValue*	n6; // boolean check_precomp_res

    if( ARG_POINTER != ( WHERE_TO_STOP + 6 ) ) {
	errs << "  wrong number of parameters passed to user-defined function "
	    << "spkez_control; expected 6, got " << ( ARG_POINTER - WHERE_TO_STOP )
	    << "\n";
	return apgen::RETURN_STATUS::FAIL;
    }

    n6 = ARG_POP;
    n5 = ARG_POP;
    n4 = ARG_POP;
    n3 = ARG_POP;
    n2 = ARG_POP;
    n1 = ARG_POP;
    if(!n1->is_boolean()) {
	errs << "Parameter 1 (enabled) in spkez_control is not a boolean.\n";
	return apgen::RETURN_STATUS::FAIL;
    } else {
	spkez_intfc::caching_enabled = n1->get_int();
    }
    if(!n2->is_numeric()) {
	errs << "Parameter 2 (tolerance) in spkez_control is not a float.\n";
	return apgen::RETURN_STATUS::FAIL;
    } else {
	spkez_intfc::theRelError = n2->get_double();
    }
    if(!n3->is_boolean()) {
	errs << "Parameter 3 (check errors) in spkez_control is not a boolean.\n";
	return apgen::RETURN_STATUS::FAIL;
    } else {
	spkez_intfc::check_errors = n3->get_int();
    }
    if(!n4->is_boolean()) {
	errs << "Parameter n4 (repeat points only) in spkez_control is not a boolean.\n";
	return apgen::RETURN_STATUS::FAIL;
    } else {
	spkez_intfc::repeat_calls_only = n4->get_int();
	if(spkez_intfc::check_errors && spkez_intfc::repeat_calls_only) {
	    errs << "Cannot both check errors and cache repeat "
		"points only; there won't be any error.\n";
	    return apgen::RETURN_STATUS::FAIL;
	}
    }
    if(!n5->is_boolean()) {
	errs << "Parameter n5 (dump spkez call parameters) in spkez_control is not a boolean.\n";
	return apgen::RETURN_STATUS::FAIL;
    } else {
	spkez_intfc::dump_call_parameters = n5->get_int();
    }
    if(!n6->is_boolean()) {
	errs << "Parameter n6 (check precomputed resource) in spkez_control is not a boolean.\n";
	return apgen::RETURN_STATUS::FAIL;
    } else {
	spkez_intfc::check_precomp_res = n6->get_int();
	if(spkez_intfc::check_precomp_res) {
	    spkez_intfc::check_precomp_file = fopen("check_precomp_res.txt", "w");
	    spkez_intfc::check_errors = false;
	}
    }
    spkez_intfc::clear_cache();

    //
    // Make the function return something so it's easy to
    // invoke it from within a global variable definition
    //
    *result = 0L;
    return apgen::RETURN_STATUS::SUCCESS;
}

//
// Invoked at the beginning of the modeling process
//
void spkez_intfc::clear_cache() {

    //
    // New modeling run. Not necessary to clear the cache
    // completely; just reset all times, which will force
    // re-evaluation of all data points.
    //
    for(int i = 0; i < spkez_intfc::prev_spkez_call().size(); i++) {
	spkez_intfc::prev_spkez_call()[i].ptime = CTime_base();
	spkez_intfc::prev_spkez_call()[i].itime = CTime_base();
	spkez_intfc::prev_spkez_call()[i].ftime = CTime_base();
    }
    total_count = 0;
    spkez_count = 0;
    interpolated_count = 0;

    //
    // 1. Initialize influencers for the S/C
    // =====================================
    //
    // Unlike moons and planets, the S/C is influenced by
    // different bodies in different parts of its trajectory.
    // Our caching system is reactive, not proactive. Therefore,
    // everything will be driven by a new call to spkez. The
    // key question, when that call comes in, is whether we
    // can use interpolation or not.
    //
    // To use interpolation, we must have a sampling data point
    // for the specific parameters of the call at the sampling
    // time T_s, and we must have a future data point for those
    // same parameters at a time T_f that satisfies
    //
    //		T_f <= T_s + 1/F_s
    //
    // where F_s is the sampling frequency required to achieve
    // the desired accuracy.
    //
    // Let T1 be the time of the new call. Then, we may use
    // interpolation provided that
    //
    //		T_s <= T1 <= T_f.
    //
    // This takes care of how to deal with the case where data
    // are available at times T_s and T_f. But how do we
    // accumulate such data, starting from nothing?
    //
    // Suppose a new call to spkez comes in, and it concerns the
    // S/C (the "light body", obviously) and another body B.
    // Assuming we cannot use interpolation for lack of data, we
    // must call spkez and use the result of the call to update
    // what data we have regarding the S/C.
    //
    // If B is one of the 11 possible influencers (DSN stations
    // are a special case), then we must do the following. We
    // denote by N the node for the S/C in bodies_of_interest().
    // Then, N->payload is an influencers object, which is
    // basically a tlist of influencer nodes. In principle, we
    // should
    //
    //	- update the key of the influencer node in N->payload
    //	  to reflect the new distance from the S/C to it
    //
    //  - update the sampling time of that influencer node
    //    based on the state returned by spkez
    //
    // Fortunately, the second item is not necessary because the
    // adaptation always updates all influencers at once.
    //
    // 2. Initialize influencers for celestial bodies
    // ==============================================
    //
    // Since we are only interested in planets and a few of their
    // moons, the problem is much simpler: only once influencer
    // needs to be considered. See comments in the code below.
    //
    spkez_intfc::bodies_of_interest().clear();

    //
    // Note: S/C ID = -159
    //
    spkez_intfc::body_state* light = new spkez_intfc::body_state(-159);

    spkez_intfc::bodies_of_interest() << light;

    //
    // Initially, each influencer is assumed in contact
    // w/ the S/C (R = 0). This makes it impossible to
    // compute a sampling frequency, as long as any
    // potential influencer is missing from the data.
    //
 
    //
    // Reference for the following table: pck.html in the NAIF
    // documentation. Note that n is the n-th planetary barycenter,
    // and n99 is the n-th planet itself.
    //

	//
	// Possible influencers for the S/C
	// ================================
	//
	// Note: 1 AU = 150 million km
	//
	//	Body		ID
	//	====		==
	//	Sun		10
	//	Venus		2
	//	Earth		399	[DSN station	399ddd]
	//	Moon		301
	//	Mars		4
	//	Jupiter		5
	//	Io		501
	//	Europa		502
	//	Ganymede	503
	//	Callisto	504
	//	Saturn		6
	//

    // Sun
    light->payload << new spkez_intfc::influencer(0L, 10);
    // Venus
    light->payload << new spkez_intfc::influencer(0L, 2);
    // Earth
    light->payload << new spkez_intfc::influencer(0L, 399);
    // Moon
    light->payload << new spkez_intfc::influencer(0L, 301);
    // Mars
    light->payload << new spkez_intfc::influencer(0L, 4);
    // Jupiter
    light->payload << new spkez_intfc::influencer(0L, 5);
    // Io
    light->payload << new spkez_intfc::influencer(0L, 501);
    // Europa
    light->payload << new spkez_intfc::influencer(0L, 502);
    // Ganymede
    light->payload << new spkez_intfc::influencer(0L, 503);
    // Callisto
    light->payload << new spkez_intfc::influencer(0L, 504);
    // Saturn
    light->payload << new spkez_intfc::influencer(0L, 6);
    assert(light->payload.infl_in_incr_abs_err_order.get_length() == 11
	   || light->payload.infl_in_incr_abs_err_order.get_length() == 1);

    //
    // For light bodies other than the S/C, we only need one
    // influencer, as shown in the table below.
    //

	//
	// Influencers for planets and moons
	// =================================
	//
	//	Light Body	ID	Heavy Body	ID
	//	----------	--	----------	--
	//	Venus Bary	2	Sun Bary	10
	//	Earth		399	Earth Bary	3
	//	Moon		301	Earth Bary	3
	//	Earth Bary	3	Sun Bary	10
	//	Mars Bary	4	Sun Bary	10
	//	Jupiter Bary	5	Sun Bary	10
	//	Io		501	Jupiter Bary	5
	//	Europa		502	Jupiter Bary	5
	//	Ganymede	503	Jupiter Bary	5
	//	Callisto	504	Jupiter Bary	5
	//	Saturn		6	Sun Bary	10
	//

    // Venus Bary
    light = new spkez_intfc::body_state(2);
    spkez_intfc::bodies_of_interest() << light;
	// Sun
	light->payload << new spkez_intfc::influencer(0L, 10);

    // Earth
    light = new spkez_intfc::body_state(399);
    spkez_intfc::bodies_of_interest() << light;
	// Earth Bary
	light->payload << new spkez_intfc::influencer(0L, 3);

    // Earth Bary
    light = new spkez_intfc::body_state(3);
    spkez_intfc::bodies_of_interest() << light;
	// Sun
	light->payload << new spkez_intfc::influencer(0L, 10);

    // Moon
    light = new spkez_intfc::body_state(301);
    spkez_intfc::bodies_of_interest() << light;
	// Earth Bary
	light->payload << new spkez_intfc::influencer(0L, 3);

    // Mars Bary
    light = new spkez_intfc::body_state(4);
    spkez_intfc::bodies_of_interest() << light;
	// Sun
	light->payload << new spkez_intfc::influencer(0L, 10);

    // Jupiter Bary
    light = new spkez_intfc::body_state(5);
    spkez_intfc::bodies_of_interest() << light;
	// Sun
	light->payload << new spkez_intfc::influencer(0L, 10);

    // Io
    light = new spkez_intfc::body_state(501);
    spkez_intfc::bodies_of_interest() << light;
	// Jupiter Bary
	light->payload << new spkez_intfc::influencer(0L, 5);

    // Europa
    light = new spkez_intfc::body_state(502);
    spkez_intfc::bodies_of_interest() << light;
	// Jupiter Bary
	light->payload << new spkez_intfc::influencer(0L, 5);

    // Ganymede
    light = new spkez_intfc::body_state(503);
    spkez_intfc::bodies_of_interest() << light;
	// Jupiter Bary
	light->payload << new spkez_intfc::influencer(0L, 5);

    // Callisto
    light = new spkez_intfc::body_state(504);
    spkez_intfc::bodies_of_interest() << light;
	// Jupiter Bary
	light->payload << new spkez_intfc::influencer(0L, 5);

    // Saturn Bary
    light = new spkez_intfc::body_state(6);
    spkez_intfc::bodies_of_interest() << light;
	// Sun
	light->payload << new spkez_intfc::influencer(0L, 10);
	assert(light->payload.infl_in_incr_abs_err_order.get_length() == 11
	   || light->payload.infl_in_incr_abs_err_order.get_length() == 1);

    //
    // ZONE OF INFLUENCE
    //
    if(check_errors) {
	static CTime_base	zerotime;
	zone_file = fopen("zone.aaf", "w");
	zone_time() = zerotime;
	aoString zone_info;
	zone_info << "apgen version \"zone info\"\n";

	zone_info << "resource influencer: state string\n";
	zone_info << "    begin\n";
	zone_info << "        attributes\n";
	zone_info << "            \"Subsystem\" = \"Geometry\";\n";
	zone_info << "        parameters\n";
	zone_info << "            x: string default to \"Sun \";\n";
	zone_info << "        states\n";
	zone_info << "            \"Sun \", \"Eart\", \"Moon\", \"Venu\", \"Jupi\", \"Call\", \"Gany\", \"Euro\", \"Io  \";\n";
	zone_info << "        profile\n";
	zone_info << "            \"Sun \";\n";
	zone_info << "        usage\n";
	zone_info << "            x;\n";
	zone_info << "    end resource influencer\n";

	zone_info << "resource sampling_time: settable duration\n";
	zone_info << "    begin\n";
	zone_info << "        attributes\n";
	zone_info << "            \"Subsystem\" = \"Geometry\";\n";
	zone_info << "        parameters\n";
	zone_info << "            x: duration default to 00:00:01;\n";
	zone_info << "        default\n";
	zone_info << "            00:00:00;\n";
	zone_info << "        usage\n";
	zone_info << "            x;\n";
	zone_info << "    end resource sampling_time\n";

	zone_info << "resource influence_info: abstract\n";
	zone_info << "    begin\n";
	zone_info << "        parameters\n";
	zone_info << "            body: string default to \"Sun \";\n";
	zone_info << "            sampling: duration default to 00:00:00;\n";
	zone_info << "        resource usage\n";
	zone_info << "            set influencer(body);\n";
	zone_info << "            set sampling_time(sampling);\n";
	zone_info << "    end resource influence_info\n";
	zone_info << "\n";
	zone_info << "activity type set_influencers\n";
	zone_info << "    begin\n";
	zone_info << "        attributes\n";
	zone_info << "            \"Legend\" = \"Influencers\";\n";
	zone_info << "            \"Subsystem\" = \"Geometry\";\n";
	zone_info << "            \"Color\" = \"Spring Green\";\n";
	zone_info << "            \"Duration\" = 1T00:00:00;\n";
	zone_info << "        modeling\n";

	Cstring zonestr(zone_info.str());
	fwrite(*zonestr, 1, zonestr.length(), zone_file);
    }
}

void spkez_intfc::last_call() {
    // dump_parameters("spkez_param_dump.aaf");
}

void spkez_intfc::dump_parameters(const Cstring& filename) {
    stringtlist	frames(true);
    aoString	aos;
    FILE*	aos_file = fopen(*filename, "w");

    if(!aos_file) {
	Cstring err;
	err << "spkez::dump_parameters: cannot open file "
	    << filename << ".\n";
	throw(eval_error(err));
    }

    //
    // Dump all the argument parameters stored in the spkez_args()
    //
    aos << "\n" "# target\tframe\tabcorr\tobserver\n\n";
    aos << "constant array spkez_info_array = [\n";
    for(int w = 0; w < spkez_args().size(); w++) {
	    data_index& index = spkez_args()[w];
	    aos << "\t[" << index.first.first
		<< ",\t" << addQuotes(index.first.second) << ",\t";
	    if(!frames.find(index.first.second)) {
		frames << new emptySymbol(index.first.second);
	    }
	    aos << addQuotes(index.second.first) << ",\t" << index.second.second << "]";
	    if(w != spkez_args().size() - 1) {
		aos << ",";
	    }
	    aos << " # " << w << "\n";
    }

    //
    // The frames used in the Europa Clipper adaptation
    // are of class 1 (inertial), 2 (body-fixed) or 5 (dynamic).
    // Inertial frames need no correction at all to the
    // interpolation error formula. Body-fixed frames are
    // attached to a body which is listed as the class frame,
    // so we can figure out the error correction from the
    // rotation rate of that body.  In principle we should
    // also consider the error correction due to the motion
    // of that body, but that should already be taken into
    // account because the body in question is always the
    // observer or the target of the SPKEZ call. This was
    // verified for the approach regression test.
    //
    // Dynamic frames could get complicated, but in practice
    // the ones used by Europa Clipper are fixed with respect
    // to a certain body frame which can be retrieved from
    // spice as a kernel variable with a specific name.
    //
    // Actually, a much simpler method is based on the fact
    // that the center ID already refers to the body that
    // the frame is attached to.  So, we can just use that.
    //
    aos << "];\n\n # Frames:\n\nconstant array FrameInformation = [\n";

    for(emptySymbol* es = frames.first_node(); es; es = es->next_node()) {
	    long int center_ID, frame_class, class_frame_ID;
	    if(FrameInfo(es->get_key(), center_ID, frame_class, class_frame_ID)) {


		//
		// As explained above, we identify rotating
		// frames based on the center information.
		// The corresponding body rotation rate (i. e.,
		// angular velocity) can be used to evaluate
		// the interpolation error.
		//
		// To get the angular velocity, we use the
		// xf2rav(xform, rot, av) Spice function.
		// To get the transformation matrix xform,
		// we use the sxform(from, to, et, xform)
		// Spice function.
		//
		// We only need a rough value for the angular
		// velocity, since all we are doing is estimating
		// the error. So, we should only check the value
		// of the angular velocity at a few points within
		// the horizon of interest. 
		//
		// To start, let us compute the angular velocity
		// at the mid point of the horizon.
		//
		CTime_base MidPoint = PlanStart + (PlanEnd - PlanStart) * 0.5;
		double middouble = MidPoint.convert_to_double_use_with_caution();
		double angular_vel[3];
		FrameAngularVelocity("J2000", *es->get_key(), middouble, angular_vel);
		double omega = angular_vel[0] * angular_vel[0]
			       + angular_vel[1] * angular_vel[1]
			       + angular_vel[2] * angular_vel[2];
		omega = sqrt(omega);
		double	period_in_hours = -1.0;
		if(omega > 1.0E-15) {
		    period_in_hours = (2 * M_PI / omega) / 3600.0;
		}

		aos << addQuotes(es->get_key()) << " = "
		     << "[\"center\" = " << center_ID << ", "
		     << "\"class\" = " << frame_class << ", "
		     << "\"class frame\" = " << class_frame_ID << ", "
		     << "\"period (hrs)\" = ";
		if(period_in_hours < 0.0) {
		    aos << "\"infinity\"]";
		} else {
		    aos << period_in_hours << "]";
		}

		if(es->next_node()) {
		    aos << ",\n";
		} else {
		    aos << "\n";
		}
	    } else {
		aos << "unknown\n";
	    }
    }
    aos << "];\n";
    Cstring tmp = aos.str();
    fwrite(*tmp, 1, tmp.length(), aos_file);
    fclose(aos_file);
}

//
// Invoked at the end of the modeling process. Good place
// to output statistics.
//
void spkez_intfc::clean_up_cache() {

    if(check_errors) {

	cout << "spkez calls: " << spkez_count << " out of " << total_count << "\n";
	cout << "interpolated calls: " << interpolated_count << " out of " << total_count << "\n";

	//
	// ZONE OF INFLUENCE
	//
	if(zone_file) {

	    //
	    // Write the instance
	    //
	    aoString zone_info;
	    zone_info << "    end activity type set_influencers\n";

	    zone_info << "activity instance Influence of type set_influencers\n";
	    zone_info << "    begin\n";
	    zone_info << "        attributes\n";
	    zone_info << "            \"Start\" = " << zone_time().to_string() << ";\n";
	    zone_info << "    end activity instance Influence\n";
	    Cstring zonestr(zone_info.str());
	    fwrite(*zonestr, 1, zonestr.length(), zone_file);
	    fclose(zone_file);
	}
	map<int, FILE*>::const_iterator error_iter;
	aoString s;
	char buf[4];
	for(error_iter = aaf_files().begin(); error_iter != aaf_files().end(); error_iter++) {
	    sprintf(buf, "%2d", error_iter->first);
	    if(buf[0] == ' ') buf[0] = '0';
	    s << "    end activity type set_spkez_errors_" << buf << "\n";
	    s << "activity instance SpkezErrorReporter of type set_spkez_errors_" << buf << "\n";
	    s << "    begin\n";
	    s << "        attributes\n";
	    s << "            \"Subsystem\" = \"Geometry\";\n";
	    s << "            \"Start\" = " << aaf_starts()[error_iter->first] << ";\n";
	    s << "    end activity instance SpkezErrorReporter\n";
	    Cstring t(s.str());
	    fwrite(*t, 1, t.length(), error_iter->second);
	    fclose(error_iter->second);
	}
	aaf_files().clear();
	s.clear();
	s << "apgen version true\n";
	s << "global array spkez_error_indices = [\n";
	FILE* generic = fopen("generic_spk_analysis.aaf", "w");
	for(int k = 0; k < aaf_indices().size(); k++) {
	    if(k > 0) s << ", ";
	    if(!((k+1) % 5)) s << "\n\t";
	    s << "\"" << aaf_indices()[k] << "\"";
	}
	s << "];\n";
	s << "resource SPKEZ( spkez_error_indices ): settable float\n";
	s << "    begin\n";
	s << "        attributes\n";
	s << "            \"Subsystem\" = \"Geometry\";\n";
	s << "            \"Units\" = \"log_10 of rel. error\";\n";
	s << "            \"Maximum\" = -4.0;\n";
	s << "        parameters\n";
	s << "            x: float default to 0.0;\n";
	s << "        default\n";
	s << "            0.0;\n";
	s << "        usage\n";
	s << "            x;\n";
	s << "    end resource SPKEZ\n";
	Cstring t(s.str());
	fwrite(*t, 1, t.length(), generic);
	fclose(generic);

	//
	// We only generate data for the first modeling run. You
	// need to change this logic if you want to collect data
	// for multiple runs; for example, define multiple sets
	// of error report adaptation files.
	//
	check_errors = false;
    }

    if(spkez_intfc::check_precomp_res) {
	aoString aos;
	for(int h = 0; h < spkez_args().size(); h++) {
	    data_index& di = spkez_args()[h];
	    aos << di.first.first << ", "
		<< di.first.second << ", "
		<< di.second.first << ", "
		<< di.second.second << ": max err = "
		<< max_error()[h] << " at "
		<< time_at_max_error()[h].to_string() << "\n";
	}
	Cstring t(aos.str());
	fwrite(*t, 1, t.length(), check_precomp_file);
	fclose(check_precomp_file);
	check_precomp_file = NULL;
    }
}

//
// Given the target or observer of a call to spkez,
// this function returns the most closely related
// influencer in the list of influencers that is
// stored in bodies_of_interest.
//
long int related_influencer(long target) {
    bool DSN_station		= target > 399000 && target < 400000;
    bool RadioTelescope		= target == -159931 || target == -159932;
    bool Fixed_S_C_point	= !RadioTelescope && (target > -160000 && target < -159000);
    if(DSN_station) {
	// Earth Barycenter
	return 3;
    }
    if(RadioTelescope) {
	// Earth Barycenter
	return 3;
    }
    if(Fixed_S_C_point) {
	// Any point that is fixed with respect to the S/C
	return -159;
    }
    switch(target) {
	case 10:
	    // Sun Barycenter
	    return 0;
	case 399:
	    // Earth Barycenter
	    return 3;
	case 599:
	    // Jupiter - never an influencer;
	    // Jupiter Barycenter is used instead
	    return -1;
	case -159:
	case 0:
	case 2:
	case 3:
	case 301:
	case 4:
	case 5:
	case 501:
	case 502:
	case 503:
	case 504:
	case 6:
	    return target;
	default:
	    // Unknown body
	    return -1;
    }
}

const char* name_this_object(long target, bool append_blanks) {
    static char buf[80];
    bool DSN_station = target > 399000 && target < 400000;
    bool RadioTelescope = target == -159931 || target == -159932;
    bool Fixed_S_C_point = !RadioTelescope && (target > -160000 && target < -159000);
    int which_station = 0;
    if(DSN_station) {
	which_station = target - 399000;
	target = 3991;
    }
    if(RadioTelescope) {
	which_station = target + 159000;
	target = 3991;
    }
    if(Fixed_S_C_point) {
	target = -1591;
    }
    switch(target) {
	case -159:
	    if(append_blanks) {
		return "S/C         ";
	    } else {
		return "S/C ";
	    }
	case -1591:
	    if(append_blanks) {
		return "S/C pt      ";
	    } else {
		return "S/Cp";
	    }
	case 0:
	    if(append_blanks) {
		return "Solar Bary  ";
	    } else {
		return "Sola";
	    }
	case 2:
	    if(append_blanks) {
		return "Venus Bary  ";
	    } else {
		return "Venu";
	    }
	case 10:
	    if(append_blanks) {
		return "Sun Bary    ";
	    } else {
		return "Sun ";
	    }
	case 3:
	    if(append_blanks) {
		return "Earth Bary  ";
	    } else {
		return "ErtB";
	    }
	case 399:
	    if(append_blanks) {
		return "Earth       ";
	    } else {
		return "Eart";
	    }
	case 3991:
	    if(append_blanks) {
		sprintf(buf, "DSN %3d     ", which_station);
		return buf;
	    } else {
		sprintf(buf, "D%3d", which_station);
		return buf;
	    }
	case 301:
	    if(append_blanks) {
		return "Moon        ";
	    } else {
		return "Moon";
	    }
	case 4:
	    if(append_blanks) {
		return "Mars Bary   ";
	    } else {
		return "Mars";
	    }
	case 5:
	    if(append_blanks) {
		return "Jupiter Bary";
	    } else {
		return "Jupi";
	    }
	case 599:
	    if(append_blanks) {
		return "Jupiter     ";
	    } else {
		return "Jupi";
	    }
	case 501:
	    if(append_blanks) {
		return "Io          ";
	    } else {
		return "Io  ";
	    }
	case 502:
	    if(append_blanks) {
		return "Europa      ";
	    } else {
		return "Euro";
	    }
	case 503:
	    if(append_blanks) {
		return "Ganymede    ";
	    } else {
		return "Gany";
	    }
	case 504:
	    if(append_blanks) {
		return "Callisto    ";
	    } else {
		return "Call";
	    }
	case 6:
	    if(append_blanks) {
		return "Saturn Bary ";
	    } else {
		return "Satu";
	    }
	default:
	    if(append_blanks) {
		sprintf(buf, "%ld           ", target);
	    } else {
		sprintf(buf, "%ld", target);
	    }
	    return buf;
    }
}

//
// GM in units km^3 / s^2
//
double GM_for_target(long target) {

    //
    // Data from Wikipedia:
    //
    switch(target) {
	case 10:
	    // Sun Barycenter
	    return 1.327E+11;
	case 2:
	    // Venus
	    return 3.25E+5;
	case 3:
	case 399:
	    // Earth <Barycenter>
	    return 3.99E+5;
	case 301:
	    // Moon
	    return 4.90E+3;
	case 4:
	    // Mars
	    return 4.26E+4;
	case 5:
	case 599:
	    // Jupiter <Barycenter>
	    return 1.27E+8;
	case 501:
	    // Io
	    return 5.96E+3;
	case 502:
	    // Europa
	    return 3.20E+3;
	case 503:
	    // Ganymede
	    return 9.88E+3;
	case 504:
	    // Callisto
	    return 7.21E+3;
	case 6:
	    // Saturn
	    return 3.79E+7;
	default:
	    return -1.0;
    }
}

apgen::RETURN_STATUS spkez_intfc::spice_spkez_ap(
			Cstring& errs,
			TypedValue* result,
			ARG_STACK) {
    TypedValue	* n1, // "targ"
                * n2, // "utc"
                * n3, // "ref"
                * n4, // "abcorr"
                * n5; // "obs"
    double	atarg[6];
    double	lt;
    errs.undefine();
    if( ARG_POINTER != ( WHERE_TO_STOP + 5 ) ) {
	errs << "  wrong number of parameters passed to "
		<< "user-defined function spkez_api; expected 5, got "
		<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n";
	return apgen::RETURN_STATUS::FAIL;
    }
    n5 = ARG_POP;
    n4 = ARG_POP;
    n3 = ARG_POP;
    n2 = ARG_POP;
    n1 = ARG_POP;

    //
    // Catch all arguments as APGenX-style data objects
    //
    long	target;
    Cstring	ref_frame;
    Cstring	abcorr;
    long	observer;
    CTime_base	nowval;

    if( n1->get_type() == apgen::DATA_TYPE::INTEGER ) {
	target = n1->get_int();
    } else {
	errs << "spkez_api_ap: first argument is not a int\n";
	return apgen::RETURN_STATUS::FAIL;
    }
    if( n2->get_type() == apgen::DATA_TYPE::TIME ) {
	nowval = n2->get_time_or_duration();
    } else {
	errs << "spkez_api_ap: second argument is not a time\n";
	return apgen::RETURN_STATUS::FAIL;
    }
    if( n3->is_string() ) {
	ref_frame = n3->get_string();
    } else {
	errs << "spkez_api_ap: third argument is not a string\n";
	return apgen::RETURN_STATUS::FAIL;
    }
    if( n4->is_string() ) {
	abcorr = n4->get_string();
    } else {
	errs << "spkez_api_ap: fourth argument is not a string\n";
	return apgen::RETURN_STATUS::FAIL;
    }
    if( n5->get_type() == apgen::DATA_TYPE::INTEGER  ) {
	observer = n5->get_int();

	//
	// Outlaw observer = target situations
	//
	if(observer == target) {
	    errs << "spkez_api_ap: target is the same as observer\n";
	    return apgen::RETURN_STATUS::FAIL;
	}
    } else {
	errs << "spkez_api_ap: fifth argument is not a integer\n";
	return apgen::RETURN_STATUS::FAIL;
    }

    try {
	double nowdouble = nowval.convert_to_double_use_with_caution();
	spkez_api(
		target,
		nowdouble,
		*ref_frame,
		*abcorr,
		observer,
		atarg,
		&lt);
    } catch(eval_error Err) {
	errs = Err.msg;
	return apgen::RETURN_STATUS::FAIL;
    }
    *result = create_float_list(6, atarg);
    return apgen::RETURN_STATUS::SUCCESS;
}

//
// Previously implemented in udef_functions.C. Migrated here to
// facilitate implementation of a caching mechanism that catches
// calls with identical or nearly identical arguments.
//
apgen::RETURN_STATUS spkez_api_ap(
			Cstring& errs,
			TypedValue* result,
			ARG_STACK) {
    spkez_intfc::total_count++;
    return spkez_intfc::call_spkez_if_necessary(errs, result, args);
}

apgen::RETURN_STATUS spkez_intfc::call_spkez_if_necessary(
			Cstring& errs,
			TypedValue* result,
			ARG_STACK) {
    TypedValue	* n1, // "targ"
                * n2, // "utc"
                * n3, // "ref"
                * n4, // "abcorr"
                * n5; // "obs"
    double	atarg[6];
    double	lt;
    errs.undefine();
    if( ARG_POINTER != ( WHERE_TO_STOP + 5 ) ) {
	errs << "  wrong number of parameters passed to "
		<< "user-defined function spkez_api; expected 5, got "
		<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n";
	return apgen::RETURN_STATUS::FAIL;
    }
    n5 = ARG_POP;
    n4 = ARG_POP;
    n3 = ARG_POP;
    n2 = ARG_POP;
    n1 = ARG_POP;

    //
    // Catch all arguments as APGenX-style data objects
    //
    long	target;
    Cstring	ref_frame;
    Cstring	abcorr;
    long	observer;
    CTime_base	nowval;

    if( n1->get_type() == apgen::DATA_TYPE::INTEGER ) {
	target = n1->get_int();
    } else {
	errs << "spkez_api_ap: first argument is not a int\n";
	return apgen::RETURN_STATUS::FAIL;
    }
    if( n2->get_type() == apgen::DATA_TYPE::TIME ) {
	nowval = n2->get_time_or_duration();
    } else {
	errs << "spkez_api_ap: second argument is not a time\n";
	return apgen::RETURN_STATUS::FAIL;
    }
    if( n3->is_string() ) {
	ref_frame = n3->get_string();
    } else {
	errs << "spkez_api_ap: third argument is not a string\n";
	return apgen::RETURN_STATUS::FAIL;
    }
    if( n4->is_string() ) {
	abcorr = n4->get_string();
    } else {
	errs << "spkez_api_ap: fourth argument is not a string\n";
	return apgen::RETURN_STATUS::FAIL;
    }
    if( n5->get_type() == apgen::DATA_TYPE::INTEGER  ) {
	observer = n5->get_int();

	//
	// Outlaw observer = target situations
	//
	if(observer == target) {
	    errs << "spkez_api_ap: target is the same as observer\n";
	    return apgen::RETURN_STATUS::FAIL;
	}
    } else {
	errs << "spkez_api_ap: fifth argument is not a integer\n";
	return apgen::RETURN_STATUS::FAIL;
    }

    //
    // Return if caching is not enabled
    //
    if(!caching_enabled) {
	try {
	    double nowdouble = nowval.convert_to_double_use_with_caution();
	    spkez_api(
			target,
			nowdouble,
			*ref_frame,
			*abcorr,
			observer,
			atarg,
			&lt);
	} catch(eval_error Err) {
	    errs = Err.msg;
	    return apgen::RETURN_STATUS::FAIL;
	}
	*result = create_float_list(6, atarg);
	return apgen::RETURN_STATUS::SUCCESS;
    }

    //
    // Look for a previous occurrence of the same parameters
    //
    data_index indx( pair<long int, Cstring>(target, ref_frame),
		     pair<Cstring, long int>(abcorr, observer));

    data_map::const_iterator iter = spkez_arg_map().find(indx);

    int hash_value = -1;

    bool valid_data_exist = false;

    if(iter == spkez_arg_map().end()) {
	hash_value = spkez_args().size();
	spkez_arg_map()[indx] = hash_value;
	spkez_args().push_back(indx);
	max_error().push_back(0.0);
	time_at_max_error().push_back(infl_data::deluge());
    } else {
	valid_data_exist = true;
	hash_value = iter->second;
    }

    if(!valid_data_exist) {
	prev_spkez_call().push_back(call_data(indx, hash_value));
    }

    call_data& previous_data = prev_spkez_call()[hash_value];

    static CTime_base	zerotime;

    //
    // Figure out how "now" relates to the time of
    // the previous call; return immediately if
    // this is a repeated call.
    //
    if(valid_data_exist) {
	static CTime_base	zerodur("00:00:00");
	CTime_base		deltaT = nowval - previous_data.ptime;

	if(deltaT == zerodur) {

	    for(int i = 0; i < 6; i++) {
		atarg[i] = previous_data.presult[i];
	    }
            *result = create_float_list(6, atarg);
	    return apgen::RETURN_STATUS::SUCCESS;
	}
    }

    //
    // The next if block is what gets executed normally,
    // i. e., when the repeat_calls_only flag is set to true.
    //
    if(repeat_calls_only) {

	try {
	    previous_data.call_spkez(nowval, result);
	} catch(eval_error Err) {
	    errs = Err.msg;
	    return apgen::RETURN_STATUS::FAIL;
	}

	return apgen::RETURN_STATUS::SUCCESS;
    } else {
	errs << "spkez_api: repeat_calls_only set to false - no longer supported.\n";
	return apgen::RETURN_STATUS::FAIL;
    }

}

void	spkez_intfc::call_data::call_spkez(
			CTime_base	nowval,
			TypedValue*	result) {

    double	nowdouble = nowval.convert_to_double_use_with_caution();
    double	atarg[6];
    double	lt;

    //
    // can throw
    //
    spkez_api(	target,
		nowdouble,
		*ref_frame,
		*abcorr,
		observer,
		atarg,
		&lt);

    *result = create_float_list(6, atarg);

    //
    // Store the results, so we can handle repeat cases
    //
    ptime = nowval;
    for(int z = 0; z < 6; z++) {
	presult[z] = atarg[z];
    }
}

//
// This method determines the basic geometry of the call
// to spkez:
//
//	- among the observer and target, which is the light_body
//
//	- among the observer and target, which is the relative_body
//
//	- compute the GM parameter for the relative_body
//
spkez_intfc::body_state* spkez_intfc::call_data::determine_geometry() {
    body_state*	light = NULL;

    if(initialized) {
	light = bodies_of_interest().find(light_body);
	return light;
    }

    initialized = true;

    //
    // Figure out which is the heavier body, which is lighter
    //
    bool RadioTelescope_is_target = target == -159931 || target == -159932;
    bool RadioTelescope_is_observer = observer == -159931 || observer == -159932;
    bool SC_is_target = target == -159 || (target != -159931 && target != -159932
				&& target > -160000 && target < -159000);
    bool SC_is_observer = observer == -159 || (observer != -159931 && observer != -159932
				&& observer > -160000 && observer < -159000);
    bool Sun_is_target = target == 10;
    bool Sun_is_observer = observer == 10;

    //
    // We 'cheat' for DSN stations and treat them as if they
    // were at the Earth (bary)center. For error estimation
    // purposes, this is OK although it needs to be proved
    // in detail.
    //
    bool Earth_is_target = (target == 399 || target == -159931 || target == -159932
				|| (target > 399000 && target < 400000));
    bool Earth_is_observer = (observer == 399 || observer == -159931 || observer == -159932
				|| (observer > 399000 && observer < 400000));
    bool Moon_is_target = target == 301;
    bool Moon_is_observer = observer == 301;
    bool Venus_is_target = target == 2;
    bool Venus_is_observer = observer == 2;
    bool Mars_is_target = target == 4;
    bool Mars_is_observer = observer == 4;
    bool Saturn_is_target = target == 6;
    bool Saturn_is_observer = observer == 6;
    bool Jup_Moon_is_target = target   == 501 || target   == 502 || target   == 503 || target   == 504;
    bool Jup_Moon_is_observer = observer == 501 || observer == 502 || observer == 503 || observer == 504;
    bool Jupiter_is_target = target == 5 || target == 599;
    bool Jupiter_is_observer = observer == 5 || observer == 599;

    if(SC_is_target || SC_is_observer) {

	//
	// The lightest of all
	// Note: we 'cheat' if we are dealing with
	// a DSN station, but only in the estimate.
	//
	if(SC_is_target) {
	    light_body = target;
	    relative_body = observer;
	} else if(SC_is_observer) {
	    light_body = observer;
	    relative_body = target;
	}

	if(light_body > -160000 && light_body < -159000) {

	    //
	    // Fixed point on S/C
	    //
	    light_body = -159;
	}

	if(relative_body > 399000 && relative_body < 400000) {

	    //
	    // DSN station
	    //
	    relative_body = 399;
	}
	if(relative_body == -159931 || relative_body == -159932) {

	    //
	    // RadioTelescope
	    //
	    relative_body = 399;
	}
    } else {

	//
	// List bodies in decreasing order of mass
	//
	if(Sun_is_target) {
	    relative_body = target;
	    light_body = observer;
	} else if(Sun_is_observer) {
	    relative_body = observer;
	    light_body = target;
	} else if(Jupiter_is_target) {
	    relative_body = target;
	    light_body = observer;
	} else if(Jupiter_is_observer) {
	    relative_body = observer;
	    light_body = target;
	} else if(Saturn_is_target) {
	    relative_body = target;
	    light_body = observer;
	} else if(Saturn_is_observer) {
	    relative_body = observer;
	    light_body = target;
	} else if(Earth_is_target) {
	    relative_body = target;
	    light_body = observer;
	} else if(Earth_is_observer) {
	    relative_body = observer;
	    light_body = target;
	} else if(Venus_is_target) {
	    relative_body = target;
	    light_body = observer;
	} else if(Venus_is_observer) {
	    relative_body = observer;
	    light_body = target;
	} else if(Mars_is_target) {
	    relative_body = target;
	    light_body = observer;
	} else if(Mars_is_observer) {
	    relative_body = observer;
	    light_body = target;
	} else if(Jup_Moon_is_target) {
	    relative_body = target;
	    light_body = observer;
	} else if(Jup_Moon_is_observer) {
	    relative_body = observer;
	    light_body = target;
	} else if(Moon_is_target) {
	    relative_body = target;
	    light_body = observer;
	} else if(Moon_is_observer) {
	    relative_body = observer;
	    light_body = target;
	}
    }
    GM = GM_for_target(relative_body);

    //
    // Make sure we've covered all possibilities
    //
    if(GM < 0.0) {
	Cstring errs("spkez error: not all GM parameters are known.");
	errs << "\nCould not find " << name_this_object(relative_body, false);
	throw(eval_error(errs));
    }

    light = bodies_of_interest().find(light_body);

    //
    // Sanity check
    //
    assert(light);
    assert(light->payload.infl_in_incr_abs_err_order.get_length() == 11
	   || light->payload.infl_in_incr_abs_err_order.get_length() == 1);
    return light;
}

void spkez_intfc::call_data::output_debug_info(
		aoString&	out_str) {

    //
    // NOTE: no closing parenthesis!!
    //
    out_str << "SPKEZ_";
    char buf[4];
    sprintf(buf, "%2d", hashval);
    out_str << buf << "\t(\""
	<< name_this_object(target, true)
	<< "\", \"" << name_this_object(observer, false)
	<< "\", \"" << abcorr
	<< "\"";
}
