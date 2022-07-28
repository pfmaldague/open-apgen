#ifndef _SPICEiNTFC_H_
#define _SPICEiNTFC_H_

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#ifdef HAVE_VALUES_H
#include <values.h>
#endif

#include <vector>
#include <map>
#include <list>

#include <iostream>
#include "APdata.H"
#include "TypedValue.H"

//
// Note: SpiceUsr.h features the "ifdef __cplusplus" macro, so
// we don't need to define it 'extern "C"'.
//
#include "SpiceUsr.h"

extern Cstring SCET_PRINT(double time);
extern double GM_for_target(long target);
extern const char* name_this_object(long target, bool append_blanks);
extern void spkez_api(int, double, const char*, const char*, int, double*, double*);
extern char* ET2UTCS(double);

extern long int	FrameNameToID(	const Cstring& name);

extern void	StateTransform(
			const Cstring& from_name,
			const Cstring& to_name,
			double timeval,
			double xform[6][6]
			);

extern void	AngularVelocityFromXform(
			double xform[6][6],
			double angular_velocity_vector[3]);

extern void	FrameAngularVelocity(
			const Cstring& from_name,
			const Cstring& to_name,
			double timeval,
			double angular_velocity_vector[3]);

extern bool	FrameInfo(	const Cstring& frame_name,
				long int& center_ID,
				long int& frame_class,
				long int& class_frame_ID);

extern "C" {		// wrapping for C++ routines used by C
    char *SCET_PRINT_char_ptr(double time);

    #include "SpicePln.h"
    #include "SpiceZpr.h"
    void vzfrac ( SpiceInt             frontID, 
                 ConstSpiceChar     * frontFrame,
                 ConstSpiceDouble     frontRadii [3],
                 SpiceInt             backID, 
                 ConstSpiceChar     * backFrame,
                 ConstSpiceDouble     backRadii  [3],
                 SpiceInt             obsID, 
                 ConstSpiceChar     * abcorr,
                 SpiceDouble          et,
                 SpiceDouble        * frac    );

};

//
// First element of the pair is the maneuver name;
// second element is true if this is an extended maneuver,
// false if it is instantaneous.
//
typedef Cntnr<alpha_time, pair<Cstring, bool> > maneuver_node;

//
// Caching interface for spkez_c
//

class spkez_intfc {
public:

	static bool		caching_enabled;
	static bool		subsystem_initialization_run;
	static bool		repeat_calls_only;
	static bool		check_errors;
	static bool		dump_call_parameters;
	static bool		check_precomp_res;
	static FILE*		check_precomp_file;
	static double		theRelError;
	static Cstring&		theAAFFileName();
	static Cstring&		theTCMFileName();
	static Cstring&		theJOIFileName();
	static CTime_base	PlanStart;
	static CTime_base	PlanEnd;

	//
	// system
	//
	static void		compute_BodyRanges(
					const tlist<alpha_time, maneuver_node>&);
	static void		compute_SC_state(
					tlist<alpha_time, maneuver_node>&);
	static void		get_maneuver_times(
					tlist<alpha_time, maneuver_node>&,
					FILE*);

	//
	// statistics
	//
	static int		total_count;
	static int		spkez_count;
	static int		interpolated_count;
	static vector<double>&	max_error();
	static vector<CTime_base>& time_at_max_error();

	static apgen::RETURN_STATUS call_spkez_if_necessary(
		Cstring&	errs,
		TypedValue*	result,
		slst<TypedValue*>& arguments);

	static apgen::RETURN_STATUS spice_spkez_ap(
		Cstring&	errs,
		TypedValue*	result,
		slst<TypedValue*>& arguments);

	static double		estimate_error(
					double act[],
					double pred[]);
	static double		estimate_error(
					const TypedValue& act,
					const TypedValue& pred);
	static void		document_error(
					const TypedValue& act,
					const TypedValue& pred,
					double out[]);

	static void		dump_parameters(const Cstring& filename);

	static void		last_call();

	//
	// The 4 call arguments to spkez (excluding time):
	// target, ref frame, abcorr, observer.
	//
	typedef pair<	
		    pair<long int, Cstring>,
		    pair<Cstring, long int>
		> data_index;

