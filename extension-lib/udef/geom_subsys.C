#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define AU 150000000L // A. U. in km (approximate)

#include <sstream>

//
// TODO: replace integer constants -159, 10, 399 etc. by
// appropriate symbolic constants defined in the SPICE toolkit.
//

#include "spiceIntfc.H"
#include "GLOBdata.H"

#define ARG_POINTER args.size()
#define ARG_POP args.pop_front()
#define ARG_STACK slst<TypedValue*>& args
#define WHERE_TO_STOP 0

vector<spkez_intfc::cached_spkez_data>&	spkez_intfc::cached_data_vector() {
	static vector<cached_spkez_data> C;
	return C;
}

map<spkez_intfc::data_index, long int>&	spkez_intfc::cached_arg_map() {
	static map<data_index, long int> M;
	return M;
}

//
// The default values for the parameters that will be
// set by init_geom_subsys_ap() below are defined in
// spkez_interface.C in this directory.
//

//
// Arguments are PlanStart, PlanEnd, relative_error. Tells
// the spkez subsystem that this run is intended to produce
// spkez interpolation coefficients files (SICF). Such files
// can be used to interpolate spkez over the entire span of
// a plan.
//
// TO DO: provide an AAF file name. The AAF will encode, in
// ASCII format, all the necessary values of Geometry
// resources as (time, value) pairs. The sampling times
// will be frequent enough to guarantee that interpolation
// will provide values that are within the requested
// accuracy.
//
apgen::RETURN_STATUS create_geom_subsys_ap(
			Cstring& errs,
			TypedValue* result,
			ARG_STACK) {
    TypedValue*	n1; // CTime_base	start
    TypedValue*	n2; // CTime_base	end
    TypedValue*	n3; // double		relative_error
    TypedValue* n4; // Cstring		TCM info file
    TypedValue* n5; // Cstring		JOI info file
    TypedValue*	n6; // Cstring		AAF_file_name

    if( ARG_POINTER != ( WHERE_TO_STOP + 6 ) ) {
	errs << "  wrong number of parameters passed to user-defined function "
	    << "create_geom_subsys; expected 6, got " << ( ARG_POINTER - WHERE_TO_STOP )
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
	errs << "Parameter 6 (AAF file name) in create_geom_subsys is not a string.\n";
	return apgen::RETURN_STATUS::FAIL;
    } else {
	spkez_intfc::theAAFFileName() = n6->get_string();
    }
    if(!n5->is_string()) {
	errs << "Parameter 5 (JOI maneuver file name) in create_geom_subsys is not a string.\n";
	return apgen::RETURN_STATUS::FAIL;
    } else {
	spkez_intfc::theJOIFileName() = n5->get_string();
    }
    if(!n4->is_string()) {
	errs << "Parameter 4 (TCM maneuver file name) in create_geom_subsys is not a string.\n";
	return apgen::RETURN_STATUS::FAIL;
    } else {
	spkez_intfc::theTCMFileName() = n4->get_string();
    }
    if(!n3->is_numeric()) {
	errs << "Parameter 3 (relative error) in create_geom_subsys is not a number.\n";
	return apgen::RETURN_STATUS::FAIL;
    } else {
	spkez_intfc::theRelError = n3->get_double();
    }
    if(!n2->is_time()) {
	errs << "Parameter 2 (plan end) in create_geom_subsys is not a time.\n";
	return apgen::RETURN_STATUS::FAIL;
    } else {
	spkez_intfc::PlanEnd = n2->get_time_or_duration();
    }
    if(!n2->is_time()) {
	errs << "Parameter 1 (plan start) in create_geom_subsys is not a time.\n";
	return apgen::RETURN_STATUS::FAIL;
    } else {
	spkez_intfc::PlanStart = n1->get_time_or_duration();
    }

    //
    // We now go over all the resources and resource arrays that
    // make up the Geometry Subsystem. For each one, we will define
    // an entry in the output AAF file.
    //
    // Reminder: interpolation formula
    //
    // Let S be a state and V its derivative over an interval [t1, t2]
    //
    // delta_t = t2 - t1
    //
    // The average velocity should reproduce the difference between
    // the initial and final values of the state:
    //
    // V_av = (S2 - S1) / delta_t
    //
    // x = 6 * (V_av - 0.5 * (V1 + V2)) / delta_t ** 2
    // y = (V2 - V1) / delta_t + x * delta_t
    //
    // Interpolated values: V(t) is a quadratic polynomial and S(t)
    // is the integral of V(t)
    //
    // V(t) = V1 + y * (t - t1) - x * (t - t1) ** 2
    // S(t) = S1 + V1 * (t - t1) + 0.5 * y * (t - t1) ** 2 - x * (t - t1) ** 3 / 3.0
    //
    // We can check that the integral of V(t) divided by delta_t is V_av
    // as it should be:
    //
    // V_av_computed = V1 + 0.5 * y * delta_t - (1/3) * x * delta_t ** 2
    //      = V1 + 0.5 * delta_t * ((V2 - V1) / delta_t + x * delta_t)
    //      	 - (1/3) * x * delta_t ** 2
    //      = V1 + 0.5 * (V2 - V1) + (1/6) * x * delta_t ** 2
    //      = 0.5 * (V2 + V1) + (V_av - 0.5 * (V1 + V2))
    //      = V_av, QED
    //
    // The acceleration is the derivative of V(t):
    //
    // A(t) = y - 2 * x * (t - t1)
    //

    //
    // The first arg of the pair is the maneuver time; the
    // second arg, if it is true, indicates a "finite maneuver",
    // in which case the next item is the end time of the
    // maneuver.  If it is false, we are dealing with a
    // "deterministic" (zero duration) maneuver, so there
    // is only one time associated with it.
    //
    tlist<alpha_time, maneuver_node> maneuver_times(true);

    try {
	spkez_intfc::compute_SC_state(maneuver_times);
	spkez_intfc::compute_BodyRanges(maneuver_times);
    } catch(eval_error Err) {
	errs = Err.msg;
	return apgen::RETURN_STATUS::FAIL;
    }

    *result = 0L;
    return apgen::RETURN_STATUS::SUCCESS;
}

