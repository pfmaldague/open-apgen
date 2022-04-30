#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//
// This file contains utilities for computing the sampling interval
// for spkez, given a desired tolerance for interpolated values of
// the position and velocity returned by spkez.
//
// Originally, spkez itself was going to be precomputed. Function
//
// 	void	spkez_intfc::generate_precomputed_spkez_AAF()
//
// below was based on that idea. The function is now obsolete because
// currently, precomputed resources are those that are used in the
// Europa Clipper adaptation. The function is kept just in case it
// might be useful at some later point.
//
// The other methods implemented in this file support the current
// scheme for precomputed resources:
//
//	CTime_base	spkez_intfc::cached_spkez_data::propagate(
//			body_state* light,
//			body_state* relative,
//			CTime_base& nowval,
//			CTime_base& plan_end,
//			double frame_period, // in hours
//			double args[],
//			bool first_time /* = false */ );
//
//	void spkez_intfc::cached_spkez_data::determine_geometry();
//
//	void spkez_intfc::body_state::update_influencers(CTime_base T);
//
//	long int spkez_intfc::infl_data::compute_abs_err_sec_4th()
//			CTime_base currtime,
//			long int this_body,
//			long int target_body,
//			double state[6],
//			double GM,
//			double& abs_error_vel,
//			double& abs_error_accel);
//

#define AU 150000000L // A. U. in km (approximate)
#define ONE_BILLION 1000000000L

//
// TODO: replace integer constants -159, 10, 399 etc. by
// appropriate symbolic constants defined in the SPICE toolkit.
//

#include "spiceIntfc.H"

// #define DEBUG_AAF_CREATION

extern const char* name_this_object(long target, bool append_blanks);

tlist<alpha_int, spkez_intfc::body_state>& spkez_intfc::init_bodies_of_interest() {
    static tlist<alpha_int, body_state> M;
    return M;
}

CTime_base& spkez_intfc::infl_data::deluge() {
    static CTime_base D = CTime_base("2000-001T00:00:00");
    return D;
}

double	deltav(double state1[], double state2[]) {
    double sq = 0.0;

    // iterate over velocity indices
    for(int i = 3; i < 6; i++) {
	sq += (state2[i] - state1[i]) * (state2[i] - state1[i]);
    }
    return sqrt(sq);
}

double		spkez_intfc::cached_spkez_data::get_delta_v_at(const CTime_base& man_time) {
    double nowdouble = man_time.convert_to_double_use_with_caution();
    double lt;
    double state1[6];
    double state2[6];

    //
    // Question: how far back / forward do we have to go to
    // observe the discontinuity?  Let's try a few things
    //

    // 1 second
    spkez_api(target, nowdouble + 1.0, *ref_frame, *abcorr, observer, state1, &lt);
    spkez_api(target, nowdouble - 1.0, *ref_frame, *abcorr, observer, state2, &lt);
    double deltav_1_sec = deltav(state1, state2);

    // 1 minute
    spkez_api(target, nowdouble + 60.0, *ref_frame, *abcorr, observer, state1, &lt);
    spkez_api(target, nowdouble - 60.0, *ref_frame, *abcorr, observer, state2, &lt);
    double deltav_1_min = deltav(state1, state2);

    // 1 hour
    spkez_api(target, nowdouble + 3600.0, *ref_frame, *abcorr, observer, state1, &lt);
    spkez_api(target, nowdouble - 3600.0, *ref_frame, *abcorr, observer, state2, &lt);
    double deltav_1_hr = deltav(state1, state2);

    // PFM debug
    // cerr << "    deltav (1 s) = " << deltav_1_sec << "\n";
    // cerr << "    deltav (1 m) = " << deltav_1_min << "\n";
    // cerr << "    deltav (1 h) = " << deltav_1_hr << "\n";

    return deltav_1_sec;
}