	//
	// Writes an AAF files defining precomputed resources
	// that represent SPKEZ results for all known combinations
	// of input parameters. Throws eval_error if failure.
	//
	static void		generate_precomputed_spkez_AAF();

	static void		initialize_body_related_globals();

	static void		clear_cache();
	static void		clean_up_cache();

	typedef map<data_index, long int> data_map;

	//
	// Cache for 4 arguments (target, ref frame, abcorr, observer).
	//
	static data_map&		spkez_arg_map();
	static map<Cstring, double>&	frame_info();
	static vector<data_index>&	spkez_args();
	static vector<data_index>&	master_list_of_spkez_arguments();


	static Cstring&			AAFname();

	//
	// Body-specific information. We use two keys: the
	// primary key is the light body, the secondary key
	// is the distance to the influencer (aka heavy body).
	//
	class infl_data {
	public:
	    infl_data() = default;
	    infl_data(int id) : bodyID(id),
			R(0.0),
			V(0.0),
			previous_time(deluge()) {
		for(int old = 0; old < 6; old++) {
		    previous_state[0] = 0.0;
		}
	    }
	    infl_data(const infl_data&) = default;
	    infl_data& operator=(const infl_data&) = default;
	    ~infl_data() = default;

	    //
	    // the ID of the influencer
	    //
	    int bodyID;

	    //
	    // the distance of the influencer to the body
	    // it influences (km)
	    //
	    double R;

	    //
	    // the velocity of the influencer relative to
	    // the body it influences (km / s)
	    //
	    double V;

    private:
	    //
	    // the time at which the key was evaluated
	    //
	    CTime_base	previous_time;
	    double	previous_state[6];
    public:
	    const CTime_base& get_previous_time() {
		return previous_time;
	    }

	    //
	    // Absolute error in km per sec to the 4th power
	    // is evaluated as the natural log of the physical
	    // quantity times 10 to the 18th power (a billion
	    // squared), cast as an integer.  We need integers
	    // so we can use this quantity as a key.
	    //
	    long int	compute_abs_err_sec_4th(
				CTime_base	currtime,
				long int	this_body,
				long int	target_body,
				double		state[6],
				double		GM,
				double&		err_vel,
				double&		err_accel);

	    void	reset() {
		previous_time = deluge();
	    }

	    static CTime_base&	deluge();
	}; // class infl_data

	//
	// NOTE: the key of this node is the estimated sampling time
	//
	typedef Cntnr<alpha_int, infl_data>	influencer;

	//
	// NOTE: the key of this node is a body ID
	//
	typedef Cntnr<alpha_int, influencer*>	ptr2influencer;

	//
	// The influencers class holds data for all the influencers
	// of a given body of interest. For planets and moons, only
	// one influencer is needed: the Sun for a planet; for a
	// moon, the planet that owns it. For the S/C, 11 potential
	// influencers need to be tracked:
	//
	//   Note: 1 AU = 150 million km
	//
	//	Body		ID
	//	====		==
	// 	Sun		10
	// 	Venus		2
	// 	Earth		399	[DSN station	399ddd]
	// 	Moon		301
	// 	Mars		4
	// 	Jupiter		5
	// 	Io		501
	// 	Europa		502
	// 	Ganymede	503
	// 	Callisto	504
	// 	Saturn		6
	//
	// Intuitively speaking, one might want to rank influencers
	// based on distance from the S/C: the smaller the distance
	// to an influencer, the more prevalent the influencer.
	// However, this does not work for planets and the Sun; the
	// Sun is so much more massive that it prevails even at
	// distances much larger than the distance to the nearest
	// planet.
	//
	// A better measure of influencer rank is the sampling
	// frequency associated with it. The higher the frequency,
	// the greater corrections to the interpolation formula.
	// Since this frequency also determines the delta T that
	// should be used in the interpolation formula, it makes
	// total sense to use it as an index as well. 
	//
	// Actually, it is most convenient to rank objects in
	// _decreasing_ order of the key. Therefore, we use the
	// sampling _time_ as a key, instead of  the frequency.
	//
	// Initially, when no data is known about an influencer,
	// the sampling time is set to 0. The first influencer
	// is the one with the shortest sampling time. Therefore,
	// As soon as the first influencer in the list has a
	// sampling time greater than zero, we know we have
	// data for all relevant influencers.
	//
	// In principle, we should keep track of the times at
	// which each influencer's sampling time is evaluated,
	// and we should make sure that these evaluation times
	// are sufficiently close together for the error estimate
	// to be valid. Thankfully, the adaptation is such that
	// data for _all_ S/C influencers are collected at the
	// same time at regular (slowly varying) time intervals.
	// Thus, we can keep track of evaluation times at the
	// level of the influencer list rather than at the level
	// of the individual influencers. This greatly simplifies
	// the logic of data acquisition for S/C influencers.
	//
	class influencers {
	public:
	    influencers() : infl_in_incr_abs_err_order(true) {}
	    influencers(const influencers&) = default;
	    influencers& operator=(const influencers&) = default;
	    ~influencers() = default;

