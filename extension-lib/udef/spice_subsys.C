#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string>
#include <unistd.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <memory>
#include <stdlib.h>


#include "APdata.H"
#include "RES_eval.H"
#include "apDEBUG.H"

#include "spiceIntfc.H"

#define ARG_POINTER args.size()
#define ARG_POP args.pop_front()
#define ARG_STACK slst<TypedValue*>& args
#define WHERE_TO_STOP 0

using namespace apgen;

#define  MAXWIN  10000

//
// spkez_api_ap and spkez_control_ap are implemented in spkez_intfc.C:
//
extern RETURN_STATUS spkez_api_ap ( Cstring & errs , TypedValue * result , ARG_STACK );
// extern RETURN_STATUS spice_spkez_ap ( Cstring & errs , TypedValue * result , ARG_STACK );
extern RETURN_STATUS spkez_control_ap ( Cstring & errs , TypedValue * result , ARG_STACK );
extern RETURN_STATUS create_geom_subsys_aaf_ap ( Cstring & errs , TypedValue * result , ARG_STACK );
extern RETURN_STATUS create_geom_subsys_ap ( Cstring & errs , TypedValue * result , ARG_STACK );

extern double	get_Sun_Elevation();
extern double	get_Sun_Azimuth();
extern double	get_Earth_Elevation();
extern double	get_Earth_Azimuth();

extern void       init_spice(const char *metakernel_file);
extern void       reinit_spice(const char *metakernel_file);
extern double     compute_Flyby_Declination( int sc_id, double time );
extern void       vprjp_api( double positn[3], double normal[3], double point[3], double *proj );
extern void       spkcov_api( int idcode,  double *ETWindow );
extern void       spkcpt_api( double trgpos_x, double trgpos_y, double trgpos_z, const char* trgctr, const char *trgref, double utc,
                              const char *outref, const char *refloc, const char *abcorr, const char *obs, double *state, double *lt );
extern void       latrec_api( double radius, double longitude, double latitude, double * rectan );
extern void       reclat_api( double rectan_x, double rectan_y, double rectan_z, double* radius, double* longitude, double* latitude);
extern void       recsph_api( double rectan_x, double rectan_y, double rectan_z, double *radius, double* colat, double* lon);
extern void       recgeo_api( double rectan_x, double rectan_y, double rectan_z, double re, double f, double* lon, double* lat, double* alt);
extern void       Cart2RADec( double r[3], double *RADec);
extern double     lspcn_api( const char *body, double utc, const char *lt);
extern void       pxform_api( const char *from_frame , const char* to_frame, double utc, double cmat[3][3] );
extern void       twovec_api( double axdef[3] ,int indexa, double plndef[3] ,int indexp, double cmat[3][3] );
extern void       m2q_api( double cmat[3][3], double quat[4] );
extern void       q2m_api(  double q[4], double r[3][3] );
extern void       m2eul_api(double r[3][3], int axis3, int axis2, int axis1, double AngleArray[3] );

extern void       vzfrac_api( int frontId, const char *frontFrame, double frontRadii[3], int backID, const char *backFrame, double backRadii[3], int obsID, const char *abcorr, double time, double *frac);
extern void       bodvrd_api( const char* body_name, const char* item, int maxn, int* dim, double* values );
extern void       sincpt_api( const char* method, const char* target, double utc, const char* fixref, const char* abcorr, const char* obsrvr, const char* dref, double dvec[3], double spoint[3], double* trgepc, double srfvec[3], int* found);
extern void       subpnt_api( const char* method, const char* target, double utc, const char* fixref, const char* abcorr, const char* obsrvr, double spoint[3], double* trgepc, double srfvec[3] );
extern void       ilumin_api(   const char* method, const char* target, double utc, const char* fixref, const char* abcorr, const char* obsrvr, double spoint[3], double* trgepc, double srfvec[3], double* phase, double* solar, double* emissn );
extern void       gfsep_api(  const char* targ1, const char* shape1, const char* frame1, const char* targ2, const char* shape2, const char* frame2, const char* abcorr, const char* obsrvr, const char* relate, double refval, double adjust, double step, int nintvls, double cnfine[2], double *result, int* numWindows );
extern void       gfposc_api(  const char* target,  const char* frame, const char* abcorr, const char* obsrvr, const char* crdsys, const char* coord, const char* relate, double refval, double adjust, double step, int nintvls, double cnfine[2], double *result, int* numWindows );
extern double       et2lst_api( double utc, const char* body, double lon, const char* type );
extern void       gfdist_api(  const char* target,  const char* abcorr, const char* obsrvr, const char* relate, double refval, double adjust, double step, int nintvls, double cnfine[2], double *result, int* numWindows );
extern void       gfilum_api(  const char* method,  const char* angtyp, const char* target, const char* illmn, const char* fixref, const char* abcorr, const char* obsrvr, double spoint[3], const char* relate, double refval, double adjust, double step, int nintvls, double cnfine[2], double *result, int* numWindows );
extern void       gflst_api(  const char* target,   const char* fixref, const char* abcorr, const char* obsrvr, const char* relate, double refval, double adjust, double step, int nintvls, double cnfine[2], double *result, int* numWindows );
extern void       gfpa_api(   const char* target, const char* illmn, const char* abcorr, const char* obsrvr, const char* relate, double refval, double adjust, double step, int nintvls,  double cnfine[2], double *result, int* numWindows );
extern void       gfemi_api(  const char* target,   const char* fixref, const char* abcorr, const char* obsrvr, const char* relate, double refval, double adjust, double step, int nintvls, double cnfine[2], double *result, int* numWindows );
extern void       gfinc_api(  const char* target,   const char* fixref, const char* abcorr, const char* obsrvr, const char* relate, double refval, double adjust, double step, int nintvls, double cnfine[2], double *result, int* numWindows );
extern void       gflllon_api(  const char* target,   const char* fixref, const char* abcorr, const char* obsrvr, const char* relate, double refval, double adjust, double step, int nintvls, double cnfine[2], double *result, int* numWindows );
extern void       gftermlon_api(  const char* target,   const char* fixref, const char* abcorr, const char* obsrvr, const char* relate, double refval, double adjust, double step, int nintvls, double cnfine[2], double *result, int* numWindows );

extern double     get_horizon_elevation( const char *dss, double azimuth);
extern double     compute_light_time( double time, int Observer, const char *Direction, int Target );
extern double     get_Flyby_q(int i );
extern double     Compute_Boresight_Angle( double time, char *frame, char* axis, int target );
extern void       Compute_Closest_Approach_Times(  char *obsrvr, char *target, char* relate, double refval,double IntervalStart, double IntervalEnd, double StepSize, double *Closest_Aproach_Times, int* actNumWin);
extern void       Compute_Range_Windows(  char *obsrvr, char *target, char *targetFrame, char* relate, double refval, double IntervalStart, double IntervalEnd, double StepSize, double *Closest_Aproach_Times, int* actualNumberOfWindows);

extern void        Compute_Occultation_Times(  char *OccultationType,  char *obsrvr, char *target, char *TargetBodyFrame, char* TargetShape, char *OccultingBody, char *OccultingBodyFrame, double IntervalStart, double IntervalEnd, double StepSize, double *OccultationWindows, int* actNumWin );
extern int Compute_Surface_Intercept( double time, char *target, char *target_frame, char *obsrvr, double dvec0, double dvec1, double dvec2, double *dist, double *lat, double *lon, double *radius, double *PlanetographicLon, double *PlanetographicLat, double *PlanetographicAlt);
void Compute_Ang_Sep_Times(  char *targ1, char *shape1, char *frame1, char *targ2, char *shape2, char *frame2, char *obsrvr, char *relate , double refval, double IntervalStart, double IntervalEnd, double StepSize, double *Sep_Times, int* actualNumberOfWindows );

void Compute_Pole_Crossing_Times(  char *target, char *fixref, char *obsrvr, char *relate , double refval, double IntervalStart, double IntervalEnd, double step, double *Sep_Times, int* actualNumberOfWindows );
void Compute_Ang_Sep_Vec_Times( char *target,
				 char *frame,
				 char *abcorr,
				 char *obsrvr,
				 char *crdsys,
				 char *coord,
				 char *relate ,
				 double refval,
				 double adjust, double IntervalStart, double IntervalEnd, double step, double *Sep_Times, int* actualNumberOfWindows  );
extern void       compute_station_angles( double time, int naifid, const char *frame, int sc_id, double* theStationVector);

extern TypedValue
create_float_list(int n, double* d);

extern TypedValue
create_float_struct(int n, const char* indices[], double* d);

extern double Time_to_ET( double time );

extern apgen::RETURN_STATUS
get_double_array(Cstring& errs, const TypedValue& given, double returned[], int returned_size);
extern double get_Orb_Elements(int i);
extern void compute_Orb_Elements(double ET,double MU,double x,double y, double z, double xdot, double ydot, double zdot);
extern double     ET_to_VTC( double ET );
extern double     scet2sclk( char *SCET, int Spacecraft );
extern char      *sclk2scet( double SCLK, int Spacecraft );
extern char      *ET2UTCS( double ET );
extern char      *utc2excel( double time );
extern char      *utc2jd( double time );


// @@@ START SPICE_CALC