void spkez_intfc::compute_BodyRanges(const tlist<alpha_time, maneuver_node>& maneuver_times) {

    //
    // The first step is to figure out from the adaptation
    // which SPKEZ arguments are needed. The code is found
    // in abstract resource Geometry_Control, which loops
    // over all bodies in global array Bodies.
    //
    // The first task is to design a convenient way to access
    // global variables. globalData::get_symbol(const Cstring&)
    // returns a TypedValue, which sounds great for that
    // purpose.
    //
    // Here is the call in Geometry_Control that gets the values
    // from which the Body Ranges are derived:
    //
    // State=spkez_api(NAIFIDs[Bodies[ibodies]], now, "J2000",ABCORR, -sc_id);
    //
    // So, we need to get the following globals:
    // 	Bodies
    // 	NAIFIDs
    // 	ABCORR
    // 	sc_id
    //
    TypedValue	Bodies, NAIFIDs, ABCORR, the_sc_id; // , otm_times;
    Cstring	item_of_interest;
    try {
	item_of_interest = "Bodies";
	Bodies = globalData::get_symbol(item_of_interest);
	item_of_interest = "NAIFIDs";
	NAIFIDs = globalData::get_symbol(item_of_interest);
	item_of_interest = "ABCORR";
	ABCORR = globalData::get_symbol(item_of_interest);
	item_of_interest = "sc_id";
	the_sc_id = globalData::get_symbol(item_of_interest);
	// item_of_interest = "OTM_Times";
	// otm_times = globalData::get_symbol(item_of_interest);
    } catch(eval_error Err) {
	Cstring error;
	error << "Error while pre-computing geometric resources: global "
	      << item_of_interest << " not found\n";
	throw(eval_error(error));
    }

    initialize_body_related_globals();

    // vector<CTime_base> maneuver_times;
    // ListOVal&		maneuvers = otm_times.get_array();
    // for(int k = 0; k < maneuvers.get_length(); k++) {
    // 	maneuver_times.push_back(maneuvers[k]->Val().get_time_or_duration());
    // }

    //
    // debug
    //
    cerr << "compute_BodyRanges: start iterating\n";

    //
    // Next, we mimic the way the adaptation computes the
    // range resources. We iterate over the Bodies list, and
    // set up the SPKEZ arguments using the appropriate global
    // arrays.
    //
    // Once we know the SPKEZ arguments, we iterate over
    // sampling times which are computed adaptively so
    // as to guarantee that the interpolation error will
    // be acceptable.
    //
    // Once we have the SPKEZ states for all the sampling
    // times, we again mimic the adaptation and compute
    // the actual geometry resources from the SPKEZ state.
    // We then output the values of the geometry resource
    // in an AAF file containing the full specification of
    // precomputed resources.
    //
    long int	Nbodies = Bodies.get_array().get_length();
    aoString	range_AAF;
    aoString	speed_AAF;
    FILE*	pos_file = fopen(*theAAFFileName(), "w");
    Cstring	header;

    header << "apgen version \"automatically precomputed resources\"\n\n";
    fwrite(*header, 1, header.length(), pos_file);
    range_AAF << "resource SpacecraftBodyRange(Bodies): settable float\n    begin\n"
	      << "\tattributes\n\t    \"Subsystem\" = \"Geometry\";\n"
	      << "\t    \"Interpolation\" = true;\n"
	      << "\t    \"Units\" = \"Kilometers\";\n"
	      << "\ttime_series\n";
    speed_AAF << "resource SpacecraftBodySpeed(Bodies): settable float\n    begin\n"
	      << "\tattributes\n\t    \"Subsystem\" = \"Geometry\";\n"
	      << "\t    \"Interpolation\" = true;\n"
	      << "\t    \"Units\" = \"Kilometers/Second\";\n"
	      << "\ttime_series\n";

    for(long int i = 0; i < Nbodies; i++) {
	Cstring		body = Bodies.get_array()[i]->Val().get_string();
	ArrayElement*	ae = NAIFIDs.get_array().find(body);
	long int	naifid = ae->Val().get_int();

	//
	// debug
	//
	cerr << "SPKEZ args: " << body << ", " << spkez_intfc::PlanStart.to_string()
	     << ", " << "J2000" << ", " << ABCORR.get_string() << ", " << (-the_sc_id.get_int())
	     << "\n";

	range_AAF << "    \"" << body << "\" = [\n";
	speed_AAF << "    \"" << body << "\" = [\n";

	data_index indx(pair<long int, Cstring>(naifid, "J2000"),
			pair<Cstring, long int>(ABCORR.get_string(), -the_sc_id.get_int()));

	//
	// We now have to generate a complete series of sampling points for the resources
	// that depend on this particular combination of SPKEZ parameters.
	//
	// tlist<alpha_time, maneuver_node> maneuver_times(true);
	cached_spkez_data& spkezData = precompute_spkez_data(indx, maneuver_times, false);

	//
	// OK, we have the state vector for each sampling time.
	// Now we compute the actual geometry resources. We need
	// to set the values of SpacecraftBodyRange and
	// SpacecraftBodySpeed.
	//
	for(int s = 0; s < (spkezData.sampled_values.size() - 1); s++) {
	    cached_spkez_data::spice_state*	spst = &spkezData.sampled_values[s];
	    cached_spkez_data::spice_state*	nxtspst = &spkezData.sampled_values[s + 1];
	    CTime_base&		sample_time = spst->first;
	    CTime_base&		next_sample_time = nxtspst->first;
	    CTime_base		delta_t = next_sample_time - sample_time;
	    vector<double>	state = spst->second;
	    vector<double>	next_state = nxtspst->second;
	    double		r, dr_dt, v, dv_dt;

	    //
	    // We need to compute the magnitude of the position
	    // vector and its derivative, and the magnitude of
	    // the velocity vector and its derivative.  The
	    // derivatives are needed so that cubic interpolation
	    // can be used to compute the geometric resources.
	    //
	    // r = sqrt(x*x + y*y + z*z)
	    // dr/dt = (x.v_z + y.v_y + z.v_z) / r 
	    // v = sqrt(v_x*v_x + v_y*v_y + v_z*v_z)
	    // dv/dt = (v_x.a_x + v_y.a_y + v_z.a_z) / v
	    // 
	    // x,y,z,v_x,v_y,v_z are known from the spkez data.
	    // a_x,a_y,a_z are not. But from the interpolation
	    // formula
	    // V_av = (S2 - S1) / delta_t
	    // X = 6 * (V_av - 0.5 * (V1 + V2)) / delta_t ** 2
	    // Y = (V2 - V1) / delta_t + X * delta_t
	    // A(t) = Y - 2 * X * (t - t1)
	    //
	    r = sqrt(state[0] * state[0] + state[1] * state[1] + state[2] * state[2]);
	    dr_dt = (state[0] * state[3] + state[1] * state[4] + state[2] * state[5]) / r;
	    range_AAF << "\t\"" << sample_time.to_string() << "\" = ["
			   << Cstring(r, MAX_SIGNIFICANT_DIGITS) << ", "
			   << Cstring(dr_dt, MAX_SIGNIFICANT_DIGITS) << "],\n";
	    if(s == spkezData.sampled_values.size() - 2) {
	        r = sqrt(next_state[0] * next_state[0] + next_state[1] *
			next_state[1] + next_state[2] * next_state[2]);
	        dr_dt = (next_state[0] * next_state[3] + next_state[1] *
			next_state[4] + next_state[2] * next_state[5]) / r;
	        range_AAF << "\t\"" << next_sample_time.to_string() << "\" = ["
			   << Cstring(r, MAX_SIGNIFICANT_DIGITS) << ", "
			   << Cstring(dr_dt, MAX_SIGNIFICANT_DIGITS) << "]];\n";
	    }

	    double delta_t_f = delta_t.convert_to_double_use_with_caution();
	    double v_av[3];
	    double X[3];
	    double Y[3];
	    double a[3];
	    double next_a[3];
	    double delta_t_sq = delta_t_f * delta_t_f; 
	    for(int icoord = 0; icoord < 3; icoord++) {
		v_av[icoord] = (next_state[icoord] - state[icoord]) / delta_t_f;
		X[icoord] = 6.0 * (v_av[icoord] - 0.5 * (state[icoord + 3] + next_state[icoord + 3])) / delta_t_sq;
		Y[icoord] = (next_state[icoord + 3] - state[icoord + 3]) / delta_t_f + X[icoord] * delta_t_f;
		a[icoord] = Y[icoord];
	    }

	    v = sqrt(state[3] * state[3] + state[4] * state[4] + state[5] * state[5]);
	    dv_dt = (state[3] * a[0] + state[4] * a[1] + state[5] * a[2]) / v;

	    speed_AAF << "\t\"" << sample_time.to_string() << "\" = ["
			   << Cstring(v, MAX_SIGNIFICANT_DIGITS) << ", "
			   << Cstring(dv_dt, MAX_SIGNIFICANT_DIGITS) << "],\n";
	    if(s == spkezData.sampled_values.size() - 2) {
		for(int icoord = 0; icoord < 3; icoord++) {
		    next_a[icoord] = Y[icoord] - 2.0 * X[icoord] * delta_t_f;
		}
	        v = sqrt(next_state[3] * next_state[3] + next_state[4] *
			next_state[4] + next_state[5] * next_state[5]);
	        dv_dt = (next_state[3] * next_a[0] + next_state[4] *
			next_a[1] + next_state[5] * next_a[2]) / v;
	        speed_AAF << "\t\"" << next_sample_time.to_string() << "\" = ["
			   << Cstring(v, MAX_SIGNIFICANT_DIGITS) << ", "
			   << Cstring(dv_dt, MAX_SIGNIFICANT_DIGITS) << "]];\n";
	    }
	}
    }
    range_AAF << "\tdefault\n\t    0.0;\n"
	      << "\tusage\n\t    evaluate();\n"
	      << "\n    end resource SpacecraftBodyRange\n\n";
    speed_AAF << "\tdefault\n\t    0.0;\n"
	      << "\tusage\n\t    evaluate();\n"
	      << "\n    end resource SpacecraftBodySpeed\n";

    Cstring position_stuff = range_AAF.str();
    Cstring velocity_stuff = speed_AAF.str();
    fwrite(*position_stuff, 1, position_stuff.length(), pos_file);
    fwrite(*velocity_stuff, 1, velocity_stuff.length(), pos_file);

    fclose(pos_file);
}