	    //
	    // The fundamental quantity, which is used as a key, is this:
	    //
	    // 	position interpolation error in (km) per (second to the 4th power)
	    //
	    // Such quantities are additive and yield the absolute interpolation
	    // error when multiplied by the 4th power of the sampling time.
	    //
	    // To avoid using floats which are not suitable as keys, use
	    // integers. Since we are dealing with astronomical quantities,
	    // we encode a floating point number of km per sec to the 4th
	    // as
	    //
	    // 	(long int) (<1 billion> * log(<km per sec to the 4th>))
	    //
	    // The influencer with the largest contribution to the absolute
	    // error comes last in the list.
	    //
	    tlist<alpha_int, influencer>	infl_in_incr_abs_err_order;
	    tlist<alpha_int, ptr2influencer>	infl_in_incr_body_id_order;

	    void operator<<(influencer* I) {
		infl_in_incr_abs_err_order << I;
		if(!infl_in_incr_body_id_order.find(I->payload.bodyID)) {
		    infl_in_incr_body_id_order
			<< new ptr2influencer(I->payload.bodyID, I);
		}
	    }
	    influencer*				find_influencer(long int n) {
		return infl_in_incr_abs_err_order.find(n);
	    }
	    ptr2influencer*			find_ptr(long int n) {
		return infl_in_incr_body_id_order.find(n);
	    }
	    influencer*				last_node() {
		return infl_in_incr_abs_err_order.last_node();
	    }
	    void				reset() {
		ptr2influencer*	ptr;
		for(ptr = infl_in_incr_body_id_order.first_node();
		    ptr;
		    ptr = ptr->next_node()) {
		    infl_in_incr_abs_err_order.remove_node(ptr->payload);
		    ptr->payload->Key = 0L;
		    ptr->payload->payload.reset();
		    infl_in_incr_abs_err_order << ptr->payload;
		}
	    }

	    //
	    // Returns the estimated interpolation error in
	    // km per second to the fourth power. The error
	    // is encoded as explained in the comments before
	    // method compute_abs_err_sec_4th() below.
	    //
	    long int err_per_sec_4th() {
		influencer* infl = infl_in_incr_abs_err_order.last_node();
		return infl ? infl->Key.get_int() : 0;
	    }
	};

	//
	// NOTE: key is body ID of the body
	//
	class body_state: public baseC<alpha_int, body_state> {
	public:
	    body_state() = default;
	    body_state(int k) : baseC<alpha_int, body_state>(k) {}
	    body_state(const body_state&) = default;
	    virtual ~body_state() {
		if(list) {
		    list->remove_node(this);
		}
	    }
	    body_state&	operator=(const body_state&) = default;

	    body_state*		copy() override {
		return new body_state(*this);
	    }
	    body_state*		get_this() override {
		return this;
	    }
	    const body_state*	get_this() const override {
		return this;
	    }

	    void		update_influencers(CTime_base T);
	    Cstring		identify();

	    influencers		payload;
	};

	static tlist<alpha_int, body_state>&	bodies_of_interest();
	static tlist<alpha_int, body_state>&	init_bodies_of_interest();