RETURN_STATUS get_Sun_Elevation_ap ( Cstring & errs , TypedValue * result , ARG_STACK ) {
    errs.undefine() ;
    if( ARG_POINTER != ( WHERE_TO_STOP + 0 ) ) {
	errs << "  wrong number of parameters passed to user-defined function get_Batt_Temp_ap; expected 0, got "
	    << ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
	return RETURN_STATUS::FAIL ;
    }
    try {
    *result = get_Sun_Elevation() ;
    } catch(eval_error Err) {
    	errs = Err.msg;
    	return RETURN_STATUS::FAIL;
    }
    return RETURN_STATUS::SUCCESS ;
}
RETURN_STATUS get_Sun_Azimuth_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{

	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 0 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function get_Batt_Temp_ap; expected 0, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	try {
	*result = get_Sun_Azimuth() ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	return RETURN_STATUS::SUCCESS ;
	}
RETURN_STATUS get_Earth_Elevation_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{

	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 0 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function get_Batt_Temp_ap; expected 0, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	try {
	*result = get_Earth_Elevation() ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	return RETURN_STATUS::SUCCESS ;
	}
RETURN_STATUS get_Earth_Azimuth_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{

	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 0 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function get_Batt_Temp_ap; expected 0, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	try {
	*result = get_Earth_Azimuth() ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	return RETURN_STATUS::SUCCESS ;
	}

RETURN_STATUS get_horizon_elevation_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1, // "dss_id"
	                * n2; // "azimuth"

	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 2 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function get_horizon_elevation_ap; expected 2, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if( ! n1->is_string() )
		{
		errs << "get_horizon_elevation_ap: first argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n2->is_numeric() )
		{
		errs << "get_horizon_elevation_ap: first argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	try {
	*result = get_horizon_elevation(  *n1->get_string(),  n2->get_double()) ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	return RETURN_STATUS::SUCCESS ;
	}
RETURN_STATUS compute_station_angles_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1, // "time"
	                * n2, // "observatorynaifid"
	                * n3, // "frame"
	                * n4; // "sc_id"
	double		stationvec[3];
	const char*	indices[3] = {"range", "azimuth", "elevation"};

	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 4 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function compute_station_angles_ap; expected 3, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if( n1->get_type() != DATA_TYPE::TIME )
		{
		errs << "compute_station_angles_ap: first argument is not a time\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( n2->get_type() != DATA_TYPE::INTEGER)
		{
		errs << "compute_station_angles_ap: second argument is not a int\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n3->is_string() )
		{
		errs << "compute_station_angles_ap: third argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( n4->get_type() != DATA_TYPE::INTEGER )
		{
		errs << "compute_station_angles_ap: fourth argument is not a int\n" ;
		return RETURN_STATUS::FAIL ;
		}
	try {
	compute_station_angles(  n1->get_double(), n2->get_int(),   *n3->get_string(), n4->get_int(), stationvec) ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	*result = create_float_struct(3, indices, stationvec);
	return RETURN_STATUS::SUCCESS ;
	}

RETURN_STATUS  generateCK( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1; // "seq_id"

	char SystemCommand[256];
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function generateCK; expected 1, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n1 = ARG_POP ;
	if( ! n1->is_string() )
		{
		errs << "generateCK: first argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	sprintf(SystemCommand,"generateCK %s",*n1->get_string());
	*result = (long) system(SystemCommand);
	return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS reinit_spice_ap ( Cstring & errs , TypedValue * result , ARG_STACK ) {
	TypedValue	* n1; // "seq_id"

	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function reinit_spice_ap; expected 1, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n1 = ARG_POP ;
	if( ! n1->is_string() )
		{
		errs << "reinit_spice_ap: first argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	try {
	reinit_spice( *n1->get_string() ) ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	*result = 1L;
	return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS init_spice_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1; // "metakernal_file"

	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function init_spice_ap; expected 1, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n1 = ARG_POP ;
	if( ! n1->is_string() )
		{
		errs << "init_spice_ap: first argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	try {
	init_spice( *n1->get_string() ) ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	*result = 1L;
	return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS compute_light_time_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1, // "time"
	                * n2, // "int"
	                * n3, // "string"
	                * n4; // "int"
	double		R;

	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 4 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function compute_light_time_ap; expected 4, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if( n1->get_type() != DATA_TYPE::TIME )
		{
		errs << "compute_light_time_ap: first argument is not a time\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n2->get_type() != DATA_TYPE::INTEGER )
		{
		errs << "compute_light_time_ap: second argument is not a integer\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( !n3->is_string() )
		{
		errs << "compute_light_time_ap: third argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n4->get_type() != DATA_TYPE::INTEGER )
		{
		errs << "compute_light_time_ap: fourth argument is not a integer\n" ;
		return RETURN_STATUS::FAIL ;
		}
	try {

	R = compute_light_time(  n1->get_double(), n2->get_int(), *n3->get_string(), n4->get_int());
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	// always a duration
	*result = CTime_base::convert_from_double_use_with_caution(R, true);
	return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS Time_to_ET_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1; // "time"

	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function Time_to_ET_ap; expected 1, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n1 = ARG_POP ;

	if( n1->get_type() != DATA_TYPE::TIME )
	  {
	    errs << "Time_to_ET_ap: first argument is not a time\n" ;
	    return RETURN_STATUS::FAIL ;
	  }
	try {
	*result = Time_to_ET(  n1->get_double());
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	return RETURN_STATUS::SUCCESS ;
	}

RETURN_STATUS spkcov_api_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	  TypedValue	* n1; // "idcode"

	double		ETWindow[2];
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function spkcov_api_ap; expected 1, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n1 = ARG_POP ;
	if(  n1->get_type() != DATA_TYPE::INTEGER )
		{
		errs << "spkcov_api_ap: first argument is not an integer\n" ;
		return RETURN_STATUS::FAIL ;
		}

	try {
	spkcov_api( n1->get_int(), ETWindow);
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	*result = create_float_list(2, ETWindow);
	return RETURN_STATUS::SUCCESS ;
	}
RETURN_STATUS spkcpt_api_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1,  // "trgpos_x"
	            * n2,  // "trgpos_y"
	            * n3,  // "trgpos_z"
	            * n4,  // "trgctr"
	            * n5,  // "trgref"
	            * n6,  // "utc"
	            * n7,  // "outref"
	            * n8,  // "refloc"
	            * n9,  // "abcorr"
	            * n10; // "obs"

	double		state[6];
	double          lt;
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 10 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function spkcpt_api_ap; expected 10, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n10 = ARG_POP ;
	n9 = ARG_POP ;
	n8 = ARG_POP ;
	n7 = ARG_POP ;
	n6 = ARG_POP ;
	n5 = ARG_POP ;
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if( n1->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "spkcpt_api_ap: first argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n2->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "spkcpt_api_ap: second argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n3->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "spkcpt_api_ap: third argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( ! n4->is_string() )
		{
		errs << "spkcpt_api_ap: fourth argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( ! n5->is_string() )
		{
		errs << "spkcpt_api_ap: fifth argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n6->get_type() != DATA_TYPE::TIME  )
		{
		errs << "spkcpt_api_ap: sixth argument is not a time\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( ! n7->is_string()  )
		{
		errs << "spkcpt_api_ap: seventh argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( ! n8->is_string()  )
		{
		errs << "spkcpt_api_ap: eighth argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( ! n9->is_string()  )
		{
		errs << "spkcpt_api_ap: ninth argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( ! n10->is_string()  )
		{
		errs << "spkcpt_api_ap: tenth argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}

	try {
	spkcpt_api( n1->get_double(), n2->get_double(), n3->get_double(), *n4->get_string(), *n5->get_string(),
	            n6->get_double(), *n7->get_string(), *n8->get_string(), *n9->get_string(), *n10->get_string(), state, &lt);
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	*result = create_float_list(6, state);
	return RETURN_STATUS::SUCCESS ;
	}

RETURN_STATUS vprjp_api_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue  * n1,  // "positn"
	            * n2,  // "normal"
	            * n3;  // "point"

	double		proj[3];
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 3 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function vprjp_api_ap; expected 3, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(!n1->is_array()) {
		errs << "vprjp_api error: first argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
        }
	ListOVal*	given_matrix = &n1->get_array();
	if(given_matrix->get_length() != 3) {
		errs << "vprjp_api error: first argument expected given array to have 3 items in it\n";
		return RETURN_STATUS::FAIL;
        }
	ArrayElement*	given_matrix_element;
	double      positn[3];
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "vprjp_api vprjp_api: expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
                }

		double	value = given_matrix_element->Val().get_double();
		positn[index] = value;
	}

	if(!n2->is_array()) {
		errs << "vprjp_api error: second argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
        }
	given_matrix = &n2->get_array();
	if(given_matrix->get_length() != 3) {
		errs << "vprjp_api error: second argument expected given array to have 3 items in it\n";
		return RETURN_STATUS::FAIL;
        }		
	double      normal[3];
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "vprjp_api vprjp_api: expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
                }

		double	value2 = given_matrix_element->Val().get_double();
		normal[index] = value2;
	}


	if(!n3->is_array()) {
		errs << "twovec_api error: third argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
        }
	given_matrix = &n3->get_array();
	if(given_matrix->get_length() != 3) {
		errs << "vprjp_api error: third argument expected given array to have 3 items in it\n";
		return RETURN_STATUS::FAIL;
        }		
	double      point[3];
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "vprjp_api vprjp_api: expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
		}

		double	value3 = given_matrix_element->Val().get_double();
		point[index] = value3;
	}


	try {
	vprjp_api(positn, normal, point, proj);
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	*result = create_float_list(3, proj);
	return RETURN_STATUS::SUCCESS ;
}		
	
RETURN_STATUS latrec_api_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1, // "radius"
	            * n2, // "longitude"
	            * n3; // "latitdue"

	double		rectan[3];
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 3 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function latrec_api; expected 3, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if( n1->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "latrec_api_ap: first argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n2->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "latrec_api_ap: second argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n3->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "latrec_api_ap: third argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}

	try {
	latrec_api( n1->get_double(), n2->get_double(), n3->get_double(), rectan);
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	*result = create_float_list(3, rectan);
	return RETURN_STATUS::SUCCESS ;
	}

RETURN_STATUS Cart2RADec_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	  TypedValue	* n1; // "r"

	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function Cart2RADec_ap; expected 1, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n1 = ARG_POP ;
	if( !n1->is_array() ){
	  errs << "Cart2RADec_ap: first argument is not an array\n" ;
	  return RETURN_STATUS::FAIL ;
	}
	double r[3];
	double RADec[2];
	if(get_double_array(errs, *n1, r, 3) != apgen::RETURN_STATUS::SUCCESS) {
	  return apgen::RETURN_STATUS::FAIL;
        }
	try {
	Cart2RADec( r, RADec);
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}

	*result = create_float_list(2, RADec);
	return RETURN_STATUS::SUCCESS ;
	}

RETURN_STATUS reclat_api_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1, // "rectan_x"
	            * n2, // "rectan_y"
	            * n3; // "rectan_z"

	double		radius;
	double		longitude;
	double		latitude;
	double      output[3];
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 3 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function reclat_api_ap; expected 3, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if( n1->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "reclat_api_ap: first argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n2->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "reclat_api_ap: second argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n3->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "reclat_api_ap: third argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}

	try {
	reclat_api( n1->get_double(), n2->get_double(), n3->get_double(), &radius, &longitude, &latitude);
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	output[0] = radius;
	output[1] = longitude;
	output[2] = latitude;

	*result = create_float_list(3, output);
	return RETURN_STATUS::SUCCESS ;
	}

RETURN_STATUS recsph_api_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue  * n1, // "rectan_x"
	            * n2, // "rectan_y"
	            * n3; // "rectan_z"

	double		radius;
	double		colat;
	double		lon;
	double      output[3];
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 3 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function recsph_api_ap; expected 3, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if( n1->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "recsph_api_ap: first argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n2->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "recsph_api_ap: second argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n3->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "recsph_api_ap: third argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}

	try {
	recsph_api( n1->get_double(), n2->get_double(), n3->get_double(), &radius, &colat, &lon);
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	output[0] = radius;
	output[1] = colat;
	output[2] = lon;

	*result = create_float_list(3, output);
	return RETURN_STATUS::SUCCESS ;
	}

RETURN_STATUS recgeo_api_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1, // "rectan_x"
	            * n2, // "rectan_y"
	  * n3, // "rectan_z"
	  * n4, // "re"
	  * n5; // "f"

	double		lon;
	double		lat;
	double		alt;
	double      output[3];
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 5 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function recgeo_api_ap; expected 5, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n5 = ARG_POP ;
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if( n1->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "recgeo_api_ap: first argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n2->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "recgeo_api_ap: second argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n3->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "recgeo_api_ap: third argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n4->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "recgeo_api_ap: fourth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n5->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "recgeo_api_ap: fifth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}

	try {
	recgeo_api( n1->get_double(), n2->get_double(), n3->get_double(), n4->get_double(), n5->get_double(), &lon, &lat, &alt);
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	output[0] = lon;
	output[1] = lat;
	output[2] = alt;

	*result = create_float_list(3, output);
	return RETURN_STATUS::SUCCESS ;
	}

RETURN_STATUS lspcn_api_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1, // "body"
	                * n2, // "utc"
	                * n3; // "abcorr"
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 3 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function lspcn_api_ap; expected 3, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if( ! n1->is_string() )
		{
		errs << "lspcn_api_ap: first argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n2->get_type() != DATA_TYPE::TIME )
		{
		errs << "lspcn_api_ap: second argument is not a time\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( ! n3->is_string() )
		{
		errs << "lspcn_api_ap: third argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}


	try {
	*result = lspcn_api( *n1->get_string(), n2->get_double(), *n3->get_string());
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	return RETURN_STATUS::SUCCESS ;
	}

RETURN_STATUS pxform_api_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1,  // "from_frame"
	            * n2, // "to_frame"
	            * n3; // "utc"

	double		R[3][3];

	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 3 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function pxform_api_ap; expected 3, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
    n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if( ! n1->is_string() )
		{
		errs << "pxform_api: first argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( ! n2->is_string()  )
		{
		errs << "pxform_api: second argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n3->get_type() != DATA_TYPE::TIME )
		{
		errs << "pxform_api: third argument is not a time\n" ;
		return RETURN_STATUS::FAIL ;
		}
	double n3_as_double = n3->get_time_or_duration().convert_to_double_use_with_caution();
	try {
	pxform_api( *n1->get_string(), *n2->get_string(), n3_as_double, R);
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	ListOVal*	returned_matrix = new ListOVal;
	
	for(int outer_index = 0; outer_index < 3; outer_index++) {
	    ListOVal*	returned_sub_array_list = new ListOVal;
	
	    for(int inner_index = 0; inner_index < 3; inner_index++) {
	        double		value = R[outer_index][inner_index];
		    returned_sub_array_list->add(inner_index, value);
	    }
		returned_matrix->add(outer_index, *returned_sub_array_list);
	}

	*result = *returned_matrix;

	return RETURN_STATUS::SUCCESS ;
}
	
	
RETURN_STATUS twovec_api_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1,  // "axdef[3]"
	            * n2, // "indexa"
	            * n3, // "plndef[3]"
	            * n4; // "indexp"

	double		R[3][3];
	double      axdef[3];
	double      plndef[3];
      
    	errs.undefine();
	if(ARG_POINTER != (WHERE_TO_STOP + 4)) {
		errs << "  wrong number of parameters passed to user-defined function twovec_api; expected 4, got "
			<< (ARG_POINTER - WHERE_TO_STOP) << "\n";
		return apgen::RETURN_STATUS::FAIL;
        }
	n4 = ARG_POP;
	n3 = ARG_POP;
	n2 = ARG_POP;
	n1 = ARG_POP;
	if(!n1->is_array()) {
		errs << "twovec_api error: first argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
        }
	ListOVal*	given_matrix = &n1->get_array();
	if(given_matrix->get_length() != 3) {
		errs << "twovec_api error: first argument expected given array to have 3 items in it\n";
		return RETURN_STATUS::FAIL;
        }
	ArrayElement*	given_matrix_element;
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "twovec_api twovec_api: expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
		}

		double	value = given_matrix_element->Val().get_double();
		axdef[index] = value;
	}	
		
	if( n2->get_type() != DATA_TYPE::INTEGER ) {
		errs << "twovec_api: second argument is not an integer \n";
		return RETURN_STATUS::FAIL;
        }

	if(!n3->is_array()) {
		errs << "twovec_api error: third argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
        }
	given_matrix = &n3->get_array();
	if(given_matrix->get_length() != 3) {
		errs << "twovec_api error: second argument expected given array to have 3 items in it\n";
		return RETURN_STATUS::FAIL;
        }
	for(int index = 0; index < given_matrix->get_length(); index++) {
	    given_matrix_element = (*given_matrix)[index];
	    if(!given_matrix_element->Val().is_numeric()) {
			errs << "twovec_api twovec_api: expected an array of doubles \n";
			return RETURN_STATUS::FAIL; 
	    }

	    double	value = given_matrix_element->Val().get_double();
	    plndef[index] = value;
	}
		
	if( n4->get_type() != DATA_TYPE::INTEGER ) {
		errs << "twovec_api: fourth argument is not an integer \n" ;
		return RETURN_STATUS::FAIL ;
	}
		
	
	try {
	twovec_api( axdef, n2->get_int(), plndef, n4->get_int(), R);
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	ListOVal*	returned_matrix = new ListOVal;
	
	for(int outer_index = 0; outer_index < 3; outer_index++) {

	    ListOVal*	returned_sub_array_list = new ListOVal;
	
	    for (int inner_index = 0; inner_index < 3; inner_index++) {
	        double		value = R[outer_index][inner_index];
			returned_sub_array_list->add(inner_index, value);
	    }
		returned_matrix->add(outer_index, *returned_sub_array_list);
	}

	*result = *returned_matrix;

	return RETURN_STATUS::SUCCESS ;
}

apgen::RETURN_STATUS vzfrac_api_ap (Cstring& errs, TypedValue* result, ARG_STACK) {
  TypedValue
    *n1, // frontId
    *n2, // frontFrame
    *n3, // frontRadii
    *n4, // backID
    *n5, // backFrame
    *n6, // backRadii
    *n7, // obsID
    *n8, // abcorr
    *n9; // time
  double frac;

  errs.undefine();
  if(ARG_POINTER != (WHERE_TO_STOP + 9)) {
    errs << "  wrong number of parameters passed to user-defined function vzfrac_api; expected 9, got "
	 << (ARG_POINTER - WHERE_TO_STOP) << "\n";
    return apgen::RETURN_STATUS::FAIL;
  }
  n9 = ARG_POP;
  n8 = ARG_POP;
  n7 = ARG_POP;
  n6 = ARG_POP;
  n5 = ARG_POP;
  n4 = ARG_POP;
  n3 = ARG_POP;
  n2 = ARG_POP;
  n1 = ARG_POP;
  if( n1->get_type() != DATA_TYPE::INTEGER ){
    errs << "vzfrac_api_ap: first argument is not a int\n" ;
    return RETURN_STATUS::FAIL ;
  }
  if( !n2->is_string() ){
    errs << "vzfrac_api_ap: second argument is not a string\n" ;
    return RETURN_STATUS::FAIL ;
  }
  if( !n3->is_array() ){
    errs << "vzfrac_api_ap: third argument is not an array\n" ;
    return RETURN_STATUS::FAIL ;
  }
  if( n4->get_type() != DATA_TYPE::INTEGER ){
    errs << "vzfrac_api_ap: fourth argument is not a int\n" ;
    return RETURN_STATUS::FAIL ;
  }
  if( !n5->is_string() ){
    errs << "vzfrac_api_ap: fifth argument is not a string\n" ;
    return RETURN_STATUS::FAIL ;
  }
  if( !n6->is_array() ){
    errs << "vzfrac_api_ap: sixth argument is not an array\n" ;
    return RETURN_STATUS::FAIL ;
  }
  if( n7->get_type() != DATA_TYPE::INTEGER ){
    errs << "vzfrac_api_ap: seventh argument is not a int\n" ;
    return RETURN_STATUS::FAIL ;
  }
  if( !n8->is_string() ){
    errs << "vzfrac_api_ap: eighth argument is not a string\n" ;
    return RETURN_STATUS::FAIL ;
  }
  if(  n9->get_type() != DATA_TYPE::TIME ){
    errs << "vzfrac_api_ap: ninth argument is not a time\n" ;
    return RETURN_STATUS::FAIL ;
  }
  // NOTE: first and second dim must agree with actual dimensions of the 3rd
  // and 6th arguments from APGen
  int		first_dim = 3;
  double	first_array[first_dim];
  int		second_dim = 3;
  double	second_array[second_dim];
  if(get_double_array(errs, *n3, first_array, first_dim) != apgen::RETURN_STATUS::SUCCESS) {
	return apgen::RETURN_STATUS::FAIL; }
  if(get_double_array(errs, *n6, second_array, second_dim) != apgen::RETURN_STATUS::SUCCESS) {
	return apgen::RETURN_STATUS::FAIL; }
  // need a new function from Pierre to do this...
  vzfrac_api(  n1->get_int(), *n2->get_string(), first_array, n4->get_int(), *n5->get_string(), second_array, n7->get_int(), *n8->get_string(), n9->get_double(), &frac );

  *result = frac;

  return apgen::RETURN_STATUS::SUCCESS;
}

apgen::RETURN_STATUS
m2q_api_ap (Cstring& errs, TypedValue* returnedval, ARG_STACK) {
	TypedValue*	n1; // incoming array of doubles
	double		R[3][3];
	double		q[4];

	errs.undefine();
	if(ARG_POINTER != (WHERE_TO_STOP + 1)) {
		errs << "  wrong number of parameters passed to user-defined function m2q_api; expected 1, got "
			<< (ARG_POINTER - WHERE_TO_STOP) << "\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	n1 = ARG_POP;
	if(!n1->is_array()) {
		errs << "m2q_api error: expected argument to be an array\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	ListOVal*	given_matrix = &n1->get_array();
	if(given_matrix->get_length() != 3) {
		errs << "m2q_api error: expected given array to have 3 items in it\n";
		return RETURN_STATUS::FAIL;
	}
	ArrayElement*					ae;

	for(int outer_index = 0; outer_index < given_matrix->get_length(); outer_index++) {
		ae = (*given_matrix)[outer_index];
		if(!ae->Val().is_array()) {
			errs << "m2q_api error: expected given array to contain arrays\n";
			return RETURN_STATUS::FAIL;
		}
		ListOVal&	given_sub_array(ae->Val().get_array());

		if(given_sub_array.get_length() != 3) {
			errs << "m2q_api error: expected given array to contain arrays of 3 elements each\n";
			return RETURN_STATUS::FAIL;
		}

		ArrayElement*					given_matrix_element;

		for(int inner_index = 0; inner_index < given_sub_array.get_length(); inner_index++) {
			given_matrix_element = given_sub_array[inner_index];
			if(!given_matrix_element->Val().is_numeric()) {
				errs << "m2q_api error: expected argument to be a 3 by 3 matrix of floats\n";
				return RETURN_STATUS::FAIL;
			}

			double		value = given_matrix_element->Val().get_double();
			R[outer_index][inner_index] = value;
		}

	}

	m2q_api(R, q);
	*returnedval = create_float_list(4, q);
	
	return apgen::RETURN_STATUS::SUCCESS;
}	
	
apgen::RETURN_STATUS
bodvrd_api_ap (Cstring& errs, TypedValue* returnedval, ARG_STACK) {
	TypedValue	* n1,  // "body_name"
	            * n2, // "item"
	            * n3; // "maxn"

	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 3 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function bodvrd_api; expected 3, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
    n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if( ! n1->is_string() )
		{
		errs << "bodvrd_api: first argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( ! n2->is_string()  )
		{
		errs << "bodvrd_api: second argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n3->get_type() != DATA_TYPE::INTEGER )
		{
		errs << "bodvrd_api: third argument is not an integer\n" ;
		return RETURN_STATUS::FAIL ;
		}

    int		dim;
    double  values[n3->get_int()];

	bodvrd_api(*n1->get_string(), *n2->get_string(), n3->get_int(), &dim, values);

	*returnedval = create_float_list(dim, values);

	return apgen::RETURN_STATUS::SUCCESS;
}


RETURN_STATUS sincpt_api_ap (Cstring& errs, TypedValue* res, ARG_STACK)
{
    TypedValue	* n1, // "method"
	            * n2, // "target"
	            * n3, // "et"
	            * n4, // "fixref"
	            * n5, // "abcorr"
	            * n6, // "obsrvr"
	            * n7, // "dref"
	            * n8; // "dvec"
    errs.undefine();
    if(ARG_POINTER != (WHERE_TO_STOP + 8)) {
       errs << "  wrong number of parameters passed to user-defined function sincpt_api; expected 8, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
        return RETURN_STATUS::FAIL;
    }
    n8 = ARG_POP ;
    n7 = ARG_POP ;
    n6 = ARG_POP ;
    n5 = ARG_POP ;
    n4 = ARG_POP ;
    n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if( ! n1->is_string() )
		{
		errs << "sincpt_api: first argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( ! n2->is_string()  )
		{
		errs << "sincpt_api: second argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n3->get_type() != DATA_TYPE::TIME )
		{
		errs << "sincpt_api: third argument is not a time \n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( ! n4->is_string()  )
		{
		errs << "sincpt_api: fourth argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( ! n5->is_string()  )
		{
		errs << "sincpt_api: fifth argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( ! n6->is_string()  )
		{
		errs << "sincpt_api: sixth argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( ! n7->is_string()  )
		{
		errs << "sincpt_api: seventh argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}

	else if( ! n8->is_array()) {
		errs << "sincpt_api error: eigth argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
	}

	double dvec[3];
	double spoint[3] = {0.0,0.0,0.0};
    double trgepc = 0.0;
    double srfvec[3] = {0.0,0.0,0.0};
    int found;

	ListOVal* given_matrix = &n8->get_array();
	if(given_matrix->get_length() != 3) {
		errs << "sincpt_api error: eigth argument expected given array to have 3 items in it\n";
		return RETURN_STATUS::FAIL;
	}
	ArrayElement* given_matrix_element;
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "sincpt_api: eigth argument expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
		}
        	double value = given_matrix_element->Val().get_double();
		dvec[index] = value;
	}	
    
       
	try {
    sincpt_api( *n1->get_string(),
                *n2->get_string(),
                n3->get_double(),
                *n4->get_string(),
                *n5->get_string(),
                *n6->get_string(),
                *n7->get_string(),
                dvec,
                spoint,
                &trgepc,
                srfvec,
                &found);
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}

    ListOVal* top_level_array = new ListOVal;
    ArrayElement* ae;
    ArrayElement* lower_ae;
    TypedValue val;

    /* Generate the following structure
    [“spoint” = [0.0,0.0,0.0], “trgepc” = 0.0, “srfvec” = [0.0,0.0,0.0], “found” = TRUE]
    */


    // now let's define the lower-level array
    ListOVal* lower_level_array = new ListOVal;
    lower_level_array->add(0, spoint[0]);
    lower_level_array->add(1, spoint[1]);
	lower_level_array->add(2, spoint[2]);

    // go back to upper-level array
	top_level_array->add("spoint", *lower_level_array);

    // now for the second element
	top_level_array->add("trgepc", trgepc);

    // now for the third element
    lower_level_array = new ListOVal;

    // we want a list
	lower_level_array->add(0, srfvec[0]);
	lower_level_array->add(1, srfvec[1]);
	lower_level_array->add(2, srfvec[2]);

    // go back to the upper-level array
	top_level_array->add("srfvec", *lower_level_array);

    // now for the fourth element
    bool bool_val = (found != 0);
	top_level_array->add("found", bool_val);

    // return the top-level array as the result of the function call
    *res = *top_level_array;
    return RETURN_STATUS::SUCCESS;
}


RETURN_STATUS subpnt_api_ap (Cstring& errs, TypedValue* res, ARG_STACK)
{
    TypedValue	* n1, // "method"
	            * n2, // "target"
	            * n3, // "et"
	            * n4, // "fixref"
	            * n5, // "abcorr"
	            * n6; // "obsrvr"
    errs.undefine();
    if(ARG_POINTER != (WHERE_TO_STOP + 6)) {
       errs << "  wrong number of parameters passed to user-defined function sincpt_api; expected 6, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
        return RETURN_STATUS::FAIL;
    }
    n6 = ARG_POP ;
    n5 = ARG_POP ;
    n4 = ARG_POP ;
    n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if( ! n1->is_string() )
		{
		errs << "sincpt_api: first argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( ! n2->is_string()  )
		{
		errs << "sincpt_api: second argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n3->get_type() != DATA_TYPE::TIME )
		{
		errs << "sincpt_api: third argument is not a time \n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( ! n4->is_string()  )
		{
		errs << "sincpt_api: fourth argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( ! n5->is_string()  )
		{
		errs << "sincpt_api: fifth argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( ! n6->is_string()  )
		{
		errs << "sincpt_api: sixth argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}

	double spoint[3] = {0.0,0.0,0.0};
    double trgepc = 0.0;
    double srfvec[3] = {0.0,0.0,0.0};

	try {
    subpnt_api( *n1->get_string(),
                *n2->get_string(),
                n3->get_double(),
                *n4->get_string(),
                *n5->get_string(),
                *n6->get_string(),
                spoint,
                &trgepc,
                srfvec );
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}

    ListOVal* top_level_array = new ListOVal;
    ArrayElement* ae;
    ArrayElement* lower_ae;
    TypedValue val;

    /* Generate the following structure
    [“spoint” = [0.0,0.0,0.0], “trgepc” = 0.0, “srfvec” = [0.0,0.0,0.0] ]
    */


    // now let's define the lower-level array
    ListOVal* lower_level_array = new ListOVal;
    lower_level_array->add(0, spoint[0]);
    lower_level_array->add(1, spoint[1]);
    lower_level_array->add(2, spoint[2]);

    // go back to upper-level array
    top_level_array->add("spoint", *lower_level_array);

    // now for the second element
    top_level_array->add("trgepc", trgepc);

    // now for the third element
    lower_level_array = new ListOVal;

    // we want a list
    lower_level_array->add(0, srfvec[0]);
    lower_level_array->add(1, srfvec[1]);
    lower_level_array->add(2, srfvec[2]);

    // go back to the upper-level array
    top_level_array->add("srfvec", *lower_level_array);

    // return the top-level array as the result of the function call
    *res = *top_level_array;
    return RETURN_STATUS::SUCCESS;
}



RETURN_STATUS ilumin_api_ap (Cstring& errs, TypedValue* res, ARG_STACK)
{
    TypedValue	* n1, // "method"
	            * n2, // "target"
	            * n3, // "et"
	            * n4, // "fixref"
	            * n5, // "abcorr"
	            * n6, // "obsrvr"
	            * n7; // "spoint"

    errs.undefine();
    if(ARG_POINTER != (WHERE_TO_STOP + 7)) {
       errs << "  wrong number of parameters passed to user-defined function ilumin_api; expected 7, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
        return RETURN_STATUS::FAIL;
    }
    n7 = ARG_POP ;
    n6 = ARG_POP ;
    n5 = ARG_POP ;
    n4 = ARG_POP ;
    n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if( ! n1->is_string() )
		{
		errs << "ilumin_api: first argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( ! n2->is_string()  )
		{
		errs << "ilumin_api: second argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n3->get_type() != DATA_TYPE::TIME )
		{
		errs << "ilumin_api: third argument is not a time \n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( ! n4->is_string()  )
		{
		errs << "ilumin_api: fourth argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( ! n5->is_string()  )
		{
		errs << "ilumin_api: fifth argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( ! n6->is_string()  )
		{
		errs << "ilumin_api: sixth argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( ! n7->is_array()) {
		errs << "ilumin_api error: seventh argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
	}

	double spoint[3];
    double trgepc = 0.0;
    double srfvec[3] = {0.0,0.0,0.0};
    double phase = 0.0;
    double solar = 0.0;
    double emissn = 0.0;

	ListOVal* given_matrix = &n7->get_array();
	if(given_matrix->get_length() != 3) {
		errs << "ilumin_api error: seventh argument expected given array to have 3 items in it\n";
		return RETURN_STATUS::FAIL;
	}
	ArrayElement* given_matrix_element;
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "ilumin_api: seventh argument expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
		}
        	double value = given_matrix_element->Val().get_double();
		spoint[index] = value;
	}
    
       
	try {
    ilumin_api( *n1->get_string(),
                *n2->get_string(),
                n3->get_double(),
                *n4->get_string(),
                *n5->get_string(),
                *n6->get_string(),
                spoint,
                &trgepc,
                srfvec,
                &phase,
                &solar,
                &emissn);
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}

    ListOVal* top_level_array = new ListOVal;

    /* Generate the following structure
    [“trgepc” = 0.0, “srfvec” = [0.0,0.0,0.0], “phase” = 0.0, "solar" = 0.0, emissn = 0.0]
    */


    // first element
    top_level_array->add("trgepc", trgepc);

    // now for the second element
    ListOVal* lower_level_array = new ListOVal;

    // we want a list
    lower_level_array->add(0, srfvec[0]);
    lower_level_array->add(1, srfvec[1]);
    lower_level_array->add(2, srfvec[2]);

    // go back to the upper-level array
    top_level_array->add("srfvec", *lower_level_array);

    // third element
    top_level_array->add("phase", phase);

    // fourth element
    top_level_array->add("solar", solar);

    // fifth element
    top_level_array->add("emissn", emissn);

    // return the top-level array as the result of the function call
    *res = *top_level_array;
    return RETURN_STATUS::SUCCESS;
}

RETURN_STATUS et2lst_api_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
{
	TypedValue	* n1,  // "utc"
	            * n2,  // "body"
	            * n3,  // "lon"
	            * n4;  // "type"
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 4 ) )
	{
		errs << "  wrong number of parameters passed to user-defined function et2lst_api; expected 4, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
	}
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(  n1->get_type() != DATA_TYPE::TIME )
		{
		errs << "et2lst_api: first argument is not a time\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n2->is_string() )
		{
		errs << "et2lst_api: second argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n3->is_numeric() )
		{
		errs << "et2lst_api: third argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n4->is_string() )
		{
		errs << "et2lst_api: fourth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}

	try {
	*result = et2lst_api(
	               n1->get_double(),
			       (char *)*n2->get_string(),
			       n3->get_double(),
			       (char *)*n4->get_string() );
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}

	return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS gfdist_api_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
{
	TypedValue	* n1,  // "target"
	            * n2,  // "abcorr"
	            * n3,  // "obsrvr"
	            * n4,  // "relate"
	            * n5, // "refval"
	            * n6, // "adjust"
	            * n7, // "step"
	            * n8, // "nintvls"
	            * n9; // "cnfine"
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 9 ) )
	{
		errs << "  wrong number of parameters passed to user-defined function gfdist_api; expected 9, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
	}
	n9 = ARG_POP ;
	n8 = ARG_POP ;
	n7 = ARG_POP ;
	n6 = ARG_POP ;
	n5 = ARG_POP ;
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(  ! n1->is_string() )
		{
		errs << "gfdist_api: first argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n2->is_string() )
		{
		errs << "gfdist_api: second argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n3->is_string() )
		{
		errs << "gfdist_api: third argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n4->is_string() )
		{
		errs << "gfdist_api: fourth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n5->is_numeric() )
		{
		errs << "gfdist_api: fifth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n6->is_numeric() )
		{
		errs << "gfdist_api: sixth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n7->is_numeric() )
		{
		errs << "gfdist_api: seventh argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( n8->get_type() != DATA_TYPE::INTEGER )
		{
		errs << "gfdist_api: eighth argument is not a integer\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n9->is_array()) {
		errs << "gfdist_api error: ninth argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
	}

	double cnfine[2];

	ListOVal* given_matrix = &n9->get_array();
	if(given_matrix->get_length() != 2) {
		errs << "gfdist_api error: ninth argument expected given array to have 2 items in it\n";
		return RETURN_STATUS::FAIL;
	}
	ArrayElement* given_matrix_element;
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(given_matrix_element->Val().get_type() != DATA_TYPE::TIME ) {
			errs << "gfdist_api: ninth argument expected an array of times \n";
			return RETURN_STATUS::FAIL;
		}
        	double value = given_matrix_element->Val().get_double();
		cnfine[index] = value;
	}		
			
	int nintvls = n8->get_int();	

	double windows[2*nintvls];


	int numWindows = 0;
	try {
	gfdist_api(
	               (char *)*n1->get_string(),
			       (char *)*n2->get_string(),
			       (char *)*n3->get_string(),
			       (char *)*n4->get_string(),
			       n5->get_double(),
			       n6->get_double(),
                   n7->get_double(),
                   nintvls,
                   cnfine,
			       windows,
			       &numWindows );
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	*result = create_float_list(2*numWindows, windows);
	return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS gfsep_api_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
{
	TypedValue	* n1,  // "targ1"
	            * n2,  // "shape1"
	            * n3,  // "frame1"
	            * n4,  // "targ2"
	            * n5,  // "shape2"
	            * n6,  // "frame2"
	            * n7,  // "abcorr"
	            * n8,  // "obsrvr"
	            * n9,  // "relate"
	            * n10, // "refval"
	            * n11, // "adjust"
	            * n12, // "step"
	            * n13, // "nintvls"
	            * n14; // "cnfine"
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 14 ) )
	{
		errs << "  wrong number of parameters passed to user-defined function gfsep_api; expected 14, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
	}
	n14 = ARG_POP ;
	n13 = ARG_POP ;
	n12 = ARG_POP ;
	n11 = ARG_POP ;
	n10 = ARG_POP ;
	n9 = ARG_POP ;
	n8 = ARG_POP ;
	n7 = ARG_POP ;
	n6 = ARG_POP ;
	n5 = ARG_POP ;
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(  ! n1->is_string() )
		{
		errs << "gfsep_api: first argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n2->is_string() )
		{
		errs << "gfsep_api: second argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n3->is_string() )
		{
		errs << "gfsep_api: third argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n4->is_string() )
		{
		errs << "gfsep_api: fourth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n5->is_string() )
		{
		errs << "gfsep_api: fifth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n6->is_string() )
		{
		errs << "gfsep_api: sixth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n7->is_string() )
		{
		errs << "gfsep_api: seventh argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n8->is_string() )
		{
		errs << "gfsep_api: eighth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n9->is_string() )
		{
		errs << "gfsep_api: ninth argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n10->is_numeric() )
		{
		errs << "gfsep_api: tenth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n11->is_numeric() )
		{
		errs << "gfsep_api: eleventh argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n12->is_numeric() )
		{
		errs << "gfsep_api: twelfth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( n13->get_type() != DATA_TYPE::INTEGER )
		{
		errs << "gfsep_api: thirteenth argument is not a integer\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n14->is_array()) {
		errs << "gfsep_api error: fourteenth argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
	}

	double cnfine[2];

	ListOVal* given_matrix = &n14->get_array();
	if(given_matrix->get_length() != 2) {
		errs << "gfsep_api error: fourteenth argument expected given array to have 2 items in it\n";
		return RETURN_STATUS::FAIL;
	}
	ArrayElement* given_matrix_element;
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(given_matrix_element->Val().get_type() != DATA_TYPE::TIME) {
			errs << "gfsep_api: fourteenth argument expected an array of times \n";
			return RETURN_STATUS::FAIL;
		}
        	double value = given_matrix_element->Val().get_double();
		cnfine[index] = value;
	}		
			
	int nintvls = n13->get_int();	
	double windows[2*nintvls];


	int numWindows = 0;
	try {
	gfsep_api(
	               (char *)*n1->get_string(), //targ1
			       (char *)*n2->get_string(), //shape1
			       (char *)*n3->get_string(), //frame1
			       (char *)*n4->get_string(), //targ2
			       (char *)*n5->get_string(), //shape2
			       (char *)*n6->get_string(), //frame2
			       (char *)*n7->get_string(), //abcorr
			       (char *)*n8->get_string(), //obsrvr
			       (char *)*n9->get_string(), //relate
			       n10->get_double(), //refval
			       n11->get_double(), //adjust
                   n12->get_double(), //step
                   nintvls, //nintvls
                   cnfine,
			       windows,
			       &numWindows );
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	*result = create_float_list(2*numWindows, windows);
	return RETURN_STATUS::SUCCESS ;
}


RETURN_STATUS gfposc_api_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
{
	TypedValue	* n1,  // "target"
	            * n2,  // "frame"
	            * n3,  // "abcorr"
	            * n4,  // "obsrvr"
	            * n5,  // "crdsys"
	            * n6,  // "coord"
	            * n7,  // "relate"
	            * n8, // "refval"
	            * n9, // "adjust"
	            * n10, // "step"
	            * n11, // "nintvls"
	            * n12; // "cnfine"
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 12 ) )
	{
		errs << "  wrong number of parameters passed to user-defined function gfposc_api; expected 12, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
	}
	n12 = ARG_POP ;
	n11 = ARG_POP ;
	n10 = ARG_POP ;
	n9 = ARG_POP ;
	n8 = ARG_POP ;
	n7 = ARG_POP ;
	n6 = ARG_POP ;
	n5 = ARG_POP ;
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(  ! n1->is_string() )
		{
		errs << "gfposc_api: first argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n2->is_string() )
		{
		errs << "gfposc_api: second argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n3->is_string() )
		{
		errs << "gfposc_api: third argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n4->is_string() )
		{
		errs << "gfposc_api: fourth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n5->is_string() )
		{
		errs << "gfposc_api: fifth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n6->is_string() )
		{
		errs << "gfposc_api: sixth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n7->is_string() )
		{
		errs << "gfposc_api: seventh argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n8->is_numeric() )
		{
		errs << "gfposc_api: eighth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n9->is_numeric() )
		{
		errs << "gfposc_api: ninth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n10->is_numeric() )
		{
		errs << "gfposc_api: tenth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( n11->get_type() != DATA_TYPE::INTEGER )
		{
		errs << "gfposc_api: eleventh argument is not a integer\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n12->is_array()) {
		errs << "gfposc_api error: twelfth argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
	}

	double cnfine[2];

	ListOVal* given_matrix = &n12->get_array();
	if(given_matrix->get_length() != 2) {
		errs << "gfposc_api error: twelfth argument expected given array to have 2 items in it\n";
		return RETURN_STATUS::FAIL;
	}
	ArrayElement* given_matrix_element;
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(given_matrix_element->Val().get_type() != DATA_TYPE::TIME ) {
			errs << "gfposc_api: twelfth argument expected an array of times \n";
			return RETURN_STATUS::FAIL;
		}
        	double value = given_matrix_element->Val().get_double();
		cnfine[index] = value;
	}		
			
	int nintvls = n11->get_int();	
	double windows[2*nintvls];


	int numWindows = 0;
	try {
	gfposc_api(
	               (char *)*n1->get_string(),
			       (char *)*n2->get_string(),
			       (char *)*n3->get_string(),
			       (char *)*n4->get_string(),
			       (char *)*n5->get_string(),
			       (char *)*n6->get_string(),
			       (char *)*n7->get_string(),
			       n8->get_double(),
			       n9->get_double(),
                   n10->get_double(),
                   nintvls,
                   cnfine,
			       windows,
			       &numWindows );
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	*result = create_float_list(2*numWindows, windows);
	return RETURN_STATUS::SUCCESS ;
}





RETURN_STATUS gfilum_api_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
{
	TypedValue	* n1,  // "method"
	            * n2,  // "angtyp"
	            * n3,  // "target"
	            * n4,  // "illmn"
	            * n5,  // "fixref"
	            * n6,  // "abcorr"
	            * n7,  // "obsrvr"
	            * n8,  // "spoint"
	            * n9,  // "relate"
	            * n10, // "refval"
	            * n11, // "adjust"
	            * n12, // "step"
	            * n13, // "nintvls"
	            * n14; // "cnfine"
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 14 ) )
	{
		errs << "  wrong number of parameters passed to user-defined function gfpa_api; expected 14, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
	}
	n14 = ARG_POP ;
	n13 = ARG_POP ;
	n12 = ARG_POP ;
	n11 = ARG_POP ;
	n10 = ARG_POP ;
	n9 = ARG_POP ;
	n8 = ARG_POP ;
	n7 = ARG_POP ;
	n6 = ARG_POP ;
	n5 = ARG_POP ;
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(  ! n1->is_string() )
		{
		errs << "gfilum_api: first argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if(  ! n2->is_string() )
		{
		errs << "gfilum_api: second argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if(  ! n3->is_string() )
		{
		errs << "gfilum_api: third argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if(  ! n4->is_string() )
		{
		errs << "gfilum_api: fourth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n5->is_string() )
		{
		errs << "gfilum_api: fifth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n6->is_string() )
		{
		errs << "gfilum_api: sixth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n7->is_string() )
		{
		errs << "gfilum_api: seventh argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n8->is_array()) {
		errs << "gfilum_api: eighth argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
	}
	if( ! n9->is_string() )
		{
		errs << "gfilum_api: ninth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n10->is_numeric() )
		{
		errs << "gfilum_api: tenth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n11->is_numeric() )
		{
		errs << "gfilum_api: eleventh argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n12->is_numeric() )
		{
		errs << "gfilum_api: twelfth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( n13->get_type() != DATA_TYPE::INTEGER )
		{
		errs << "gfilum_api: thirteenth argument is not a integer\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n14->is_array()) {
		errs << "gfilum_api: fourteenth argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
	}


	double spoint[3];
	ListOVal* given_matrix = &n8->get_array();
	if(given_matrix->get_length() != 3) {
		errs << "ilumin_api error: eighth argument expected given array to have 3 items in it\n";
		return RETURN_STATUS::FAIL;
	}
	ArrayElement* given_matrix_element;
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(!given_matrix_element->Val().is_numeric()) {
			errs << "ilumin_api: eighth argument expected an array of doubles \n";
			return RETURN_STATUS::FAIL;
		}
        	double value = given_matrix_element->Val().get_double();
		spoint[index] = value;
	}	
	
	
	double cnfine[2];
	given_matrix = &n14->get_array();
	if(given_matrix->get_length() != 2) {
		errs << "gfdist_api error: fourteenth argument expected given array to have 2 items in it\n";
		return RETURN_STATUS::FAIL;
	}
	ArrayElement* given_matrix_element2;
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element2 = (*given_matrix)[index];
		if(given_matrix_element2->Val().get_type() != DATA_TYPE::TIME ) {
			errs << "gfdist_api: fourteenth argument expected an array of times \n";
			return RETURN_STATUS::FAIL;
		}
        	double value = given_matrix_element2->Val().get_double();
		cnfine[index] = value;
	}		
			
	int nintvls = n13->get_int();	
	double windows[2*nintvls];


	int numWindows = 0;
	try {
	gfilum_api(
	               (char *)*n1->get_string(),
			       (char *)*n2->get_string(),
			       (char *)*n3->get_string(),
			       (char *)*n4->get_string(),
			       (char *)*n5->get_string(),
			       (char *)*n6->get_string(),
			       (char *)*n7->get_string(),
			       spoint,
			       (char *)*n9->get_string(),
			       n10->get_double(),
			       n11->get_double(),
                   n12->get_double(),
                   nintvls,
                   cnfine,
			       windows,
			       &numWindows );
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	*result = create_float_list(2*numWindows, windows);
	return RETURN_STATUS::SUCCESS ;
}


RETURN_STATUS gfpa_api_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
{
	TypedValue	* n1,  // "target"
	            * n2,  // "illmn"
	            * n3,  // "abcorr"
	            * n4,  // "obsrvr"
	            * n5,  // "relate"
	            * n6, // "refval"
	            * n7, // "adjust"
	            * n8, // "step"
	            * n9, // "nintvls"
	            * n10; // "cnfine"
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 10 ) )
	{
		errs << "  wrong number of parameters passed to user-defined function gfpa_api; expected 10, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
	}
	n10 = ARG_POP ;
	n9 = ARG_POP ;
	n8 = ARG_POP ;
	n7 = ARG_POP ;
	n6 = ARG_POP ;
	n5 = ARG_POP ;
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(  ! n1->is_string() )
		{
		errs << "gfpa_api: first argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n2->is_string() )
		{
		errs << "gfpa_api: second argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n3->is_string() )
		{
		errs << "gfpa_api: third argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n4->is_string() )
		{
		errs << "gfpa_api: fourth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n5->is_string() )
		{
		errs << "gfpa_api: fifth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n6->is_numeric() )
		{
		errs << "gfpa_api: sixth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n7->is_numeric() )
		{
		errs << "gfpa_api: seventh argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n8->is_numeric() )
		{
		errs << "gfpa_api: eighth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( n9->get_type() != DATA_TYPE::INTEGER )
		{
		errs << "gfpa_api: ninth argument is not a integer\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n10->is_array()) {
		errs << "gfpa_api error: tenth argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
	}

	double cnfine[2];

	ListOVal* given_matrix = &n10->get_array();
	if(given_matrix->get_length() != 2) {
		errs << "gfdist_api error: tenth argument expected given array to have 2 items in it\n";
		return RETURN_STATUS::FAIL;
	}
	ArrayElement* given_matrix_element;
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(given_matrix_element->Val().get_type() != DATA_TYPE::TIME ) {
			errs << "gfdist_api: tenth argument expected an array of times \n";
			return RETURN_STATUS::FAIL;
		}
        	double value = given_matrix_element->Val().get_double();
		cnfine[index] = value;
	}		
			
	int nintvls = n9->get_int();	
	double windows[2*nintvls];


	int numWindows = 0;
	try {
	gfpa_api(
	               (char *)*n1->get_string(),
			       (char *)*n2->get_string(),
			       (char *)*n3->get_string(),
			       (char *)*n4->get_string(),
			       (char *)*n5->get_string(),
			       n6->get_double(),
			       n7->get_double(),
                   n8->get_double(),
                   nintvls,
                   cnfine,
			       windows,
			       &numWindows );
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	*result = create_float_list(2*numWindows, windows);
	return RETURN_STATUS::SUCCESS ;
}


RETURN_STATUS gflst_api_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
{
	TypedValue	* n1,  // "target"
	            * n2,  // "fixref"
	            * n3,  // "abcorr"
	            * n4,  // "obsrvr"
	            * n5,  // "relate"
	            * n6, // "refval"
	            * n7, // "adjust"
	            * n8, // "step"
	            * n9, // "nintvls"
	            * n10; // "cnfine"
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 10 ) )
	{
		errs << "  wrong number of parameters passed to user-defined function gflst_api; expected 10, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
	}
	n10 = ARG_POP ;
	n9 = ARG_POP ;
	n8 = ARG_POP ;
	n7 = ARG_POP ;
	n6 = ARG_POP ;
	n5 = ARG_POP ;
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(  ! n1->is_string() )
		{
		errs << "gflst_api: first argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n2->is_string() )
		{
		errs << "gflst_api: second argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n3->is_string() )
		{
		errs << "gflst_api: third argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n4->is_string() )
		{
		errs << "gflst_api: fourth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n5->is_string() )
		{
		errs << "gflst_api: fifth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n6->is_numeric() )
		{
		errs << "gflst_api: sixth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n7->is_numeric() )
		{
		errs << "gflst_api: seventh argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n8->is_numeric() )
		{
		errs << "gflst_api: eighth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( n9->get_type() != DATA_TYPE::INTEGER )
		{
		errs << "gflst_api: ninth argument is not a integer\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n10->is_array()) {
		errs << "gflst_api: tenth argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
	}

	double cnfine[2];

	ListOVal* given_matrix = &n10->get_array();
	if(given_matrix->get_length() != 2) {
		errs << "gflst_api error: tenth argument expected given array to have 2 items in it\n";
		return RETURN_STATUS::FAIL;
	}
	ArrayElement* given_matrix_element;
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(given_matrix_element->Val().get_type() != DATA_TYPE::TIME ) {
			errs << "gflst_api: tenth argument expected an array of times \n";
			return RETURN_STATUS::FAIL;
		}
        	double value = given_matrix_element->Val().get_double();
		cnfine[index] = value;
	}		
			
	int nintvls = n9->get_int();	
	double windows[2*nintvls];


	int numWindows = 0;
	try {
	gflst_api(  (char *)*n1->get_string(),
	            (char *)*n2->get_string(),
                (char *)*n3->get_string(),
	            (char *)*n4->get_string(),
	            (char *)*n5->get_string(),
			    n6->get_double(),
			    n7->get_double(),
                n8->get_double(),
                nintvls,
                cnfine,
			    windows,
			    &numWindows );
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	*result = create_float_list(2*numWindows, windows);
	return RETURN_STATUS::SUCCESS ;
}


RETURN_STATUS gfemi_api_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
{
	TypedValue	* n1,  // "target"
	            * n2,  // "fixref"
	            * n3,  // "abcorr"
	            * n4,  // "obsrvr"
	            * n5,  // "relate"
	            * n6, // "refval"
	            * n7, // "adjust"
	            * n8, // "step"
	            * n9, // "nintvls"
	            * n10; // "cnfine"
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 10 ) )
	{
		errs << "  wrong number of parameters passed to user-defined function gfemi_api; expected 10, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
	}
	n10 = ARG_POP ;
	n9 = ARG_POP ;
	n8 = ARG_POP ;
	n7 = ARG_POP ;
	n6 = ARG_POP ;
	n5 = ARG_POP ;
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(  ! n1->is_string() )
		{
		errs << "gfemi_api: first argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n2->is_string() )
		{
		errs << "gfemi_api: second argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n3->is_string() )
		{
		errs << "gfemi_api: third argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n4->is_string() )
		{
		errs << "gfemi_api: fourth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n5->is_string() )
		{
		errs << "gfemi_api: fifth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n6->is_numeric() )
		{
		errs << "gfemi_api: sixth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n7->is_numeric() )
		{
		errs << "gfemi_api: seventh argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n8->is_numeric() )
		{
		errs << "gfemi_api: eighth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( n9->get_type() != DATA_TYPE::INTEGER )
		{
		errs << "gfemi_api: ninth argument is not a integer\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n10->is_array()) {
		errs << "gfemi_api: tenth argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
	}

	double cnfine[2];

	ListOVal* given_matrix = &n10->get_array();
	if(given_matrix->get_length() != 2) {
		errs << "gfemi_api error: tenth argument expected given array to have 2 items in it\n";
		return RETURN_STATUS::FAIL;
	}
	ArrayElement* given_matrix_element;
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(given_matrix_element->Val().get_type() != DATA_TYPE::TIME ) {
			errs << "gfemi_api: tenth argument expected an array of times \n";
			return RETURN_STATUS::FAIL;
		}
        	double value = given_matrix_element->Val().get_double();
		cnfine[index] = value;
	}		
			
	int nintvls = n9->get_int();	
	double windows[2*nintvls];


	int numWindows = 0;
	try {
	gfemi_api(  (char *)*n1->get_string(),
	            (char *)*n2->get_string(),
                (char *)*n3->get_string(),
	            (char *)*n4->get_string(),
	            (char *)*n5->get_string(),
			    n6->get_double(),
			    n7->get_double(),
                n8->get_double(),
                nintvls,
                cnfine,
			    windows,
			    &numWindows );
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	*result = create_float_list(2*numWindows, windows);
	return RETURN_STATUS::SUCCESS ;
}


RETURN_STATUS gfinc_api_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
{
	TypedValue	* n1,  // "target"
	            * n2,  // "fixref"
	            * n3,  // "abcorr"
	            * n4,  // "obsrvr"
	            * n5,  // "relate"
	            * n6, // "refval"
	            * n7, // "adjust"
	            * n8, // "step"
	            * n9, // "nintvls"
	            * n10; // "cnfine"
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 10 ) )
	{
		errs << "  wrong number of parameters passed to user-defined function gfinc_api; expected 10, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
	}
	n10 = ARG_POP ;
	n9 = ARG_POP ;
	n8 = ARG_POP ;
	n7 = ARG_POP ;
	n6 = ARG_POP ;
	n5 = ARG_POP ;
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(  ! n1->is_string() )
		{
		errs << "gfinc_api: first argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n2->is_string() )
		{
		errs << "gfinc_api: second argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n3->is_string() )
		{
		errs << "gfinc_api: third argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n4->is_string() )
		{
		errs << "gfinc_api: fourth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n5->is_string() )
		{
		errs << "gfinc_api: fifth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n6->is_numeric() )
		{
		errs << "gfinc_api: sixth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n7->is_numeric() )
		{
		errs << "gfinc_api: seventh argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n8->is_numeric() )
		{
		errs << "gfinc_api: eighth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( n9->get_type() != DATA_TYPE::INTEGER )
		{
		errs << "gfinc_api: ninth argument is not a integer\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n10->is_array()) {
		errs << "gfinc_api: tenth argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
	}

	double cnfine[2];

	ListOVal* given_matrix = &n10->get_array();
	if(given_matrix->get_length() != 2) {
		errs << "gfinc_api error: tenth argument expected given array to have 2 items in it\n";
		return RETURN_STATUS::FAIL;
	}
	ArrayElement* given_matrix_element;
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(given_matrix_element->Val().get_type() != DATA_TYPE::TIME ) {
			errs << "gfinc_api: tenth argument expected an array of times \n";
			return RETURN_STATUS::FAIL;
		}
        	double value = given_matrix_element->Val().get_double();
		cnfine[index] = value;
	}		
			
	int nintvls = n9->get_int();	
	double windows[2*nintvls];


	int numWindows = 0;
	try {
	gfinc_api(  (char *)*n1->get_string(),
	            (char *)*n2->get_string(),
                (char *)*n3->get_string(),
	            (char *)*n4->get_string(),
	            (char *)*n5->get_string(),
			    n6->get_double(),
			    n7->get_double(),
                n8->get_double(),
                nintvls,
                cnfine,
			    windows,
			    &numWindows );
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	*result = create_float_list(2*numWindows, windows);
	return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS gflllon_api_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
{
	TypedValue	* n1,  // "target"
	            * n2,  // "fixref"
	            * n3,  // "abcorr"
	            * n4,  // "obsrvr"
	            * n5,  // "relate"
	            * n6, // "refval"
	            * n7, // "adjust"
	            * n8, // "step"
	            * n9, // "nintvls"
	            * n10; // "cnfine"
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 10 ) )
	{
		errs << "  wrong number of parameters passed to user-defined function gflllon_api; expected 10, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
	}
	n10 = ARG_POP ;
	n9 = ARG_POP ;
	n8 = ARG_POP ;
	n7 = ARG_POP ;
	n6 = ARG_POP ;
	n5 = ARG_POP ;
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(  ! n1->is_string() )
		{
		errs << "gflllon_api: first argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n2->is_string() )
		{
		errs << "gflllon_api: second argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n3->is_string() )
		{
		errs << "gflllon_api: third argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n4->is_string() )
		{
		errs << "gflllon_api: fourth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n5->is_string() )
		{
		errs << "gflllon_api: fifth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n6->is_numeric() )
		{
		errs << "gflllon_api: sixth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n7->is_numeric() )
		{
		errs << "gflllon_api: seventh argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n8->is_numeric() )
		{
		errs << "gflllon_api: eighth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( n9->get_type() != DATA_TYPE::INTEGER )
		{
		errs << "gflllon_api: ninth argument is not a integer\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n10->is_array()) {
		errs << "gflllon_api: tenth argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
	}

	double cnfine[2];

	ListOVal* given_matrix = &n10->get_array();
	if(given_matrix->get_length() != 2) {
		errs << "gflllon_api error: tenth argument expected given array to have 2 items in it\n";
		return RETURN_STATUS::FAIL;
	}
	ArrayElement* given_matrix_element;
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(given_matrix_element->Val().get_type() != DATA_TYPE::TIME ) {
			errs << "gflllon_api: tenth argument expected an array of times \n";
			return RETURN_STATUS::FAIL;
		}
        	double value = given_matrix_element->Val().get_double();
		cnfine[index] = value;
	}		
			
	int nintvls = n9->get_int();	
	double windows[2*nintvls];


	int numWindows = 0;
	try {
	gflllon_api(  (char *)*n1->get_string(),
	            (char *)*n2->get_string(),
                (char *)*n3->get_string(),
	            (char *)*n4->get_string(),
	            (char *)*n5->get_string(),
			    n6->get_double(),
			    n7->get_double(),
                n8->get_double(),
                nintvls,
                cnfine,
			    windows,
			    &numWindows );
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	*result = create_float_list(2*numWindows, windows);
	return RETURN_STATUS::SUCCESS ;
}


RETURN_STATUS gftermlon_api_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
{
	TypedValue	* n1,  // "target"
	            * n2,  // "fixref"
	            * n3,  // "abcorr"
	            * n4,  // "obsrvr"
	            * n5,  // "relate"
	            * n6, // "refval"
	            * n7, // "adjust"
	            * n8, // "step"
	            * n9, // "nintvls"
	            * n10; // "cnfine"
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 10 ) )
	{
		errs << "  wrong number of parameters passed to user-defined function gftermlon_api; expected 10, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
	}
	n10 = ARG_POP ;
	n9 = ARG_POP ;
	n8 = ARG_POP ;
	n7 = ARG_POP ;
	n6 = ARG_POP ;
	n5 = ARG_POP ;
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(  ! n1->is_string() )
		{
		errs << "gftermlon_api: first argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n2->is_string() )
		{
		errs << "gftermlon_api: second argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n3->is_string() )
		{
		errs << "gftermlon_api: third argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n4->is_string() )
		{
		errs << "gftermlon_api: fourth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n5->is_string() )
		{
		errs << "gftermlon_api: fifth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n6->is_numeric() )
		{
		errs << "gftermlon_api: sixth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n7->is_numeric() )
		{
		errs << "gfternlon_api: seventh argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( !n8->is_numeric() )
		{
		errs << "gftermlon_api: eighth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( n9->get_type() != DATA_TYPE::INTEGER )
		{
		errs << "gftermlon_api: ninth argument is not a integer\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n10->is_array()) {
		errs << "gftermlon_api: tenth argument is not an array\n";
		return apgen::RETURN_STATUS::FAIL;
	}

	double cnfine[2];

	ListOVal* given_matrix = &n10->get_array();
	if(given_matrix->get_length() != 2) {
		errs << "gftermlon_api error: tenth argument expected given array to have 2 items in it\n";
		return RETURN_STATUS::FAIL;
	}
	ArrayElement* given_matrix_element;
	for(int index = 0; index < given_matrix->get_length(); index++) {
		given_matrix_element = (*given_matrix)[index];
		if(given_matrix_element->Val().get_type() != DATA_TYPE::TIME ) {
			errs << "gftermlon_api: tenth argument expected an array of times \n";
			return RETURN_STATUS::FAIL;
		}
        	double value = given_matrix_element->Val().get_double();
		cnfine[index] = value;
	}		
			
	int nintvls = n9->get_int();	
	double windows[2*nintvls];


	int numWindows = 0;
	try {
	gftermlon_api(  (char *)*n1->get_string(),
	            (char *)*n2->get_string(),
                (char *)*n3->get_string(),
	            (char *)*n4->get_string(),
	            (char *)*n5->get_string(),
			    n6->get_double(),
			    n7->get_double(),
                n8->get_double(),
                nintvls,
                cnfine,
			    windows,
			    &numWindows );
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	*result = create_float_list(2*numWindows, windows);
	return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS get_Flyby_q_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1;  //"integer"

	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function get_Flyby_q; expected 1, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n1 = ARG_POP ;
	if( n1->get_type() != DATA_TYPE::INTEGER )
		{
		errs << "get_Flyby_q: first argument is not an integer\n" ;
		return RETURN_STATUS::FAIL ;
		}
	try {
	*result = get_Flyby_q( n1->get_int() ) ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	return RETURN_STATUS::SUCCESS ;
	}

RETURN_STATUS get_Orb_Elements_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1;  //"integer"

	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function get_Orb_Elements; expected 1, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n1 = ARG_POP ;
	if( n1->get_type() != DATA_TYPE::INTEGER )
		{
		errs << "get_Orb_Elements: first argument is not an integer\n" ;
		return RETURN_STATUS::FAIL ;
		}
	try {
	*result = get_Orb_Elements( n1->get_int() ) ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	return RETURN_STATUS::SUCCESS ;
	}

RETURN_STATUS compute_Orb_Elements_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1, // "ET"
	                * n2, // "Mu"
                        * n3, // "x"
                        * n4, // "y"
                        * n5, // "z"
                        * n6, // "xdot"
                        * n7, // "ydot"
                        * n8; // "zdot"

	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 8 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function compute_Orb_Elements_ap; expected 8, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n8 = ARG_POP ;
	n7 = ARG_POP ;
	n6 = ARG_POP ;
	n5 = ARG_POP ;
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if( n1->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "compute_Orb_Elements_ap: first argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n2->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "compute_Orb_Elements_ap: second argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n3->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "compute_Orb_Elements_ap: third argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n4->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "compute_Orb_Elements_ap: fourth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n5->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "compute_Orb_Elements_ap: fifth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n6->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "compute_Orb_Elements_ap: sixth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n7->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "compute_Orb_Elements_ap: seventh argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n8->get_type() != DATA_TYPE::FLOATING )
		{
		errs << "compute_Orb_Elements_ap: eigth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}

	try {
	compute_Orb_Elements(  n1->get_double(), n2->get_double(), n3->get_double(), n4->get_double(), n5->get_double(), n6->get_double(), n7->get_double(), n8->get_double());
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	*result = (long) 1;
	return RETURN_STATUS::SUCCESS ;
	}


RETURN_STATUS ET_to_VTC_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1; // "ET"
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function ET_to_VTC_ap; expected 1, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n1 = ARG_POP ;
	if( ! n1->is_numeric() )
		{
		errs << "ET_to_VTC_ap: first argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	try {
	*result = ET_to_VTC( n1->get_double() ) ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	return RETURN_STATUS::SUCCESS ;
	}

RETURN_STATUS scet2sclk_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1; // "SCET"
	TypedValue	* n2; // integer value
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 2 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function scet2sclk_ap; expected 2, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if( ! n1->is_string() )
		{
		errs << "scet2sclk_ap: first argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n2->get_type() != DATA_TYPE::INTEGER )
		{
		errs << "scet2sclk_ap: second argument is not a int\n" ;
		return RETURN_STATUS::FAIL ;
		}
	try {
	*result = scet2sclk( (char *) *n1->get_string(), n2->get_int() ) ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	return RETURN_STATUS::SUCCESS ;
	}
RETURN_STATUS ET2UTCS_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1; // double value
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function ET2UTCS_ap; expected 1, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n1 = ARG_POP ;
	if( ! n1->is_numeric() )
		{
		errs << "ET2UTCS_ap: first argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	try {
	*result = ET2UTCS( n1->get_double() ) ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	return RETURN_STATUS::SUCCESS ;
	}
RETURN_STATUS utc2excel_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue* n1; // double value
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) ) {
		errs << "  wrong number of parameters passed to user-defined function utc2excel_ap; expected 1, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n";
		return RETURN_STATUS::FAIL; }
	n1 = ARG_POP;
	double n1_as_double;
	if(n1->is_time()) {
 		n1_as_double = n1->get_time_or_duration().convert_to_double_use_with_caution(); }
	else if(n1->is_numeric()) {
		n1_as_double = n1->get_double(); }
	else {
		errs << "utc2excel_ap: first argument (";
		errs << n1->to_string();
		errs << ") is neither a double nor a time\n";
		return RETURN_STATUS::FAIL;
	}
	try {
	*result = utc2excel(n1_as_double);
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	return RETURN_STATUS::SUCCESS;
	}

RETURN_STATUS utc2jd_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue* n1; // double value
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) ) {
		errs << "  wrong number of parameters passed to user-defined function utc2jd_ap; expected 1, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n";
		return RETURN_STATUS::FAIL; }
	n1 = ARG_POP;
	double n1_as_double;
	if(n1->is_time()) {
 		n1_as_double = n1->get_time_or_duration().convert_to_double_use_with_caution(); }
	else if(n1->is_numeric()) {
		n1_as_double = n1->get_double(); }
	else {
		errs << "utc2jd_ap: first argument (";
		errs << n1->to_string();
		errs << ") is neither a double nor a time\n";
		return RETURN_STATUS::FAIL; }
	try {
	*result = utc2jd(n1_as_double);
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	return RETURN_STATUS::SUCCESS; }

RETURN_STATUS sclk2scet_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1; // integer value
	TypedValue	* n2; // double value
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 2 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function sclk2scet_ap; expected 2, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if( ! n1->is_numeric() )
		{
		errs << "sclk2scet_ap: first argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	else if( n2->get_type() != DATA_TYPE::INTEGER )
		{
		errs << "sclk2scet_ap: second argument is not a int\n" ;
		return RETURN_STATUS::FAIL ;
		}
	try {
	*result = sclk2scet( n1->get_double(), n2->get_int() ) ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	return RETURN_STATUS::SUCCESS ;
	}


RETURN_STATUS Compute_Boresight_Angle_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1,  //"time"
	    * n2,  // "frame"
	    * n3,  // "axis"
	    * n4;  // "target"
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 4 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function Compute_Boresight_Angle; expected 4, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if( n1->get_type() != DATA_TYPE::TIME )
		{
		errs << "Compute_Boresight_Angle: first argument is not an time\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n2->is_string() )
		{
		errs << "Compute_Boresight_Angle_ap: second argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n3->is_string() )
		{
		errs << "Compute_Boresight_Angle_ap: third argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( n4->get_type() != DATA_TYPE::INTEGER )
		{
		errs << ": Compute_Boresight_Angle_ap fourth argument is not an integer\n" ;
		return RETURN_STATUS::FAIL ;
		}
	try {
	*result = Compute_Boresight_Angle( n1->get_double(), (char *)*n2->get_string(), (char *)*n3->get_string(), n4->get_int() ) ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	return RETURN_STATUS::SUCCESS ;
	}

RETURN_STATUS Compute_Closest_Approach_Times_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1,  //"obsrvr"
	                * n2,  // "target"
	                * n3,  // "relate"
	                * n4,  // "refval"
	                * n5,  // "IntervalStart"
	                * n6,  // "IntervalEnd"
	                * n7;  // "StepSize"
	double          windows[MAXWIN];
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 7 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function Compute_Closest_Approach_Times; expected 7, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n7 = ARG_POP ;
	n6 = ARG_POP ;
	n5 = ARG_POP ;
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(  ! n1->is_string() )
		{
		errs << "Compute_Closest_Approach_Times: first argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n2->is_string() )
		{
		errs << "Compute_Closest_Approach_Times_ap: second argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n3->is_string() )
		{
		errs << "Compute_Closest_Approach_Times_ap: third argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n4->is_numeric() )
		{
		errs << ": Compute_Closest_Approach_Times_ap fourth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( n5->get_type() != DATA_TYPE::TIME )
		{
		errs << "Compute_Closest_Approach_Times_ap: fifth argument is not a time\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( n6->get_type() != DATA_TYPE::TIME )
		{
		errs << ": Compute_Closest_Approach_Times_ap sixth argument is not an time\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n7->is_numeric() )
		{
		errs << ": Compute_Closest_Approach_Times_ap seventh argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	//(void)fprintf(stdout,"Compute_Closest_Approach_Times n3 = %f, n4 = %f\n",n3->get_double(), n4->get_double(), n5->get_double() ) ;

	int actualNumberOfWindows = 0;

	try {
	Compute_Closest_Approach_Times( (char *)*n1->get_string(), (char *)*n2->get_string(), (char *)*n3->get_string(), n4->get_double(), n5->get_double(), n6->get_double(), n7->get_double(), windows, &actualNumberOfWindows) ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	*result = create_float_list(2*actualNumberOfWindows, windows);
	return RETURN_STATUS::SUCCESS ;
	}
RETURN_STATUS Compute_Range_Windows_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1,  //"obsrvr"
	                * n2,  // "target"
	                * n3,  // "targetFrame"
	                * n4,  // "relate"
	                * n5,  // "refval"
	                * n6,  // "IntervalStart"
	                * n7,  // "IntervalEnd"
	                * n8;  // "StepSize"
	double          windows[MAXWIN];
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 8 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function Compute_Closest_Approach_Times; expected 8, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n8 = ARG_POP ;
	n7 = ARG_POP ;
	n6 = ARG_POP ;
	n5 = ARG_POP ;
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(  ! n1->is_string() )
		{
		errs << "Compute_Closest_Approach_Times: first argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n2->is_string() )
		{
		errs << "Compute_Closest_Approach_Times_ap: second argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n3->is_string() )
		{
		errs << "Compute_Closest_Approach_Times_ap: third argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n4->is_string() )
		{
		errs << ": Compute_Closest_Approach_Times_ap fourth argument is not a string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n5->is_numeric() )
		{
		errs << ": Compute_Closest_Approach_Times_ap fifth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( n6->get_type() != DATA_TYPE::TIME )
		{
		errs << "Compute_Closest_Approach_Times_ap: sixth argument is not a time\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( n7->get_type() != DATA_TYPE::TIME )
		{
		errs << ": Compute_Closest_Approach_Times_ap seventh argument is not an time\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n8->is_numeric() )
		{
		errs << ": Compute_Closest_Approach_Times_ap eight argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	//(void)fprintf(stdout,"Compute_Closest_Approach_Times n3 = %f, n4 = %f\n",n3->get_double(), n4->get_double(), n5->get_double() ) ;

	int actualNumberOfWindows = 0;

	try {
	Compute_Range_Windows( (char *)*n1->get_string(), (char *)*n2->get_string(), (char *)*n3->get_string(), (char *)*n4->get_string(), n5->get_double(), n6->get_double(), n7->get_double(), n8->get_double(), windows, &actualNumberOfWindows) ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	*result = create_float_list(2*actualNumberOfWindows, windows);
	return RETURN_STATUS::SUCCESS ;
	}

RETURN_STATUS Compute_Occultation_Times_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1,  //"OccultationType"
	                * n2,  //"obsrvr"
	                * n3,  // "target"
	                * n4,  // "TargetBodyFrame"
	                * n5,  // "TargetShape"
	                * n6,  // "OccultingBody"
	                * n7,  // "OccultingBodyFrame"
	                * n8,  // "IntervalStart"
	                * n9,  // "IntervalEnd"
	                * n10;  // "StepSize"
	double          windows[MAXWIN];
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 10 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function Compute_Occultation_Times; expected 10, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n10 = ARG_POP ;
	n9 = ARG_POP ;
	n8 = ARG_POP ;
	n7 = ARG_POP ;
	n6 = ARG_POP ;
	n5 = ARG_POP ;
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(  ! n1->is_string() )
		{
		errs << "Compute_Occultation_Times: first argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if(  ! n2->is_string() )
		{
		errs << "Compute_Occultation_Times: second argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n3->is_string() )
		{
		errs << "Compute_Occultation_Times_ap: third argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n4->is_string() )
		{
		errs << "Compute_Occultation_Times_ap: fourth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n5->is_string() )
		{
		errs << "Compute_Occultation_Times_ap: fifth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n6->is_string() )
		{
		errs << "Compute_Occultation_Times_ap: sixth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n7->is_string() )
		{
		errs << "Compute_Occultation_Times_ap: sixth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( n8->get_type() != DATA_TYPE::TIME )
		{
		errs << "Compute_Occultation_Times_ap: seventh argument is not a time\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( n9->get_type() != DATA_TYPE::TIME )
		{
		errs << ": Compute_Occultation_Times_ap eighth argument is not an time\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n10->is_numeric() )
		{
		errs << ": Compute_Occultation_Times_ap ninth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	int actualNumberOfWindows = 0;
	try {

	Compute_Occultation_Times( (char *)*n1->get_string(), (char *)*n2->get_string(), (char *)*n3->get_string(), (char *)*n4->get_string(), (char *)*n5->get_string(), (char *)*n6->get_string(), (char *)*n7->get_string(), n8->get_double(), n9->get_double(), n10->get_double(), windows, &actualNumberOfWindows ) ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	*result = create_float_list(2*actualNumberOfWindows, windows);
	return RETURN_STATUS::SUCCESS ;
	}


RETURN_STATUS Compute_Surface_Intercept_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1,  //"time"
	                * n2,  // "target"
	                * n3,  // "target_frame"
	                * n4,  // "obsrvr"
	                * n5,  // "dvec0"
	                * n6,  // "dvec1"
	                * n7;  // "dvec2"
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 7 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function Compute_Occultation_Times; expected 7, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n7 = ARG_POP ;
	n6 = ARG_POP ;
	n5 = ARG_POP ;
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(  n1->get_type() != DATA_TYPE::TIME )
		{
		errs << "Compute_Surface_Intercept: first argument is not a time\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n2->is_string() )
		{
		errs << "Compute_Surface_Intercept_ap: second argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n3->is_string() )
		{
		errs << "Compute_Surface_Intercept_ap: third argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n4->is_string() )
		{
		errs << "Compute_Surface_Intercept_ap: fourth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n5->is_numeric() )
		{
		errs << "Compute_Surface_Intercept_ap: fifth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}

	if( ! n6->is_numeric() )
		{
		errs << "Compute_Surface_Intercept_ap: sixth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}

	if( ! n7->is_numeric() )
		{
		errs << "Compute_Surface_Intercept_ap: seventh argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	int found;
	double  dist;
	double  lat;
	double  lon;
	double  radius;
	double  PlanetographicLon;
	double  PlanetographicLat;
	double  PlanetographicAlt;
	double  ReturnData[7];

	try {
	found = Compute_Surface_Intercept( n1->get_double(), (char *)*n2->get_string(), (char *)*n3->get_string(), (char *)*n4->get_string(), n5->get_double(), n6->get_double(), n7->get_double(), &dist, &lat, &lon, &radius, &PlanetographicLon, &PlanetographicLat, &PlanetographicAlt ) ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	ReturnData[0] = dist;
	ReturnData[1] = lat;
	ReturnData[2] = lon;
	ReturnData[3] = radius;
	ReturnData[4] = PlanetographicLon;
	ReturnData[5] = PlanetographicLat;
	ReturnData[6] = PlanetographicAlt;

	*result = create_float_list(7, ReturnData);
	return RETURN_STATUS::SUCCESS ;
	}

#ifdef OBSOLETE
RETURN_STATUS Compute_Surface_Intercept2_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1,  //"time"
	                * n2,  // "target"
	                * n3,  // "target_frame"
	                * n4; // "obsrvr"
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 4 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function Compute_Occultation_Times; expected 4, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(  n1->get_type() != DATA_TYPE::TIME )
		{
		errs << "Compute_Surface_Intercept: first argument is not a time\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n2->is_string() )
		{
		errs << "Compute_Surface_Intercept_ap: second argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n3->is_string() )
		{
		errs << "Compute_Surface_Intercept_ap: third argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n4->is_string() )
		{
		errs << "Compute_Surface_Intercept_ap: fourth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	double  dist;
	double  lat;
	double  lon;
	double  radius;
	double  PlanetographicLon;
	double  PlanetographicLat;
	double  PlanetographicAlt;
	double  ReturnData[7];

	try {
	Compute_Surface_Intercept2(
		n1->get_double(), (char *)*n2->get_string(), (char *)*n3->get_string(), (char *)*n4->get_string(),
		&dist, &lat, &lon, &radius, &PlanetographicLon, &PlanetographicLat, &PlanetographicAlt ) ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	ReturnData[0] = dist;
	ReturnData[1] = lat;
	ReturnData[2] = lon;
	ReturnData[3] = radius;
	ReturnData[4] = PlanetographicLon;
	ReturnData[5] = PlanetographicLat;
	ReturnData[6] = PlanetographicAlt;

	*result = create_float_list(7, ReturnData);
	return RETURN_STATUS::SUCCESS ;
	}

#endif /* OBSOLETE */

RETURN_STATUS Compute_Ang_Sep_Times_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1,  //"targ1"
	                * n2,  // "shape1"
	                * n3,  // "frame1"
	                * n4,  // "targ2"
	                * n5,  // "shape2"
	                * n6,  // "frame2"
	                * n7,  // "obsrvr"
	                * n8,  // "relate"
	                * n9,  // "refval"
	                * n10,  // "IntervalStart"
	                * n11,  // "IntervalEnd"
	                * n12;  // "StepSize"
	double          windows[2*MAXWIN];
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 12 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function Compute_Ang_Sep_Times; expected 12, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n12 = ARG_POP ;
	n11 = ARG_POP ;
	n10 = ARG_POP ;
	n9 = ARG_POP ;
	n8 = ARG_POP ;
	n7 = ARG_POP ;
	n6 = ARG_POP ;
	n5 = ARG_POP ;
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(  ! n1->is_string() )
		{
		errs << "Compute_Ang_Sep_Times: first argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n2->is_string() )
		{
		errs << "Compute_Ang_Sep_Times_ap: second argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n3->is_string() )
		{
		errs << "Compute_Ang_Sep_Times_ap: third argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n4->is_string() )
		{
		errs << "Compute_Ang_Sep_Times_ap: fourth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n5->is_string() )
		{
		errs << "Compute_Ang_Sep_Times_ap: fifth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n6->is_string() )
		{
		errs << "Compute_Ang_Sep_Times_ap: sixth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n7->is_string() )
		{
		errs << "Compute_Ang_Sep_Times_ap: seventh argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n8->is_string() )
		{
		errs << "Compute_Ang_Sep_Times_ap: eighth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n9->is_numeric() )
		{
		errs << ": Compute_Ang_Sep_Times_ap ninth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( n10->get_type() != DATA_TYPE::TIME )
		{
		errs << "Compute_Ang_Sep_Times_ap: tenth argument is not a time\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( n11->get_type() != DATA_TYPE::TIME )
		{
		errs << ": Compute_Ang_Sep_Times_ap eleventh argument is not an time\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n12->is_numeric() )
		{
		errs << ": Compute_Ang_Sep_Times_ap twelfth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	int actualNumberOfWindows = 0;
	try {
	Compute_Ang_Sep_Times( (char *)*n1->get_string(), //targ1
			       (char *)*n2->get_string(), //shape1
			       (char *)*n3->get_string(), //frame1
			       (char *)*n4->get_string(), //targ2
			       (char *)*n5->get_string(), //shape2
			       (char *)*n6->get_string(), //fram22
			       (char *)*n7->get_string(), //obsrvr
			       (char *)*n8->get_string(), //relate
			       n9->get_double(), //refval
			       n10->get_double(), //IntervalStart
			       n11->get_double(), //IntervalEnd
			       n12->get_double(), //step
			       windows, &actualNumberOfWindows ) ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	//(void)fprintf(stdout," in udef_functions.C, actualNumberOfWindows = %d\n",actualNumberOfWindows);
	*result = create_float_list(2*actualNumberOfWindows, windows);
	return RETURN_STATUS::SUCCESS ;
	}

RETURN_STATUS Compute_Pole_Crossing_Times_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1,  //"target"
	                * n2,  // "fixref"
	                * n3,  // "obsrvr"
	                * n4,  // "relate"
	                * n5,  // "refval"
	                * n6,  // "IntervalStart"
	                * n7,  // "IntervalEnd"
	                * n8;  // "step"
	double          windows[2*MAXWIN];
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 8 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function Compute_Pole_Crossing_Times; expected 8, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n8 = ARG_POP ;
	n7 = ARG_POP ;
	n6 = ARG_POP ;
	n5 = ARG_POP ;
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(  ! n1->is_string() )
		{
		errs << "Compute_Pole_Crossing_Times: first argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n2->is_string() )
		{
		errs << "Compute_Pole_Crossing_Times: second argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n3->is_string() )
		{
		errs << "Compute_Pole_Crossing_Times: third argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n4->is_string() )
		{
		errs << "Compute_Pole_Crossing_Times: fourth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n5->is_numeric() )
		{
		errs << "Compute_Pole_Crossing_Times: fifth argument is not an double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n6->is_numeric() )
		{
		errs << "Compute_Pole_Crossing_Times: sixth argument is not an double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n7->is_numeric() )
		{
		errs << "Compute_Pole_Crossing_Times: seventh argument is not an double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n8->is_numeric() )
		{
		errs << "Compute_Pole_Crossing_Times: eighth argument is not an double\n" ;
		return RETURN_STATUS::FAIL ;
		}

	int actualNumberOfWindows = 0;
	try {
	Compute_Pole_Crossing_Times( (char *)*n1->get_string(), //target
			       (char *)*n2->get_string(), //fixref
			       (char *)*n3->get_string(), //obsrvr
			       (char *)*n4->get_string(), //relate
			       n5->get_double(), //refval
			       n6->get_double(), //IntervalStart
			       n7->get_double(), //IntervalEnd
			       n8->get_double(), //step
			       windows, &actualNumberOfWindows ) ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	(void)fprintf(stdout," in udef_functions.C, actualNumberOfWindows = %d\n",actualNumberOfWindows);
	*result = create_float_list(2*actualNumberOfWindows, windows);
	return RETURN_STATUS::SUCCESS ;
	}
RETURN_STATUS Compute_Ang_Sep_Vec_Times_ap ( Cstring & errs , TypedValue * result , ARG_STACK )
	{
	TypedValue	* n1,  //"target"
	                * n2,  // "frame"
	                * n3,  // "abcorr"
	                * n4,  // "obsrvr"
	                * n5,  // "crdsys"
	                * n6,  // "coord"
	                * n7,  // "relate"
	                * n8,  // "refval"
	                * n9,  // "adjust"
	                * n10,  // "IntervalStart"
	                * n11,  // "IntervalEnd"
	                * n12;  // "StepSize"

	double          windows[2*MAXWIN];
	errs.undefine() ;
	if( ARG_POINTER != ( WHERE_TO_STOP + 12 ) )
		{
		errs << "  wrong number of parameters passed to user-defined function Compute_Ang_Sep_Times; expected 12, got "
			<< ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
		return RETURN_STATUS::FAIL ;
		}
	n12 = ARG_POP ;
	n11 = ARG_POP ;
	n10 = ARG_POP ;
	n9 = ARG_POP ;
	n8 = ARG_POP ;
	n7 = ARG_POP ;
	n6 = ARG_POP ;
	n5 = ARG_POP ;
	n4 = ARG_POP ;
	n3 = ARG_POP ;
	n2 = ARG_POP ;
	n1 = ARG_POP ;
	if(  ! n1->is_string() )
		{
		errs << "Compute_Ang_Sep_Times: first argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n2->is_string() )
		{
		errs << "Compute_Ang_Sep_Times_ap: second argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n3->is_string() )
		{
		errs << "Compute_Ang_Sep_Times_ap: third argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n4->is_string() )
		{
		errs << "Compute_Ang_Sep_Times_ap: fourth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n5->is_string() )
		{
		errs << "Compute_Ang_Sep_Times_ap: fifth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n6->is_string() )
		{
		errs << "Compute_Ang_Sep_Times_ap: sixth argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n7->is_string() )
		{
		errs << "Compute_Ang_Sep_Times_ap: seventh argument is not an string\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n8->is_numeric() )
		{
		errs << "Compute_Ang_Sep_Times_ap: eighth argument is not an double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n9->is_numeric() )
		{
		errs << ": Compute_Ang_Sep_Times_ap ninth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( n10->get_type() != DATA_TYPE::TIME )
		{
		errs << "Compute_Ang_Sep_Times_ap: tenth argument is not a time\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( n11->get_type() != DATA_TYPE::TIME )
		{
		errs << ": Compute_Ang_Sep_Times_ap eleventh argument is not an time\n" ;
		return RETURN_STATUS::FAIL ;
		}
	if( ! n12->is_numeric() )
		{
		errs << ": Compute_Ang_Sep_Times_ap twelfth argument is not a double\n" ;
		return RETURN_STATUS::FAIL ;
		}
	int actualNumberOfWindows = 0;
	try {
	Compute_Ang_Sep_Vec_Times( (char *)*n1->get_string(), //target
			       (char *)*n2->get_string(), //frame
			       (char *)*n3->get_string(), //abcorr
			       (char *)*n4->get_string(), //obsrvr
			       (char *)*n5->get_string(), //crdsys
			       (char *)*n6->get_string(), //coord
			       (char *)*n7->get_string(), //relate
			       n8->get_double(), //refval
			       n9->get_double(), //adjust
			       n10->get_double(), //IntervalStart
			       n11->get_double(), //IntervalEnd
			       n12->get_double(), //step
			       windows, &actualNumberOfWindows ) ;
	} catch(eval_error Err) {
		errs = Err.msg;
		return RETURN_STATUS::FAIL;
	}
	//(void)fprintf(stdout," in udef_functions.C, actualNumberOfWindows = %d\n",actualNumberOfWindows);
	*result = create_float_list(2*actualNumberOfWindows, windows);
	return RETURN_STATUS::SUCCESS ;
}

RETURN_STATUS FrameInfo_ap( Cstring & errs , TypedValue * result , ARG_STACK ) {
    TypedValue	* n1;  // "frame"

    errs.undefine() ;
    if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) ) {
	errs << "  wrong number of parameters passed to user-defined function FrameInfo; "
		"expected 1, got " << ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
	return RETURN_STATUS::FAIL ;
    }
    n1 = ARG_POP ;
    if(n1->get_type() != DATA_TYPE::STRING) {
	errs << "FrameInfo: argument is not a string";
	return RETURN_STATUS::FAIL;
    }

    long int center_ID, frame_class, class_frame_ID;
    if(!FrameInfo(*n1->get_string(), center_ID, frame_class, class_frame_ID)) {
	errs << "FrameInfo: frame " << n1->get_string() << " unknown\n";
	return RETURN_STATUS::FAIL;
    }
    ListOVal*	returned_array = new ListOVal;
    TypedValue	val;
    val = center_ID;
    returned_array->add(Cstring("center"), val);
    val = frame_class;
    returned_array->add(Cstring("frame_class"), val);
    val = class_frame_ID;
    returned_array->add(Cstring("class_frame_ID"), val);
    *result = *returned_array;
    return RETURN_STATUS::SUCCESS;
}

RETURN_STATUS FrameXform_ap( Cstring & errs , TypedValue * result , ARG_STACK ) {
    TypedValue*	n1; // "from_frame"
    TypedValue*	n2; // "to_frame"
    TypedValue*	n3; // "now_time"

    Cstring	from_frame, to_frame;
    double	now_time;
    double	xform[6][6];

    errs.undefine() ;
    if( ARG_POINTER != ( WHERE_TO_STOP + 3 ) ) {
	errs << "  wrong number of parameters passed to user-defined function FrameXform; "
		"expected 3, got " << ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
	return RETURN_STATUS::FAIL ;
    }
    n3 = ARG_POP ;
    n2 = ARG_POP ;
    n1 = ARG_POP ;
    if(n1->get_type() != DATA_TYPE::STRING) {
	errs << "FrameXform: argument 1 is not a string";
	return RETURN_STATUS::FAIL;
    } else {
	from_frame = n1->get_string();
    }
    if(n2->get_type() != DATA_TYPE::STRING) {
	errs << "FrameXform: argument 2 is not a string";
	return RETURN_STATUS::FAIL;
    } else {
	from_frame = n2->get_string();
    }
    if(n3->get_type() != DATA_TYPE::FLOATING) {
	errs << "FrameXform: argument 3 is not a double";
    } else {
	now_time = n3->get_double();
    }

    //
    // Get the 6 x 6 transformation matrix
    //
    try {
	StateTransform(from_frame, to_frame, now_time, xform);
    } catch(eval_error Err) {
	errs = Err.msg;
	return RETURN_STATUS::FAIL;
    }
    ListOVal*	returned_matrix = new ListOVal;
	
    for(int outer_index = 0; outer_index < 6; outer_index++) {

	ListOVal*	returned_sub_array_list = new ListOVal;
	
	for (int inner_index = 0; inner_index < 6; inner_index++) {
	    double		value = xform[outer_index][inner_index];
	    returned_sub_array_list->add(inner_index, value);
	}
	returned_matrix->add(outer_index, *returned_sub_array_list);
    }

    *result = *returned_matrix;
    return RETURN_STATUS::SUCCESS;
}

RETURN_STATUS AngularVelocityFromXform_ap( Cstring & errs , TypedValue * result , ARG_STACK ) {
    TypedValue*	n1; // "xform"

    double	xform[6][6];

    errs.undefine() ;
    if( ARG_POINTER != ( WHERE_TO_STOP + 1 ) ) {
	errs << "  wrong number of parameters passed to user-defined function AngularVelocityFromXform; "
		"expected 1, got " << ( ARG_POINTER - WHERE_TO_STOP ) << "\n" ;
	return RETURN_STATUS::FAIL ;
    }
    n1 = ARG_POP ;
    if(n1->get_type() != DATA_TYPE::ARRAY) {
	errs << "FrameAngularVelocity: argument 1 is not a string";
	return RETURN_STATUS::FAIL;
    } else {
	ListOVal&	given_matrix = n1->get_array();

	//
	// Extract matrix from array
	//
	for(int index = 0; index < given_matrix.get_length(); index++) {
	    ArrayElement* given_matrix_element = given_matrix[index];
	    if(!given_matrix_element->Val().is_array()) {
		errs << "AngularVelocityFromXform: expected a matrix of doubles \n";
		return RETURN_STATUS::FAIL;
            }
	    ListOVal& submatrix = given_matrix_element->Val().get_array();
	    for(int j = 0; j < submatrix.get_length(); index++) {
		ArrayElement* sub_array_element = submatrix[j];
		if(!sub_array_element->Val().is_array()) {
		    errs << "AngularVelocityFromXform: expected a matrix of doubles \n";
		    return RETURN_STATUS::FAIL;
        	}
		double	value = sub_array_element->Val().get_double();
		xform[index][j] = value;
	    }
	}
    }

    //
    // Get the angular velocity vector
    //
    double	ang_vel[3];
    try {
	AngularVelocityFromXform(xform, ang_vel);
    } catch(eval_error Err) {
	errs = Err.msg;
	return RETURN_STATUS::FAIL;
    }
    ListOVal*	lov = new ListOVal;
    for (int i = 0; i < 3; i++) {
	double	value = ang_vel[i];
	lov->add(i, value);
    }
    *result = *lov;
    return RETURN_STATUS::SUCCESS;
}

// @@@ END   SPICE_CALC

void register_spice_subsystem_functions() {

	udef_intfc::add_to_Func_list( "Compute_Boresight_Angle",	Compute_Boresight_Angle_ap,	apgen::DATA_TYPE::FLOATING);
	udef_intfc::add_to_Func_list( "Compute_Closest_Approach_Times",	Compute_Closest_Approach_Times_ap,apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "Compute_Range_Windows",		Compute_Range_Windows_ap,	apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "Compute_Occultation_Times",	Compute_Occultation_Times_ap,	apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "Compute_Surface_Intercept",	Compute_Surface_Intercept_ap,	apgen::DATA_TYPE::ARRAY);

#ifdef OBSOLETE
	udef_intfc::add_to_Func_list( "Compute_Surface_Intercept2",	Compute_Surface_Intercept2_ap,	apgen::DATA_TYPE::ARRAY);
#endif /* OBSOLETE */

	udef_intfc::add_to_Func_list( "Compute_Ang_Sep_Times",		Compute_Ang_Sep_Times_ap,	apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "Compute_Pole_Crossing_Times",	Compute_Pole_Crossing_Times_ap,	apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "Compute_Ang_Sep_Vec_Times",	Compute_Ang_Sep_Vec_Times_ap,	apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "get_Sun_Elevation" ,		get_Sun_Elevation_ap,		apgen::DATA_TYPE::FLOATING ) ;
	udef_intfc::add_to_Func_list( "get_Sun_Azimuth" ,		get_Sun_Azimuth_ap,		apgen::DATA_TYPE::FLOATING ) ;
	udef_intfc::add_to_Func_list( "get_Earth_Elevation" ,		get_Earth_Elevation_ap,		apgen::DATA_TYPE::FLOATING ) ;
	udef_intfc::add_to_Func_list( "get_Earth_Azimuth" ,		get_Earth_Azimuth_ap,		apgen::DATA_TYPE::FLOATING ) ;


	udef_intfc::add_to_Func_list( "get_horizon_elevation",		get_horizon_elevation_ap,	apgen::DATA_TYPE::FLOATING);
	udef_intfc::add_to_Func_list( "get_Flyby_q",			get_Flyby_q_ap,			apgen::DATA_TYPE::FLOATING);
	udef_intfc::add_to_Func_list( "init_spice",			init_spice_ap,			apgen::DATA_TYPE::INTEGER);
	udef_intfc::add_to_Func_list( "reinit_spice",			reinit_spice_ap,		apgen::DATA_TYPE::INTEGER);

	udef_intfc::add_to_Func_list( "compute_light_time",		compute_light_time_ap,		apgen::DATA_TYPE::DURATION);
	udef_intfc::add_to_Func_list( "compute_station_angles", 	compute_station_angles_ap,	apgen::DATA_TYPE::ARRAY);


	udef_intfc::add_to_Func_list( "et2utcs",			ET2UTCS_ap,			apgen::DATA_TYPE::STRING);
	udef_intfc::add_to_Func_list( "utc2excel",			utc2excel_ap,			apgen::DATA_TYPE::STRING);
	udef_intfc::add_to_Func_list( "utc2jd",				utc2jd_ap,			apgen::DATA_TYPE::STRING);
	udef_intfc::add_to_Func_list( "spkcov_api",			spkcov_api_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "Cart2RADec",			Cart2RADec_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "spkez_api",  			spkez_api_ap,			apgen::DATA_TYPE::ARRAY);

	//
	// Soon to be obsolete:
	//
	udef_intfc::add_to_Func_list( "spice_spkez",  			spkez_intfc::spice_spkez_ap,	apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "spkez_control", 			spkez_control_ap,		apgen::DATA_TYPE::INTEGER);
	udef_intfc::add_to_Func_list( "CreateGeometrySubsystemAAF",	create_geom_subsys_aaf_ap,	apgen::DATA_TYPE::INTEGER);

	//
	// The new way of creating the geometry subsystem:
	//
	udef_intfc::add_to_Func_list( "create_geom_subsys",		create_geom_subsys_ap,		apgen::DATA_TYPE::INTEGER);

	udef_intfc::add_to_Func_list( "Time_to_ET",			Time_to_ET_ap,			apgen::DATA_TYPE::FLOATING);
	udef_intfc::add_to_Func_list( "compute_Orb_Elements",		compute_Orb_Elements_ap,	apgen::DATA_TYPE::INTEGER);
	udef_intfc::add_to_Func_list( "get_Orb_Elements",		get_Orb_Elements_ap,		apgen::DATA_TYPE::FLOATING);
	udef_intfc::add_to_Func_list( "scet2sclk",			scet2sclk_ap,			apgen::DATA_TYPE::FLOATING);
	udef_intfc::add_to_Func_list( "sclk2scet",			sclk2scet_ap,			apgen::DATA_TYPE::STRING);

	udef_intfc::add_to_Func_list( "ET_to_VTC",			ET_to_VTC_ap,			apgen::DATA_TYPE::FLOATING);
	udef_intfc::add_to_Func_list( "generateCK",			generateCK,			apgen::DATA_TYPE::INTEGER);
	udef_intfc::add_to_Func_list( "vprjp_api",			vprjp_api_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "spkcpt_api",  			spkcpt_api_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "reclat_api",  			reclat_api_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "recsph_api",			recsph_api_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "recgeo_api",			recgeo_api_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "latrec_api",			latrec_api_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "lspcn_api",			lspcn_api_ap,			apgen::DATA_TYPE::FLOATING);
	udef_intfc::add_to_Func_list( "pxform_api",			pxform_api_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "twovec_api",			twovec_api_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "m2q_api", 			m2q_api_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "vzfrac_api", 			vzfrac_api_ap,			apgen::DATA_TYPE::FLOATING);
	udef_intfc::add_to_Func_list( "bodvrd_api", 			bodvrd_api_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "sincpt_api", 			sincpt_api_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "subpnt_api", 			subpnt_api_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "ilumin_api", 			ilumin_api_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "gfsep_api", 			gfsep_api_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "gfposc_api", 			gfposc_api_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "gfdist_api", 			gfdist_api_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "et2lst_api", 			et2lst_api_ap,			apgen::DATA_TYPE::FLOATING);
	udef_intfc::add_to_Func_list( "gflst_api", 			gflst_api_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "gfemi_api", 			gfemi_api_ap,			apgen::DATA_TYPE::ARRAY);	
	udef_intfc::add_to_Func_list( "gfinc_api", 			gfinc_api_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "gflllon_api", 			gflllon_api_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "gftermlon_api", 			gftermlon_api_ap,		apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "gfpa_api", 			gfpa_api_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "gfilum_api",			gfilum_api_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "frinfo_api",			FrameInfo_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "sxform_api",			FrameXform_ap,			apgen::DATA_TYPE::ARRAY);
	udef_intfc::add_to_Func_list( "xfrav_api",			AngularVelocityFromXform_ap,	apgen::DATA_TYPE::ARRAY);
}

//
// Invoked at the beginning of modeling
//
void clear_spice_cache() {
	spkez_intfc::clear_cache();
}

//
// Invoked at the end of modeling
//
void clean_up_spice_cache() {
	spkez_intfc::clean_up_cache();
}

//
// Invoked just before exit
//
void last_call_to_spice() {
	spkez_intfc::last_call();
}