//
// A function that evaluates the 6-dim. S/C state from spkez and
// sticks it into 3 pre-computed position resources. This function
// is not normally needed when running APGenX, but it is useful for
// validating trajectory and maneuver data.
//
// SPKEZ args for the first call:
//
// 	body = 10 (Sun Barycenter), time = spkez_intfc::PlanStart,
//	frame = J2000, ABCORR = NONE, S/C ID = -159
//
// The time of subsequent calls is determined by the maximum
// requirement on the interpolation error.
//
void spkez_intfc::compute_SC_state(tlist<alpha_time, maneuver_node>& maneuver_times) {

    initialize_body_related_globals();

    //
    // The first arg of the pair is the maneuver time; the
    // second arg, if it is true, indicates a "finite maneuver",
    // in which case the next item is the end time of the
    // maneuver.  If it is false, we are dealing with a
    // "deterministic" (zero duration) maneuver, so there
    // is only one time associated with it.
    //
    // tlist<alpha_time, maneuver_node> maneuver_times(true);

    //
    // We must read information from Nav's maneuver
    // information files:
    //
    // 	- TCMs	(...DETERMINISTIC_MANEUVERS.csv)
    // 	- JOI	(...FINITE_MANEUVERS.csv)
    //
    FILE*	tcm_file = fopen(*theTCMFileName(), "r");
    FILE*	joi_file = fopen(*theJOIFileName(), "r");
    if(!tcm_file) {
	Cstring err = "File ";
	err << theTCMFileName() << " does not exist\n";
	throw(eval_error(err));
    }
    if(!joi_file) {
	Cstring err = "File ";
	err << theJOIFileName() << " does not exist\n";
	throw(eval_error(err));
    }

    get_maneuver_times(maneuver_times, tcm_file);

    //
    // debug
    //
    cerr << maneuver_times.get_length() << " TCM maneuver(s) detected...\n";

    get_maneuver_times(maneuver_times, joi_file);

    //
    // debug
    //
    cerr << maneuver_times.get_length() << " total maneuver(s) detected.\n";

    // ListOVal&		maneuvers = otm_times.get_array();
    // for(int k = 0; k < maneuvers.get_length(); k++) {
    // 	maneuver_times.push_back(maneuvers[k]->Val().get_time_or_duration());
    // }

    //
    // debug
    //
    cerr << "compute_SC_state: start iterating\n";

    aoString	state_AAF, stateX, stateY, stateZ;

    //
    // Hard-code the SC state AAF file, at least for now (this
    // resource is not needed in production runs)
    //
    FILE*	aaf_file = fopen("SC_state.aaf", "w");
    Cstring	header;

    header << "apgen version \"automatically precomputed resources\"\n\n";
    fwrite(*header, 1, header.length(), aaf_file);
    state_AAF << "resource SC_State[\"X\", \"Y\", \"Z\"]: settable float\n    begin\n"
	      << "\tattributes\n\t    \"Subsystem\" = \"Geometry\";\n"
	      << "\t    \"Interpolation\" = true;\n"
	      << "\t    \"Units\" = \"Kilometers\";\n"
	      << "\ttime_series\n";

    stateX << "    \"X\" = [\n";
    stateY << "    \"Y\" = [\n";
    stateZ << "    \"Z\" = [\n";

    data_index indx(pair<long int, Cstring>(10, "J2000"), pair<Cstring, long int>("NONE", -159));

    //
    // We now have to generate a complete series of sampling points for the resources
    // that depend on this particular combination of SPKEZ parameters.
    //
    cached_spkez_data& spkezData = precompute_spkez_data(indx, maneuver_times, true);

    //
    // OK, we have the state vector for each sampling time.
    // Now we compute the actual geometry resources. We need
    // to set the values of SpacecraftBodyRange and
    // SpacecraftBodySpeed.
    //
    for(int s = 0; s < (spkezData.sampled_values.size() - 1); s++) {
	cached_spkez_data::spice_state*	spst = &spkezData.sampled_values[s];
	cached_spkez_data::spice_state*	nxtspst = &spkezData.sampled_values[s + 1];
	CTime_base&		sample_time = spst->first;
	CTime_base&		next_sample_time = nxtspst->first;
	CTime_base		delta_t = next_sample_time - sample_time;
	vector<double>&		state = spst->second;
	vector<double>&		next_state = nxtspst->second;

	stateX << "\t\"" << sample_time.to_string() << "\" = ["
			   << Cstring(state[0], MAX_SIGNIFICANT_DIGITS) << ", "
			   << Cstring(state[3], MAX_SIGNIFICANT_DIGITS) << "],\n";
	stateY << "\t\"" << sample_time.to_string() << "\" = ["
			   << Cstring(state[1], MAX_SIGNIFICANT_DIGITS) << ", "
			   << Cstring(state[4], MAX_SIGNIFICANT_DIGITS) << "],\n";
	stateZ << "\t\"" << sample_time.to_string() << "\" = ["
			   << Cstring(state[2], MAX_SIGNIFICANT_DIGITS) << ", "
			   << Cstring(state[5], MAX_SIGNIFICANT_DIGITS) << "],\n";

	if(s == spkezData.sampled_values.size() - 2) {

	        stateX << "\t\"" << next_sample_time.to_string() << "\" = ["
			   << Cstring(next_state[0], MAX_SIGNIFICANT_DIGITS) << ", "
			   << Cstring(next_state[3], MAX_SIGNIFICANT_DIGITS) << "]];\n";
	        stateY << "\t\"" << next_sample_time.to_string() << "\" = ["
			   << Cstring(next_state[1], MAX_SIGNIFICANT_DIGITS) << ", "
			   << Cstring(next_state[4], MAX_SIGNIFICANT_DIGITS) << "]];\n";
	        stateZ << "\t\"" << next_sample_time.to_string() << "\" = ["
			   << Cstring(next_state[2], MAX_SIGNIFICANT_DIGITS) << ", "
			   << Cstring(next_state[5], MAX_SIGNIFICANT_DIGITS) << "]];\n";
	}
    }

    Cstring stuff = stateX.str();
    state_AAF << stuff;
    stuff = stateY.str();
    state_AAF << stuff;
    stuff = stateZ.str();
    state_AAF << stuff;
    state_AAF << "\tdefault\n\t    0.0;\n"
	      << "\tusage\n\t    evaluate();\n"
	      << "\n    end resource SC_State\n\n";

    stuff = state_AAF.str();
    fwrite(*stuff, 1, stuff.length(), aaf_file);
    fclose(aaf_file);

    if(tcm_file) {
	fclose(tcm_file);
    }
    if(joi_file) {
	fclose(joi_file);
    }

    // PFM debug
    cerr << "Finished generating S/C state resources.\n"; // "exiting\n";
    // exit(0);
}