	class call_data {
	public:
	    call_data() = default;
	    call_data(const data_index& di, int h)
		: args(di),
			hashval(h),
			target(di.first.first),
			ref_frame(di.first.second),
			abcorr(di.second.first),
			observer(di.second.second),
			light_body(-1),
			relative_body(-1),
			GM(-1.0),
			initialized(false)
			 {}
	    call_data(const call_data& c)
		: args(c.args),
			hashval(c.hashval),
			target(c.args.first.first),
			ref_frame(c.args.first.second),
			abcorr(c.args.second.first),
			observer(c.args.second.second),
			ptime(c.ptime),
			itime(c.itime),
			ftime(c.ftime),
			light_body(c.light_body),
			relative_body(c.relative_body),
			GM(c.GM),
			initialized(c.initialized) {
		for(int k = 0; k < 6; k++) {
		    presult[k] = c.presult[k];
		    iresult[k] = c.iresult[k];
		    fresult[k] = c.fresult[k];
		}
	    }
	    ~call_data() = default;

	    call_data& operator=(const call_data& c) {
		ptime = c.ptime;
		itime = c.itime;
		ftime = c.ftime;
		args = c.args;
		hashval = c.hashval;
		target = args.first.first;
		ref_frame = args.first.second;
		abcorr = args.second.first;
		observer = args.second.second;
		light_body = c.light_body;
		relative_body = c.relative_body;
		GM = c.GM;
		initialized = c.initialized;
		for(int k = 0; k < 6; k++) {
		    presult[k] = c.presult[k];
		    iresult[k] = c.iresult[k];
		    fresult[k] = c.fresult[k];
		}
		return *this;
	    }

	    //
	    // Invoked when a call with these parameter
	    // values has a time outside the sampling
	    // interval, indicating that we need to
	    // start collecting sampling information
	    // again
	    //
	    void	reset() {
		itime = CTime_base();
		ftime = CTime_base();
	    }

	    //
	    // Computes the state at time T. Assumes
	    // that i and f data are available. Uses
	    // the formulas from the document in the
	    // git repository Europa/TOLdiff/doc.
	    //
	    void	interpolate(
				CTime_base T,
				double state[],
				aoString& info);

	    //
	    // Sets light_body, relative_body, GM and initialized.
	    // The returned body_state is the "light body", i. e.,
	    // the lighter one of the target and the observer.
	    //
	    body_state*	determine_geometry();

	    void	call_spkez(CTime_base now, TypedValue* res);

	    //
	    // Returns true if and only if we are ready to interpolate
	    //
	    bool	assess_sampling_state(
				body_state* light,
				bool& may_return,
				CTime_base& nowval,
				TypedValue* result);

	    void	output_debug_info(/* CTime_base now, */ aoString& out);

	    //
	    // The data below refer to calls to spkez
	    // with a specific set of parameters.
	    //

	    //
	    // Prefix p refers to previous call
	    // Prefix i refers to start of sampling interval
	    // prefix f refers to end of sampling interval
	    //

	    //
	    // Set by the time of the last spkez call with
	    // these params:
	    //
	    CTime_base	ptime;
	    double	presult[6];

	    //
	    // If light_body = S/C, itime and iresults are set
	    // once the states of all influencers of the S/C
	    // are known. These states are determined by calls
	    // to spkez whose relative body happens to be one
	    // of the influencers of the S/C.
	    //
	    // If light_body = a planet or a moon, itime and
	    // iresults are set proactively by the spkez wrapper;
	    // the main influencer is known, and a call to spkez
	    // is made to determine the state of the (light body,
	    // influencer) combination. The sampling rate for this
	    // light body is known since it is in orbit around its
	    // influencer. The sampling rate for the relative
	    // body, which is heavier, is always less than that of
	    // the light body.
	    //
	    // After itime is set because all influencer parameters
	    // are known, interpolation is used for subsequent
	    // calls to spkez until the time parameter exceeds
	    // ftime (see below.) When that happens, the process of
	    // determining influencer states starts over.
	    //
	    CTime_base	itime;
	    double	iresult[6];

	    //
	    // Set automatically once itime is known and all
	    // influencer states are known.
	    //
	    CTime_base	ftime;
	    double	fresult[6];

	    //
	    // Data member set by the constructor(s)
	    //
	    data_index	args;
	    int		hashval;