CTime_base	spkez_intfc::cached_spkez_data::propagate(
			body_state* light,
			body_state* relative,
			CTime_base& nowval,
			CTime_base& plan_end,
			double frame_period, // in hours
			double args[],
			bool first_time /* = false */ ) {

#   ifdef DEBUG_AAF_CREATION
    cerr << identify() << "->propagate: updating influencers for light body\n";
#   endif /* DEBUG_AAF_CREATION */

    //
    // The caching equivalent of this method was assess_sampling_state(),
    // which had to wait until all influencer calls had been made so
    // that the appropriate data were available. It was very important,
    // then, to flag those spkez calls that allowed us to fill in the
    // influencer data for some body_state.
    //
    // In our case, we may of course just compute the stuff we need by
    // fiat. We don't need to fetch some cached_spkez_data object to compute
    // the influencer parameters.
    //
    light->update_influencers(nowval);

    if(relative) {

#	ifdef DEBUG_AAF_CREATION
	cerr << "    updating influencers for relative body\n";
#	endif /* DEBUG_AAF_CREATION */

	relative->update_influencers(nowval);
    }

  
    double hour_to_the_4th = pow(3600.0, 4);

    //
    // We are now in a position to compute the absolute error.
    //
    double encoded_error = (double)light->payload.err_per_sec_4th();
    double decoded_error = exp(encoded_error / (double)ONE_BILLION);

    // debug
#   ifdef DEBUG_AAF_CREATION
    cerr << "    vel + accel abs. error (km * hr**4) = " << decoded_error * hour_to_the_4th << "\n";
#   endif /* DEBUG_AAF_CREATION */

    if(relative) {
	double second_error = (double)relative->payload.err_per_sec_4th();
	decoded_error += exp(second_error / (double)ONE_BILLION);

#   ifdef DEBUG_AAF_CREATION
	cerr << "    (incl. relative) vel + accel abs. error (hr) = " << decoded_error * hour_to_the_4th << "\n";
#   endif /* DEBUG_AAF_CREATION */
    }


    double nowdouble = nowval.convert_to_double_use_with_caution();
    double lt;
    spkez_api(target, nowdouble, *ref_frame, *abcorr, observer, args, &lt);

    //
    // Get magnitude of relative position so we can compute the relative error
    //
    double R = sqrt(args[0] * args[0] + args[1] * args[1] + args[2] * args[2]);

#   ifdef DEBUG_AAF_CREATION
    cerr << "    R from actual spkez call: " << R / AU << " AU\n";
#   endif /* DEBUG_AAF_CREATION */

    //
    // The first time around, we don't need a delta T;
    // we just need the spkez results.
    //
    if(first_time) {
	return nowval;
    }

    //
    // Compute sampling time for achieving the desired relative error.
    // Keep in mind that
    //
    // 	theRelError = delta_T**4 * (decoded_error / R + 4.05 / frame_T**4)
    //
    // where frame_T is the period of the rotation of the reference frame.
    // If the frame is inertial, frame_T is infinite and the second term
    // in the parentheses disappears.
    //
    double frame_rotation_error = 0.0;
    if(frame_period > 1.0E-15) {

	//
	// frame_period is in hours
	//
	frame_rotation_error = 4.05 / pow(3600.0 * frame_period, 4);
    }

    double sampling_seconds = pow(theRelError
		/ (decoded_error / R + frame_rotation_error), 0.25);

#   ifdef DEBUG_AAF_CREATION
    cerr << "    rel. error (hr) = " << decoded_error * hour_to_the_4th / R
    	<< " + frame error " << frame_rotation_error * hour_to_the_4th
    	<< "; sampling interval = " << sampling_seconds / 3600.0 << " hrs\n";
#   endif /* DEBUG_AAF_CREATION */

    //
    // Make sure it's not too immense
    //
    if(sampling_seconds > 3E+7) {

	//
	// about 1 year
	//
	sampling_seconds = 3E+7;
    }
    CTime_base delta0 = CTime_base::convert_from_double_use_with_caution(
							sampling_seconds,
							/* duration? */ true);

    if(nowval + delta0 > plan_end) {
	nowdouble = plan_end.convert_to_double_use_with_caution();
	spkez_api(target, nowdouble, *ref_frame, *abcorr, observer, args, &lt);
	return plan_end;
    }

    nowval = nowval + delta0;
    nowdouble = nowval.convert_to_double_use_with_caution();
    spkez_api(target, nowdouble, *ref_frame, *abcorr, observer, args, &lt);

    return nowval;
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
// After a call to determine_geometry(), the target and observer
// of the cached_spkez_data object are both identified as light or relative
// objects, both documented in init_bodies_of_interest().
//
void spkez_intfc::cached_spkez_data::determine_geometry() {
    body_state*	light = NULL;

    if(initialized) {
	return;
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

#   ifdef DEBUG_AAF_CREATION
    cerr << "determine geometry: observer = " << observer << ", target = "
         << target << ", frame = " << ref_frame << "\n";
    cerr << "                    light_body = " << light_body << ", relative_body = " << relative_body << "\n";
#   endif /* DEBUG_AAF_CREATION */

    GM = GM_for_target(relative_body);

    //
    // Make sure we've covered all possibilities
    //
    if(GM < 0.0) {
	Cstring errs("spkez error: not all GM parameters are known.");
	errs << "\nCould not find " << name_this_object(relative_body, false);
	throw(eval_error(errs));
    }

    light = init_bodies_of_interest().find(light_body);

    //
    // Sanity check
    //
    assert(light);
    assert(light->payload.infl_in_incr_abs_err_order.get_length() == 11
	   || light->payload.infl_in_incr_abs_err_order.get_length() == 1);

    return;
}

void spkez_intfc::body_state::update_influencers(CTime_base T) {
    slist<alpha_int, ptr2influencer>::iterator
			infl_iter(payload.infl_in_incr_body_id_order);
    ptr2influencer*	heavy_ptr;

#   ifdef DEBUG_AAF_CREATION
    cerr << "\t" << T.to_string() << " - " << identify() << "->update infl:\n";
#   endif /* DEBUG_AAF_CREATION */

    //
    // Loop through all influencers of this body, and
    // for each one, use spkez to get its state; having
    // done that, store the parameters that will be useful
    // to estimate the interpolation error.
    //
    while((heavy_ptr = infl_iter())) {
	influencer*	heavy = heavy_ptr->payload;
	if(heavy->payload.get_previous_time() == T) {

#	    ifdef DEBUG_AAF_CREATION
	    cerr << "\tsame time - returning.\n";
#	    endif /* DEBUG_AAF_CREATION */

	    continue;
	}
	int		heavy_body0 = heavy_ptr->Key.get_int();

	payload.infl_in_incr_abs_err_order.remove_node(heavy);

	double		pos_and_vel[6];

	//
	// The previous state was captured at time
	// previous_time, which is set to deluge()
	// if the parameters have never been evaluated.
	//
	double GM0 = GM_for_target(heavy_body0);
	double abs_error_velocity = 0.0;
	double abs_error_acceleration = 0.0;

	heavy->Key = heavy->payload.compute_abs_err_sec_4th(
				T,
				Key.get_int(),
				heavy_body0,
				pos_and_vel,
				GM0,
				abs_error_velocity,
				abs_error_acceleration
				);

#	ifdef DEBUG_AAF_CREATION
	double hour_to_the_4th = pow(3600.0, 4);
	cerr << "\t    " << name_this_object(heavy->payload.bodyID, false)
		<< " contributes Err.vel = " << abs_error_velocity * hour_to_the_4th
	 	<< ", Err.accel = " << abs_error_acceleration * hour_to_the_4th
		<< " (R = " << sqrt(
				pos_and_vel[0] * pos_and_vel[0]
				+ pos_and_vel[1] * pos_and_vel[1]
				+ pos_and_vel[2] * pos_and_vel[2]) / AU
		<< " AU)\n";
#	endif /* DEBUG_AAF_CREATION */

	payload << heavy;
    }
}


//
// Given the state[6] of a body relative to an "influencing
// body" whose GM parameter (G = gravitational constant, M = mass)
// is also given, this function computes the absolute error per
// second to the fourth power when using cubic interpolation to
// compute the position of the body relative to an inertial frame.
//
long int spkez_intfc::infl_data::compute_abs_err_sec_4th(
			CTime_base currtime,
			long int this_body,
			long int target_body,
			double state[6],
			double GM,
			double& abs_error_vel,
			double& abs_error_accel) {

    assert(currtime > previous_time);

    double	nowtime = currtime.convert_to_double_use_with_caution();
    //
    // According to the V&V document in TOLdiff/doc, the formula
    // for the relative error when using cubic interpolation between
    // exact values is 
    //
    //	dR/R = 0.0624 v^2 (GM / R^5) deltaT^4		(1)
    //
    // This results from combining the formulas
    //
    // 	| ä | < 24 GM v^2 / R ^ 4
    //
    // obtained for solutions of Newton's equation and
    //
    //  dR < 0.0026 | ä | dt ^ 4
    //
    // obtained by analyzing the cubic interpolation polynomial
    // that guarantees continuity of position and velocity values
    // across sampling points.
    //
    // Important note
    // --------------
    //
    // The above formula is an underestimate when the S/C is aimed
    // straight at a planet, which is the case in a gravity assist.
    //
    // In that case we have a correction
    //
    // | ä | < 2 GM a / R ^ 3
    //
    // where a is an approximation to the acceleration.
    //
    double		lt;

    spkez_api(
	this_body,
	nowtime,
	"J2000",
	"NONE",
	target_body,
	state,
	&lt);

    //
    // distance to the target in km ^ 2
    //
    double R2 = state[0] * state[0] + state[1] * state[1] + state[2] * state[2];
    R = sqrt(R2);

    //
    // velocity^2 in (km / s)^2
    //
    double v2 = state[3] * state[3] + state[4] * state[4] + state[5] * state[5];
    V = sqrt(v2);

    double abs_error_km = 0.0;
    double A = 0.0;

    //
    // Estimate the acceleration.  This can only be done
    // if previous data are available.
    //
    if(previous_time > infl_data::deluge()) {
	double deltaT = (currtime - previous_time).convert_to_double_use_with_caution();
	double A2 = 0.0;
	for(int ac = 3; ac < 6; ac++) {
	    double accel = (state[ac] - previous_state[ac]) / deltaT;
	    A2 += accel * accel;
	}
	A = sqrt(A2);
	abs_error_km = GM * (0.0624 * v2 / R2  + 0.0052 * A / R) / R2;

	abs_error_vel = GM * 0.0624 * v2 / (R2 * R2);
	abs_error_accel = GM * 0.0052 * A / (R * R2);
    } else {
	abs_error_km = 0.0624 * GM * v2 / (R2 * R2);
	abs_error_vel = GM * 0.0624 * v2 / (R2 * R2);
    }

    //
    // estimate the deltaT
    //
    double sampling_seconds = pow(theRelError / (abs_error_km / R), 0.25);

    //
    // estimate error again at the new time point
    //
    CTime_base delta0 = CTime_base::convert_from_double_use_with_caution(
							sampling_seconds,
							/* duration? */ true);
    CTime_base new_time = currtime + delta0;

    if(new_time < PlanEnd) {
	nowtime = new_time.convert_to_double_use_with_caution();
	double	newstate[6];

	spkez_api(
	    this_body,
	    nowtime,
	    "J2000",
	    "NONE",
	    target_body,
	    newstate,
	    &lt);
	double newR2 = newstate[0] * newstate[0] + newstate[1] * newstate[1] + newstate[2] * newstate[2];
	double newR = sqrt(newR2);
	double newv2 = newstate[3] * newstate[3] + newstate[4] * newstate[4] + newstate[5] * newstate[5];
	double newV = sqrt(newv2);

	double new_abs_error_km = GM * (0.0624 * newv2 / newR2  + 0.0052 * A / newR) / newR2;
	if(new_abs_error_km > abs_error_km) {
	    abs_error_km = new_abs_error_km;
	    abs_error_vel = GM * 0.0624 * newv2 / (newR2 * newR2);
	    abs_error_accel = GM * 0.0052 * A / (newR * newR2);
	    for(int old = 0; old < 6; old++) {
		state[old] = newstate[old];
	    }
	}
    }

    previous_time = currtime;
    for(int old = 0; old < 6; old++) {
	previous_state[old] = state[old];
    }

    long int encoded_err = (long int) (ONE_BILLION * log(abs_error_km));

    return encoded_err;
}

void	spkez_intfc::generate_precomputed_spkez_AAF() {

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

    FILE*	AAFfile = fopen(*spkez_intfc::AAFname(), "w");
    if(!AAFfile) {
	Cstring err;
	err << "generate_geom_resources: cannot create file \"" << spkez_intfc::AAFname() << "\"\n";
	throw(eval_error(err));
    }
    Cstring	header = "apgen version \"Geometry Subsystem AAF\"\n\n";
    fwrite(*header, 1, header.length(), AAFfile);

    aoString one_time_series;
    one_time_series << "precomputed_resource pre_spkez\n"
	<< "    begin\n"
	<< "\tparameters\n"
	<< "\n"
	<< "\t    #\n"
	<< "\t    # When spkez is invoked, it should be given the parameters\n"
	<< "\t    # specified below, including a time parameter\n"
	<< "\t    #\n"
	<< "\t    target: integer default to -159;\n"
	<< "\t    frame: string default to \"J2000\";\n"
	<< "\t    abcorr: string default to \"NONE\";\n"
	<< "\t    observer: integer default to 5;\n"
	<< "\t    eval_time: time default to 2000-001T00:00:00;\n"
	<< "\n\t    #\n"
	<< "\t    # Method precomp__initialize() is executed once, when the\n"
	<< "\t    # definition of the precomputed resource is read\n"
	<< "\t    # from an AAF\n"
	<< "\t    #\n"
	<< "\tfunction precomp__initialize()\n"
	<< "\t{\n";

    one_time_series << "\n\t    #\n"
	<< "\t    # time_series() is a built-in function in APGenX. It takes\n"
	<< "\t    # two arguments. The first is a list of non-time\n"
	<< "\t    # parameters as specified above. The second is an array of\n"
	<< "\t    # interpolation data (values + time derivatives) specified\n"
	<< "\t    # at a number of sampling times. These times were pre-computed\n"
	<< "\t    # so as to maximize the spacing between points while maintaining\n"
	<< "\t    # the required accuracy.\n"
	<< "\t    #\n"
	<< "\t    # Each call to time_series() adds interpolation data for\n"
	<< "\t    # one particular combination of non-time parameters.\n"
	<< "\t    # When spkez is invoked, the non-time parameters are collected,\n"
	<< "\t    # the appropriate interpolation data are retrieved\n"
	<< "\t    # from the database, and the value at the time specified as\n"
	<< "\t    # the last parameter is evaluated using cubic interpolation.\n"
	<< "\t    #\n";

    for(int w = 0; w < master_list_of_spkez_arguments().size(); w++) {

	data_index& one_value = master_list_of_spkez_arguments()[w];
	cached_spkez_data* theInitData = new cached_spkez_data(one_value, w);

	theInitData->determine_geometry();

	//
	// Determine the name of the Geometry Subsystem resource
	//
	aoString one_AAF_record;
	Cstring geom_res_name = "time_series(";
	char u[128];
	sprintf(u, "[%d, \"%s\", \"%s\", %d],\n",
		one_value.first.first,
		*one_value.first.second,
		*one_value.second.first,
		one_value.second.second);
	geom_res_name << u;

	one_AAF_record << "\n\t# light body " << name_this_object(theInitData->light_body, false)
	    << ", heavy body " << name_this_object(theInitData->relative_body, false) << "\n"
	    << "\t" << geom_res_name;
	one_AAF_record << "\t[\n";

	spkez_intfc::body_state* relative = NULL;

	//
	// IDs of the two bodies
	//
	light = init_bodies_of_interest().find(theInitData->light_body);
	relative = init_bodies_of_interest().find(theInitData->relative_body);

	light->payload.reset();
	if(relative) {
	    relative->payload.reset();
	}

	//
	// Period of rotation of the reference frame, 0.0 if infinite (!)
	//
	map<Cstring, double>::const_iterator frame_iter
		= frame_info().find(one_value.first.second);
	assert(frame_iter != frame_info().end());
	double	frame_period = frame_iter->second;

	//
	// 10 is the Sun Barycenter, which is the origin of the system
	//
	assert(relative || theInitData->relative_body == 10);

	CTime_base	newtime = PlanStart;
	CTime_base	currenttime = newtime;
	bool		first_time = true;

	while(currenttime < PlanEnd) {
	    double state[6];

	    //
	    // Now we want to generate a history for this
	    // particular combination of spkez parameters.
	    // 
	    // 2.1 start at PlanStart and evaluate influencers.
	    //
	    if(first_time) {
		first_time = false;

		//
		// The first time around, newtime is not changed.
		//
		theInitData->propagate(light, relative,
				currenttime, PlanEnd, frame_period, state, true);

		one_AAF_record << "\t\"" << newtime.to_string() << "\" = ["
		    << Cstring(state[0], MAX_SIGNIFICANT_DIGITS) << ", "
		    << Cstring(state[1], MAX_SIGNIFICANT_DIGITS) << ", "
		    << Cstring(state[2], MAX_SIGNIFICANT_DIGITS) << ", "
		    << Cstring(state[3], MAX_SIGNIFICANT_DIGITS) << ", "
		    << Cstring(state[4], MAX_SIGNIFICANT_DIGITS) << ", "
		    << Cstring(state[5], MAX_SIGNIFICANT_DIGITS) << "],\n";
	    }

	    newtime = theInitData->propagate(light, relative,
					currenttime, PlanEnd, frame_period, state);

	    one_AAF_record << "\t\"" << newtime.to_string() << "\" = ["
		<< Cstring(state[0], MAX_SIGNIFICANT_DIGITS) << ", "
		<< Cstring(state[1], MAX_SIGNIFICANT_DIGITS) << ", "
		<< Cstring(state[2], MAX_SIGNIFICANT_DIGITS) << ", "
		<< Cstring(state[3], MAX_SIGNIFICANT_DIGITS) << ", "
		<< Cstring(state[4], MAX_SIGNIFICANT_DIGITS) << ", "
		<< Cstring(state[5], MAX_SIGNIFICANT_DIGITS) << "]";
	    if(newtime < PlanEnd) {
		one_AAF_record << ",";
	    }
	    one_AAF_record << "\n";
	    currenttime = newtime;
	}

	one_AAF_record << "\t]);\n";
	Cstring rec = one_AAF_record.str();
	one_time_series << rec;

	//
	// Write what you've got so far
	//
	Cstring series1 = one_time_series.str();
	fwrite(*series1, 1, series1.length(), AAFfile);
	one_time_series.clear();

	//
	// end of one iteration
	//
	delete theInitData;
    }
    one_time_series << "\t}\n"
	<< "    end precomputed_resource pre_spkez\n";
    Cstring series = one_time_series.str();
    fwrite(*series, 1, series.length(), AAFfile);

    //
    // SUCCESS
    //
    fclose(AAFfile);
}