void	spkez_intfc::get_maneuver_times(
				tlist<alpha_time, maneuver_node>& maneuvers,
				FILE* maneuver_file) {
    int c;
    int line = 0;
    stringstream s;
    vector<string> linevec;
    bool omit_blanks = true;
    stringstream blanks;
    bool this_is_a_maneuver_file = false;
    bool this_is_a_joi_file = false;
    while((c = getc(maneuver_file)) != EOF) {
	if(c == ' ') {
	    if(!omit_blanks) {
		blanks << ' ';
	    }
	} else if(c == '\n') {
	    if(!linevec.size()) {
		return;
	    }
	    blanks.str("");
	    linevec.push_back(s.str());
	    s.str("");

	    //
	    // PFM debug
	    //
	    cerr << "line " << (line + 1) << ": ";
	    for(int k = 0; k < linevec.size(); k++) {
		cerr << linevec[k];
		if(k < linevec.size() - 1) {
		    cerr << ", ";
		} else {
		    cerr << "\n";
		}
	    }

	    //
	    // Identify the file based on the header items
	    //
	    if(!line) {
		if(linevec[2] == "Maneuver Name") {
		    this_is_a_maneuver_file = true;
		} else if(linevec[0] == "Burn Name") {
		    this_is_a_joi_file = true;
		} else {
		    throw(eval_error("reading a file that is neither a maneuver file nor a joi file\n"));
		}
	    } else {

		if(this_is_a_maneuver_file) {

		    //
		    // Don't grab the first item, which is Julian time expressed
		    // as a float. Use the second field, which is a string:
		    //
		    // linevec[0].c_str() is the string holding the time value
		    double jd_time;
		    double et_time;

		    //
		    // This works pretty well but is not exact:
		    //
		    // sscanf(linevec[0].c_str(), "%lf", &jd_time);
		    // string jd = string("jd ") + linevec[0];
		    // utc2et_c ( jd.c_str(), &et_time);

		    str2et_c(linevec[1].c_str(), &et_time);
		    CTime_base scet_time = CTime_base(ET2UTCS(et_time));
		    cerr << "    SCET time(" << linevec[2] << "): " << scet_time.to_string() << "\n";

		    //
		    // The second arg is false unless we have an extended maneuver
		    //
		    Cstring	maneuver_name(linevec[2].c_str());
		    maneuvers << new maneuver_node(scet_time, pair<Cstring, bool>(maneuver_name, false));
		} else if(this_is_a_joi_file) {

		    //
		    // Grab the second and third items, which are julian date strings
		    //
		    Cstring	maneuver_name(linevec[0].c_str());
		    double et_start, et_end;
		    str2et_c(linevec[1].c_str(), &et_start);
		    str2et_c(linevec[2].c_str(), &et_end);
		    Cstring scet_start = ET2UTCS(et_start);
		    Cstring scet_end = ET2UTCS(et_end);
		    CTime_base start_time = CTime_base(scet_start);
		    CTime_base end_time = CTime_base(scet_end);
		    cerr << "    " << maneuver_name << " JOI times(" << linevec[0] << "): "
				<< scet_start << ", " << scet_end << "\n";
		    maneuvers << new maneuver_node(start_time, pair<Cstring, bool>(maneuver_name, true));
		    maneuvers << new maneuver_node(end_time, pair<Cstring, bool>(maneuver_name, true));
		}
	    }
	    linevec.clear();
	    line++;
	} else if(c == ',') {

	    //
	    // separator
	    //
	    linevec.push_back(s.str());
	    s.str("");
	    blanks.str("");
	    omit_blanks = true;
	} else {
	    if(!omit_blanks) {
		s << blanks.str();
		blanks.str("");
	    }
	    s << (char)c;
	    omit_blanks = false;
	}
    }
}