	    //
	    // Data members that need to be computed
	    // by determine_geometry():
	    //
	    bool	initialized;

	    // set by the call to spkez:
	    long int	target;
	    Cstring	ref_frame;
	    Cstring	abcorr;
	    long int	observer;

	    // target/observer pair, after analysis:
	    int		light_body;
	    int		relative_body;

	    // G times Mass of the relative body:
	    double	GM;
	};

	//
	// Like call_data, but intended for initialization, when
	// we are in charge of timing as opposed to the simulation
	// case, where we are responding to calls from the adaptation
	// at times that are determined by the modeling algorithm.
	//
	class cached_spkez_data {
	public:
	    cached_spkez_data() = default;
	    cached_spkez_data(const data_index& di, int h)
		: args(di),
			hashval(h),
			target(di.first.first),
			ref_frame(di.first.second),
			abcorr(di.second.first),
			observer(di.second.second),
			light_body(-1),
			relative_body(-1),
			GM(-1.0),
			initialized(false)
			 {}
	    cached_spkez_data(const cached_spkez_data& c)
		: args(c.args),
			hashval(c.hashval),
			target(c.args.first.first),
			ref_frame(c.args.first.second),
			abcorr(c.args.second.first),
			observer(c.args.second.second),
			light_body(c.light_body),
			relative_body(c.relative_body),
			GM(c.GM),
			initialized(c.initialized),
			sampled_values(c.sampled_values) {
	    }
	    ~cached_spkez_data() = default;

	    //
	    // A little lame... it is understood that the
	    // vector in this case should have exactly 6
	    // elements, the first 3 for position and the
	    // last 3 for velocity
	    //
	    typedef pair<CTime_base, vector<double> >	spice_state;
	    vector<spice_state>				sampled_values;

	    cached_spkez_data& operator=(const cached_spkez_data& c) {
		args = c.args;
		// etime = c.etime;
		hashval = c.hashval;
		target = args.first.first;
		ref_frame = args.first.second;
		abcorr = args.second.first;
		observer = args.second.second;
		light_body = c.light_body;
		relative_body = c.relative_body;
		GM = c.GM;
		initialized = c.initialized;
		return *this;
	    }

	    Cstring identify();

	    //
	    // Sets light_body, relative_body, GM, and initialized:
	    //
	    void	determine_geometry();

	    void	call_spkez(CTime_base now, TypedValue* res);

	    //
	    // The frame rotation period should be set
	    // to zero if the frame is inertial:
	    //
	    CTime_base	propagate(
				body_state*	light,
				body_state*	relative,
				CTime_base&	nowval,
				CTime_base&	end_of_plan,
				double		frame_rotation_period,
				double		state[],
				bool		first_time = false);

	    //
	    // Useful to ascertain that we have a maneuver (i. e.,
	    // a discontinuity in the velocity)
	    //
	    double	get_delta_v_at(const CTime_base&);

	    void	output_debug_info(/* CTime_base now, */ aoString& out);

	    //
	    // Data member set by the constructor(s)
	    //
	    data_index	args;
	    int		hashval;

	    //
	    // Data members that need to be computed
	    // by determine_geometry():
	    //
	    bool	initialized;

	    //
	    // Basic data (args of the call to spkez)
	    //
	    long int	target;
	    Cstring	ref_frame;
	    Cstring	abcorr;
	    long int	observer;

	    double	iresult[6];

	    //
	    // target/observer pair, after analysis:
	    //
	    int		light_body;
	    int		relative_body;

	    // G times Mass of the relative body:
	    double	GM;
	};

	static vector<call_data>&		prev_spkez_call();
	static data_map&			cached_arg_map();
	static vector<cached_spkez_data>&	cached_data_vector();

	//
	// Same as above precompute functions, but keeps data in
	// memory to allow computation of all resources derived
	// from SPKEZ calls. Returns a reference to existing data
	// if the computation was done earlier.
	//
	static cached_spkez_data&		precompute_spkez_data(
				const data_index&			indexData,
				const tlist<alpha_time, maneuver_node>&	maneuverTimes,
				bool  debug);

};


#endif /* _SPICEiNTFC_H_ */