void	spkez_intfc::initialize_body_related_globals() {
    static bool already_initialized = false;

    if(already_initialized) {
	return;
    }
    already_initialized = true;

    //
    // 1. initialize init_bodies_of_interest()
    //
    spkez_intfc::init_bodies_of_interest().clear();

    //
    // Note: S/C ID = -159
    //
    spkez_intfc::body_state* light = new spkez_intfc::body_state(-159);

    spkez_intfc::init_bodies_of_interest() << light;

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
    // Note:	see pck.html for documentation on planet-related
    //		constants.
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
    spkez_intfc::init_bodies_of_interest() << light;
	// Sun
	light->payload << new spkez_intfc::influencer(0L, 10);

    // Earth
    light = new spkez_intfc::body_state(399);
    spkez_intfc::init_bodies_of_interest() << light;
	// Earth Bary
	light->payload << new spkez_intfc::influencer(0L, 3);

    // Earth Bary
    light = new spkez_intfc::body_state(3);
    spkez_intfc::init_bodies_of_interest() << light;
	// Sun
	light->payload << new spkez_intfc::influencer(0L, 10);

    // Moon
    light = new spkez_intfc::body_state(301);
    spkez_intfc::init_bodies_of_interest() << light;
	// Earth Bary
	light->payload << new spkez_intfc::influencer(0L, 3);

    // Mars Bary
    light = new spkez_intfc::body_state(4);
    spkez_intfc::init_bodies_of_interest() << light;
	// Sun
	light->payload << new spkez_intfc::influencer(0L, 10);

    // Jupiter Bary
    light = new spkez_intfc::body_state(5);
    spkez_intfc::init_bodies_of_interest() << light;
	// Sun
	light->payload << new spkez_intfc::influencer(0L, 10);

    // Io
    light = new spkez_intfc::body_state(501);
    spkez_intfc::init_bodies_of_interest() << light;
	// Jupiter Bary
	light->payload << new spkez_intfc::influencer(0L, 5);

    // Europa
    light = new spkez_intfc::body_state(502);
    spkez_intfc::init_bodies_of_interest() << light;
	// Jupiter Bary
	light->payload << new spkez_intfc::influencer(0L, 5);

    // Ganymede
    light = new spkez_intfc::body_state(503);
    spkez_intfc::init_bodies_of_interest() << light;
	// Jupiter Bary
	light->payload << new spkez_intfc::influencer(0L, 5);

    // Callisto
    light = new spkez_intfc::body_state(504);
    spkez_intfc::init_bodies_of_interest() << light;
	// Jupiter Bary
	light->payload << new spkez_intfc::influencer(0L, 5);

    // Saturn Bary
    light = new spkez_intfc::body_state(6);
    spkez_intfc::init_bodies_of_interest() << light;

    // Sun
    light->payload << new spkez_intfc::influencer(0L, 10);
    assert(light->payload.infl_in_incr_abs_err_order.get_length() == 11
	   || light->payload.infl_in_incr_abs_err_order.get_length() == 1);
}

spkez_intfc::cached_spkez_data&
spkez_intfc::precompute_spkez_data(
			const data_index&			one_value,
			const tlist<alpha_time, maneuver_node>&	maneuver_times,
			/* debug = */ bool debug_flag) {

    //
    // First, see if we have already processed this
    // combination of spkez arguments
    //
    data_map::const_iterator	map_iter = cached_arg_map().find(one_value);

    if(map_iter != cached_arg_map().end()) {

	//
	// Yes, we already have it.  Nothing to do.
	//

	//
	// debug
	//
	cached_spkez_data& spkezData = cached_data_vector()[map_iter->second];

	cerr << "precompute_spkez_data(): target = " << spkezData.target
	     << ", frame = " << spkezData.ref_frame
	     << ", abcorr = " << spkezData.abcorr
	     << ", observer = " << spkezData.observer << "\n";

	cerr << " " << spkezData.sampled_values.size()
	     << " sampled states already added.\n";
	return cached_data_vector()[map_iter->second];
    }

    //
    // Since this is a new set of arguments, we define a node
    // in which we can store the data we are about to compute
    //
    long int hash_index = cached_data_vector().size();
    cached_arg_map()[one_value] = hash_index;
    cached_data_vector().push_back(cached_spkez_data(one_value, hash_index));
    cached_spkez_data& spkezData = cached_data_vector()[hash_index];

    spkezData.determine_geometry();

    //
    // IDs of the two bodies
    //
    spkez_intfc::body_state* relative = NULL;
    spkez_intfc::body_state* light = init_bodies_of_interest().find(spkezData.light_body);
    relative = init_bodies_of_interest().find(spkezData.relative_body);

    light->payload.reset();
    if(relative) {
	relative->payload.reset();
    }

    //
    // Get the frame information. For range resources, this is
    // not important, but other resources may care.
    //
    long int center_ID, frame_class, class_frame_ID;
    Cstring frame_name = one_value.first.second;
    if(!FrameInfo(frame_name, center_ID, frame_class, class_frame_ID)) {
	throw(eval_error("frame info error"));
    }
    CTime_base MidPoint = PlanStart + (PlanEnd - PlanStart) * 0.5;
    double middouble = MidPoint.convert_to_double_use_with_caution();
    double angular_vel[3];
    FrameAngularVelocity("J2000", *frame_name, middouble, angular_vel);
    double omega = angular_vel[0] * angular_vel[0]
		       + angular_vel[1] * angular_vel[1]
		       + angular_vel[2] * angular_vel[2];
    omega = sqrt(omega);
    double	period_in_hours = -1.0;
    if(omega > 1.0E-15) {
	    period_in_hours = (2 * M_PI / omega) / 3600.0;
    }

    //
    // debug
    //
    cerr << frame_name << " = "
	 << "[\"center\" = " << center_ID << ", "
	 << "\"class\" = " << frame_class << ", "
	 << "\"class frame\" = " << class_frame_ID << ", "
	 << "\"period (hrs)\" = ";
    if(period_in_hours < 0.0) {
	cerr << "\"infinity\"]\n";
    } else {
	cerr << period_in_hours << "]\n";
    }

    //
    // 10 is the Sun Barycenter, which is the origin of the system
    //
    assert(relative || spkezData.relative_body == 10);


    //
    // Now, compute sampling points iteratively, adjusting the
    // sampling interval to make sure the interpolation error
    // will be within bounds.
    //
    // Store the computed data in spkezData.sampled_values.
    //
    CTime_base				newtime = PlanStart;
    CTime_base				currenttime = newtime;
    cached_spkez_data::spice_state	vec;
    bool				first_time = true;

    //
    // We need to keep track of maneuvers. Let us find the
    // first maneuver after PlanStart.
    //
    maneuver_node* one_maneuver;
    for(	one_maneuver = maneuver_times.first_node();
		one_maneuver;
		one_maneuver = one_maneuver->next_node()) {

	//
	// debug
	//
	cerr << "  checking maneuver " << one_maneuver->payload.first
		<< ": etime = " << one_maneuver->Key.getetime().to_string()
		<< ", Plan Start = " << PlanStart.to_string() << "\n";

	if(one_maneuver->Key.getetime() > PlanStart) {
	    break;
	}
    }

    bool	there_is_a_maneuver = one_maneuver != NULL;
    CTime_base	next_maneuver_time;
    CTime_base	one_minute("1:00");

    //
    // PFM debug
    //
    if(there_is_a_maneuver) {
	next_maneuver_time = one_maneuver->Key.getetime();
	cerr << "    maneuver " << one_maneuver->payload.first << " time = "
	     << one_maneuver->Key.getetime().to_string() << "\n";
    }

    while(currenttime < PlanEnd) {
	double state[6];

	//
	// We want to generate a history for this particular
	// combination of spkez parameters.
	// 

	//
	// Special case: first time we call propagate()
	//
	if(first_time) {
	    first_time = false;

	    //
	    // The first time around, newtime is not changed.
	    //
	    spkezData.propagate(light, relative,
			currenttime, PlanEnd, period_in_hours, state, true);
	    vec.first = newtime;
	    vec.second.clear();
	    for(int hh = 0; hh < 6; hh++) {
		vec.second.push_back(state[hh]);
	    }
	    spkezData.sampled_values.push_back(vec);
	}

	//
	// Save currenttime since propagate() increases it
	//
	CTime_base oldtime = currenttime;

	//
	// Now call propagate in the 'generic' (i. e., not first)
	// time
	//
	newtime = spkezData.propagate(light, relative,
			currenttime, PlanEnd, period_in_hours, state);

	bool skip_newtime = false;

	//
	// Deal with maneuvers
	//
	if(there_is_a_maneuver) {
	    assert(next_maneuver_time > oldtime);

	    if(newtime >= next_maneuver_time) {

		//
		// if newtime is within two minutes of the next maneuver time,
		// we don't bother inserting a node at newtime
		//
		skip_newtime = (newtime - next_maneuver_time <= 2 * one_minute);

	        //
	        // PFM debug
	        //
		cerr << "    maneuver " << one_maneuver->payload.first
		     << "; adding two nodes at time = "
		     << next_maneuver_time.to_string() << "\n";

		//
		// We define a 2-minute interval around the maneuver time
		// and include the endpoints of that interval in the set
		// of sampling times. This minimizes the impact of non-
		// inertial acceleration on the interpolation accuracy.
		//
		CTime_base maneuver_start = next_maneuver_time - one_minute;
		CTime_base maneuver_end   = next_maneuver_time + one_minute;
		double maneuver_state[6];
		double nowval = maneuver_start.convert_to_double_use_with_caution();
		double lt;
		spkez_api(
			spkezData.target,
			nowval,
			*spkezData.ref_frame,
			*spkezData.abcorr,
			spkezData.observer,
			maneuver_state,
			&lt);
		vec.first = maneuver_start;
		vec.second.clear();
		for(int hh = 0; hh < 6; hh++) {
		    vec.second.push_back(maneuver_state[hh]);
		}	    
		spkezData.sampled_values.push_back(vec);
		nowval = maneuver_end.convert_to_double_use_with_caution();
		spkez_api(
			spkezData.target,
			nowval,
			*spkezData.ref_frame,
			*spkezData.abcorr,
			spkezData.observer,
			maneuver_state,
			&lt);
		vec.first = maneuver_end;
		vec.second.clear();
		for(int hh = 0; hh < 6; hh++) {
		    vec.second.push_back(maneuver_state[hh]);
		}	    
		spkezData.sampled_values.push_back(vec);

		currenttime = maneuver_end;

#ifdef DEBUG_ONLY
		double deltav = spkezData.get_delta_v_at(next_maneuver_time);

		// PFM debug
		cerr << "    deltav(" << one_maneuver->payload.first << ", "
			<< next_maneuver_time.to_string() << ") = " << deltav << "\n";

#endif /* DEBUG_ONLY */

		while(next_maneuver_time <= newtime) {
		    one_maneuver = one_maneuver->next_node();
		    if(one_maneuver) {
			next_maneuver_time = one_maneuver->Key.getetime();
		    } else {
			there_is_a_maneuver = false;
			break;
		    }
		}

		//
		// If one_maneuver is not NULL, next_maneuver_time is > newtime
		//
	    }
	}

	if(!skip_newtime) {
	    vec.first = newtime;
	    vec.second.clear();
	    for(int hh = 0; hh < 6; hh++) {
		vec.second.push_back(state[hh]);
	    }
	    spkezData.sampled_values.push_back(vec);
	}

	currenttime = newtime;
    }

    //
    // debug
    //
    cerr << " " << spkezData.sampled_values.size() << " sampled states added through propagation.\n";

    //
    // SUCCESS
    //
    return spkezData;
}



