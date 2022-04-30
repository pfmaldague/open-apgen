#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "spiceIntfc.H"

extern "C" {

    char *SCET_PRINT_char_ptr(double time) {
	static char charbuffer[100];
	strcpy(charbuffer, *SCET_PRINT(time));
	return charbuffer;
    }
}


//
// Implemented later in this file:
//
extern void compute_subsc_lst_decr( void ( * udfuns ) ( SpiceDouble    et,
                                         SpiceDouble  * value ),
                            SpiceDouble    et,
                            SpiceBoolean * isdecr );   
extern void compute_subsc_lst( SpiceDouble et, SpiceDouble * value );
extern void compute_subsc_emi_decr( void ( * udfuns ) ( SpiceDouble    et,
                                         SpiceDouble  * value ),
                            SpiceDouble    et,
                            SpiceBoolean * isdecr );   
extern void compute_subsc_emi( SpiceDouble et, SpiceDouble * value );
extern void compute_subsc_inc_decr( void ( * udfuns ) ( SpiceDouble    et,
                                         SpiceDouble  * value ),
                            SpiceDouble    et,
                            SpiceBoolean * isdecr );   
extern void compute_subsc_inc( SpiceDouble et, SpiceDouble * value );
extern void compute_subsc_litlimblon_decr( void ( * udfuns ) ( SpiceDouble    et,
                                         SpiceDouble  * value ),
                            SpiceDouble    et,
                            SpiceBoolean * isdecr );   
extern void compute_subsc_litlimblon( SpiceDouble et, SpiceDouble * value );
extern void compute_subsc_termlon_decr( void ( * udfuns ) ( SpiceDouble    et,
                                         SpiceDouble  * value ),
                            SpiceDouble    et,
                            SpiceBoolean * isdecr );   
extern void compute_subsc_termlon( SpiceDouble et, SpiceDouble * value );
	   
#define NO_ERROR                          0
#define ERRORs                             1
#define MAX_LENGTH 1024
#ifndef FALSE
#define FALSE          0
#endif
#ifndef TRUE
#define TRUE           -1
#endif

static   int spice_init = FALSE;

static  SpiceDouble             sunel;
static  SpiceDouble             sunaz;
static  SpiceDouble             sunrange;
static  SpiceDouble             earthel;
static  SpiceDouble             earthaz;
static  SpiceDouble             earthrange;
static  SpiceDouble             orbiterel;
static  SpiceDouble             orbiteraz;
static  SpiceDouble             orbiterrange;

static  int orbiter_available = FALSE;
static  int comet_available = FALSE;
static  int impactor_available = FALSE;
static  int flyby_available = FALSE;

//
// Wrapper that provides more information about kernels:
//
int call_furnsh_c(const char* metakernelFile);


void init_spice(const char *metakernel_file) {


  SpiceInt  handle;
  (void)erract_c ( "SET", 256, "REPORT" );

  if ( !spice_init ){
    std::cout << "Initializing SPICE " << tkvrsn_c("TOOLKIT") <<" with "<< metakernel_file <<"\n";
    if(call_furnsh_c ( metakernel_file )) {
        throw(eval_error("Errors found in call_furnsh_c"));
    }
  }
  spice_init = TRUE;
  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}

void reinit_spice(const char *ck_file){


  SpiceInt  which,count,fillen=256,typlen=256,srclen=256,handle;
  SpiceChar file[256],filtyp[256],source[256];
  SpiceBoolean found;
  char ck_file_path[256];
  ktotal_c( "ck", &which );
  fprintf(stdout,"Number of C-Kernels to unload %d\n",which);
  for ( count=0; count<= which; count++ ){
    kdata_c ( count,  "ck", fillen,   typlen,  srclen, file,  filtyp, source,  &handle, &found );
    // The .bc represent the C-Kernels for the launch vehicle...
    if ( strcmp( file, "" ) != 0 && strcmp( file, ".bc" ) != 0){
      fprintf(stdout,"Unloading ::%s::\n",file);
      unload_c( file );
    }
    
  }
  sprintf(ck_file_path,"%s/spice/%s",getenv("PSS_INPUT_PATH"),ck_file);
  fprintf(stdout,"loading %s\n",ck_file_path);
  // furnsh_c(ck_file_path);
  if(call_furnsh_c ( ck_file_path )) {
      throw(eval_error("Errors found in call_furnsh_c"));
  }
  fprintf(stdout,"New C-Kernels loaded\n");

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}

static char OutTime[1024];
char *ConvertMPFTime( char *MPFTime )
{
  SpiceDouble  epoch;

  str2et_c ( MPFTime , &epoch );
  et2utc_c ( epoch, "ISOD", 4, 1024, OutTime );
  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }

  return( OutTime );
}
double ET_to_VTC( double ET )
{
  char clkstr[50];
  double temp = 0.0;

  sce2s_c  ( -140,  ET, 50, clkstr );
  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
  return atof( &clkstr[2]);

}

char* ET2UTCS( double ET)
{
  static char         MGSO_time[1024];
  et2utc_c ( ET, "ISOD", 3, 1024, MGSO_time );
  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
  return MGSO_time;
}
double scet2sclk( char *SCET, int Spacecraft )
{
  static double       SCLK;
  char clkstr[50];
  SpiceDouble  epoch;
  str2et_c ( SCET, &epoch );
  sce2c_c  ( -Spacecraft,  epoch, &SCLK );
  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
  return SCLK/256.0;
}


char* sclk2scet( double SCLK, int Spacecraft )
{
  static char SCET[1024];
  SpiceDouble  epoch;
  sct2e_c(-Spacecraft, SCLK*256, &epoch);
  et2utc_c ( epoch, "ISOD", 4, 1024, SCET );
  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
  return SCET;
}
double Time_to_ET( double time )
{
  char         MGSO_time[1024];
  SpiceDouble  epoch;
  strcpy( MGSO_time, SCET_PRINT_char_ptr( time ));
  
  str2et_c ( MGSO_time , &epoch );
  //(void)fprintf(stdout,"%f %s %f\n",time, MGSO_time, epoch);
  //(void)fprintf(stdout,"%f %s %f\n",time,MGSO_time,epoch);
  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
  return epoch;
}
char *utc2jd( double time )
{
  SpiceDouble  et;
  static char  Julian_time[1024];
  double JulianDate;
  et =  Time_to_ET( time );
  timout_c(et,"JULIAND.##############::UTC",24,Julian_time);
  //sscanf(Julian_time,"%f",&JulianDate);
  //fprintf(stdout," time = %f et = %f Julian_time = %s JulianDate = %f\n",time,et,Julian_time,JulianDate);
  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
  return Julian_time;
}
char *utc2excel( double time )
{
  SpiceDouble  et;
  static char  EXCEL_time[1024];
  et =  Time_to_ET( time );
  timout_c(et,"YYYY-MM-DD HR:MN:SC.###::UTC",24,EXCEL_time);
  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
  return EXCEL_time;
}

int compute_lander_sun_earth_az_el(  int sc_id, double time, const char *frame ){

  char         MGSO_time[1024];
  SpiceDouble  epoch;
  SpiceInt     targ;
  SpiceInt     obs;
  SpiceDouble  lt;
  SpiceDouble  state  [ 6 ];
  SpiceChar    utc_epoch [ 32 ];
  
  //(void)fprintf( stdout, "sc = %d, frame = %s\n",sc_id, frame );

  strcpy( MGSO_time, SCET_PRINT_char_ptr( time ));
  targ      = 10;
  obs       = -sc_id;

  /* Convert UTC epoch to ephemeris seconds past J2000 */
  str2et_c ( MGSO_time , &epoch );
   
  /*
    Get state of target as seen from observer in a specified 
    coordinate frame.
    */
  spkez_c  ( targ, epoch, frame, "lt", obs, state, &lt );
            
  /*
    If state obtained on the previous step is relative to a lander in
    topocentric frame(Z up, X - north), then it can be directly converted 
    to azimuth and elevation.
    */
  recrad_c ( state,  &sunrange,  &sunaz,     &sunel );

  targ      = 3;

  /*
    Get state of target as seen from observer in a specified 
    coordinate frame.
    */
  spkez_c  ( targ, epoch, frame, "lt", obs, state, &lt );
            
  /*
    If state obtained on the previous step is relative to a lander in
    topocentric frame(Z up, X - north), then it can be directly converted 
    to azimuth and elevation.
    */
  recrad_c ( state,  &earthrange,  &earthaz,     &earthel );

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }


  return (NO_ERROR);
}
int compute_lander_orbiter_az_el(  int lander_sc_id, int orbiter_sc_id, double time, const char *frame ){

  char         MGSO_time[1024];
  SpiceDouble  epoch;
  SpiceInt     targ;
  SpiceInt     obs;
  SpiceDouble  lt;
  SpiceDouble  state  [ 6 ];
  SpiceChar    utc_epoch [ 32 ];

  //  (void)fprintf( stdout, "lsc = %d, osc = %d, frame = %s\n",lander_sc_id, orbiter_sc_id, frame );

  if ( !orbiter_available ){

    return (ERRORs);
  }

  strcpy( MGSO_time, SCET_PRINT_char_ptr( time ));
  targ      = -orbiter_sc_id;
  obs       = -lander_sc_id;

  /* Convert UTC epoch to ephemeris seconds past J2000 */
  str2et_c ( MGSO_time , &epoch );
   
  /*
    Get state of target as seen from observer in a specified 
    coordinate frame.
    */
  spkez_c  ( targ, epoch, frame, "lt", obs, state, &lt );
            
  /*
    If state obtained on the previous step is relative to a lander in
    topocentric frame(Z up, X - north), then it can be directly converted 
    to azimuth and elevation.
    */
  recrad_c ( state,  &orbiterrange,  &orbiteraz,     &orbiterel );
  //  (void)fprintf(stdout, "Time: %s, Orbiter elevation : %f\n", MGSO_time , orbiterel*dpr_c() );

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
  return (NO_ERROR);
}

double get_Sun_Elevation( void )
{
  return( sunel * dpr_c() );
}
double get_Sun_Azimuth( void )
{
  return( sunaz * dpr_c() );
}
double get_Earth_Elevation( void )
{
  return( earthel * dpr_c() );
}
double get_Earth_Azimuth( void )
{
  return( earthaz * dpr_c() );
}
double get_Orbiter_Elevation( void )
{
  return( orbiterel * dpr_c() );
}
double get_Orbiter_Azimuth( void )
{
  return( 360.0 - orbiteraz * dpr_c() );
}
double get_Orbiter_Range( void )
{
  return( orbiterrange );
}


void compute_comet_state( int flyby_id, int comet_id, double time, double* results)
{
  char         MGSO_time[1024];
  SpiceDouble  epoch;
  SpiceDouble  lt;

  strcpy( MGSO_time, SCET_PRINT_char_ptr( time ));

  /* Convert UTC epoch to ephemeris seconds past J2000 */
  str2et_c ( MGSO_time , &epoch );
  //(void)fprintf(stdout,"%f %s %f\n",time, MGSO_time, epoch);

  spkez_c  ( comet_id, epoch, "j2000", "NONE", -flyby_id, results, &lt );

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }

}

void compute_sun_state(int flyby_id, double time, double* sunstatevec )
{
  /* Returns state vector from flyby s/c to sun*/
  char         MGSO_time[1024];
  SpiceDouble  epoch;
  SpiceDouble  lt;

  strcpy( MGSO_time, SCET_PRINT_char_ptr( time ));

  /* Convert UTC epoch to ephemeris seconds past J2000 */
  str2et_c ( MGSO_time , &epoch );
  //(void)fprintf(stdout,"%f %s %f\n",time, MGSO_time, epoch);
  spkez_c  ( 10, epoch, "j2000", "NONE", -flyby_id, sunstatevec, &lt );
  //fprintf(stdout,"%d %s %f %f\n",-flyby_id,MGSO_time, epoch, sunstatevec[0]);

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}
void compute_impactor_state(int impactor_id, int flyby_id, double time, double*	impactorstatevec)
{
  char         MGSO_time[1024];
  SpiceDouble  epoch;
  SpiceDouble  lt;

  strcpy( MGSO_time, SCET_PRINT_char_ptr( time ));

  /* Convert UTC epoch to ephemeris seconds past J2000 */
  str2et_c ( MGSO_time , &epoch );
  //(void)fprintf(stdout,"%f %s %f\n",time, MGSO_time, epoch);
  spkez_c  ( -impactor_id, epoch, "j2000", "NONE", -flyby_id, impactorstatevec, &lt );

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}

// earthstatevec should have dimension at least 6
void compute_earth_state( int flyby_id, double time, double* earthstatevec)
{
  char         MGSO_time[1024];
  SpiceDouble  epoch;
  SpiceDouble  lt;

  strcpy( MGSO_time, SCET_PRINT_char_ptr( time ));

  /* Convert UTC epoch to ephemeris seconds past J2000 */
  str2et_c ( MGSO_time , &epoch );
  //(void)fprintf(stdout,"%f %s %f\n",time, MGSO_time, epoch);
  spkez_c  ( 399, epoch, "j2000", "NONE", -flyby_id, earthstatevec, &lt );
  //(void)fprintf(stdout,"time = %s flyby_id = %d epoch = %f earthstatevec = %f,%f,%f\n",MGSO_time,flyby_id,epoch,earthstatevec[0],earthstatevec[1],earthstatevec[2]);

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}


// statevec must have dimension at least 6
void compute_state( int CenterId, int TargetId, double time, double* statevec )
{
  char         MGSO_time[1024];
  SpiceDouble  epoch;
  SpiceDouble  lt;
  SpiceDouble  jed;

  strcpy( MGSO_time, SCET_PRINT_char_ptr( time ));

  /* Convert UTC epoch to ephemeris seconds past J2000 */
  str2et_c ( MGSO_time , &epoch );
  //(void)fprintf(stdout,"%f %s %f\n",time, MGSO_time, epoch);
  spkez_c  ( TargetId, epoch, "j2000", "NONE", CenterId, statevec, &lt );
  jed = unitim_c (  epoch,"TDB","JED");
  //(void)fprintf(stdout,"%s,%f,%f,%f,%d,%d,%f,%f,%f,%f,%f,%f\n",MGSO_time,epoch,jed,ET_to_VTC(epoch),CenterId,TargetId,statevec[0],statevec[1],statevec[2],statevec[3],statevec[4],statevec[5]);

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}
void compute_state_wframe( int CenterId, int TargetId, const char *frame, double time, double* statevec )
{
  char         MGSO_time[1024];
  SpiceDouble  epoch;
  SpiceDouble  lt;
  SpiceDouble  jed;

  strcpy( MGSO_time, SCET_PRINT_char_ptr( time ));

  /* Convert UTC epoch to ephemeris seconds past J2000 */
  str2et_c ( MGSO_time , &epoch );
  //(void)fprintf(stdout,"%f %s %f\n",time, MGSO_time, epoch);
  spkez_c  ( TargetId, epoch, frame, "NONE", CenterId, statevec, &lt );
  //jed = unitim_c (  epoch,"TDB","JED");
  //(void)fprintf(stdout,"%s,%f,%f,%f,%d,%d,%f,%f,%f,%f,%f,%f\n",MGSO_time,epoch,jed,ET_to_VTC(epoch),CenterId,TargetId,statevec[0],statevec[1],statevec[2],statevec[3],statevec[4],statevec[5]);

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}

double lspcn_api( const char *body, double utc, const char *abcorr)
{
  char         MGSO_time[1024];
  SpiceDouble  et;
  SpiceDouble  Longitude;

  strcpy( MGSO_time, SCET_PRINT_char_ptr( utc ));

  /* Convert UTC epoch to ephemeris seconds past J2000 */
  str2et_c ( MGSO_time , &et );

  Longitude = dpr_c()*lspcn_c( body, et, abcorr );
  (void)fprintf(stdout," UTC = %s, et = %f, Longitude = %f\n",MGSO_time, et, Longitude);

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
  return Longitude;
}
void vprjp_api( double positn[3], double normal[3], double point[3], double *proj )
{
  SpicePlane   plane;
  //(void)fprintf(stdout,"positn = %f %f %f normal = %f %f %f point = %f %f %f\n",positn[0],positn[1],positn[2],normal[0],normal[1],normal[2],point[0],point[1],point[2]);
  nvp2pl_c( normal, point, &plane);
  vprjp_c ( positn, &plane, proj ); 

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}
#ifdef MOVED_TO_SPKEZ_INTERFACE_C
void spkez_api( int targ, double utc, const char *ref, const char *abcorr, int obs, double *atarg, double *lt )
{
  char         MGSO_time[1024];
  SpiceDouble  et;

  strcpy( MGSO_time, SCET_PRINT_char_ptr( utc ));

  /* Convert UTC epoch to ephemeris seconds past J2000 */
  str2et_c ( MGSO_time , &et );

  //(void)fprintf(stdout,"%f %s %f\n",utc, MGSO_time, et);

  spkez_c  ( targ, et, ref, abcorr, obs, atarg, lt );

  //(void)fprintf(stdout,"%s,%f,%d,%d,%f,%f,%f,%f,%f,%f\n",MGSO_time,et,obs,targ,atarg[0],atarg[1],atarg[2],atarg[3],atarg[4],atarg[5]);

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}
#endif /* MOVED_TO_SPKEZ_INTERFACE_C */

void spkcpt_api( double trgpos_x, double trgpos_y, double trgpos_z, const char* trgctr, const char *trgref, double utc,
                 const char *outref, const char *refloc, const char *abcorr, const char *obs, double *state, double *lt )
{
  SpiceDouble  trgpos  [ 3 ];
  char         MGSO_time[1024];
  SpiceDouble  et;
  
  trgpos[0] = trgpos_x;
  trgpos[1] = trgpos_y;
  trgpos[2] = trgpos_z;
  
  /* Convert UTC epoch to ephemeris seconds past J2000 */
  strcpy( MGSO_time, SCET_PRINT_char_ptr( utc ));
  str2et_c ( MGSO_time , &et );

  spkcpt_c  ( trgpos, trgctr, trgref, et, outref, refloc, abcorr, obs, state, lt );


  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}

void latrec_api( double radius, double longitude, double latitude, double * rectan )
{
 
  latrec_c( radius, longitude, latitude, rectan);

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }

}
void reclat_api( double rectan_x, double rectan_y, double rectan_z, double* radius, double* longitude, double* latitude)
{
 
  SpiceDouble  rectan  [ 3 ];
  
  rectan[0] = rectan_x;
  rectan[1] = rectan_y;
  rectan[2] = rectan_z;

  reclat_c( rectan, radius, longitude, latitude );

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }

}
void recsph_api( double rectan_x, double rectan_y, double rectan_z, double* radius, double* colat, double* lon)
{
 
  SpiceDouble  rectan  [ 3 ];
  
  rectan[0] = rectan_x;
  rectan[1] = rectan_y;
  rectan[2] = rectan_z;

  recsph_c( rectan, radius, colat, lon );


  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}

void recgeo_api( double rectan_x, 
		 double rectan_y, 
		 double rectan_z, 
		 double re, 
		 double f, 
		 double* lon, 
		 double* lat, 
		 double* alt)
{
 
  SpiceDouble  rectan  [ 3 ];
  
  rectan[0] = rectan_x;
  rectan[1] = rectan_y;
  rectan[2] = rectan_z;

  recgeo_c( rectan, re, f, lon, lat,alt );
  //(void)fprintf(stdout,"\nrecgeo: x = %f,y =%f,z =%f,re =%f,f =%f\n",rectan_x,rectan_y,rectan_z,re,f);
  //(void)fprintf(stdout,"recgeo: lon =%f, lat=%f, alt=%f\n",*lon,*lat,*alt);

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}

void pxform_api( const char *from_frame , const char* to_frame, double utc, double cmat[3][3] ) 
{

    SpiceDouble  trgpos  [ 3 ];
    char         MGSO_time[1024];
    SpiceDouble  et;

    /* Convert UTC epoch to ephemeris seconds past J2000 */
    strcpy( MGSO_time, SCET_PRINT_char_ptr( utc ));
    str2et_c ( MGSO_time , &et );

    pxform_c(from_frame, to_frame, et, cmat );

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }

}

void twovec_api( double axdef[3] ,int indexa, double plndef[3] ,int indexp, double cmat[3][3] ) 
{
    twovec_c(axdef, indexa, plndef, indexp, cmat);

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}
void m2eul_api(double r[3][3], int axis3, int axis2, int axis1, double angleArray[3] ){
  double angle3;
  double angle2;
  double angle1;
  //(void)fprintf(stdout,"m2eul_api: r = %f, %f, %f, %f, %f, %f, %f, %f, %f, %i, %i, %i\n",r[0][0],r[0][1],r[0][2],r[1][0],r[1][1],r[1][2],r[2][0],r[2][1],r[2][2],axis3, axis2, axis1);
  m2eul_c(r, axis3, axis2, axis1, &angle3, &angle2, &angle1);
  angleArray[0] = angle3;
  angleArray[1] = angle1;
  angleArray[2] = angle1;

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}
void q2m_api(double q[4], double r[3][3]){
    SpiceDouble q_spice[4];
    q_spice[0] = q[3];
    q_spice[1] = -q[0];
    q_spice[2] = -q[1];
    q_spice[3] = -q[2];
    q2m_c(q_spice, r );

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}
void m2q_api( double cmat[3][3], double quat[4] ) 
{
    SpiceDouble q_spice[4];
    //cout << "cmat = " << cmat[0][0] << "," << cmat[0][1] << "," << cmat[0][2] << "\n";
    //cout << "       " << cmat[1][0] << "," << cmat[1][1] << "," << cmat[1][2] << "\n";
    //cout << "       " << cmat[2][0] << "," << cmat[2][1] << "," << cmat[2][2] << "\n";
   m2q_c( cmat, q_spice );

    // Convert spice quaternion into engineering quaternion
    // Please refer to the m2q_c cspice documentation for more information on this transformation
    quat[0] = -q_spice[1];
    quat[1] = -q_spice[2];
    quat[2] = -q_spice[3];
    quat[3] = q_spice[0];

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
    
}

void bodvrd_api( const char* body_name, const char* item, int maxn, int* dim, double* values ) 
{
    bodvrd_c(body_name, item, maxn, dim, values);

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}
                   
void sincpt_api( const char* method, 
                 const char* target, 
                 double utc, 
                 const char* fixref, 
                 const char* abcorr, 
                 const char* obsrvr, 
                 const char* dref, 
                 double dvec[3], 
                 double spoint[3], 
                 double* trgepc, 
                 double srfvec[3],
                 int* found)
{
    char MGSO_time[1024];
    SpiceDouble  et;
    strcpy( MGSO_time, SCET_PRINT_char_ptr( utc ));
    /* Convert UTC epoch to ephemeris seconds past J2000 */
    str2et_c ( MGSO_time , &et );
    
    sincpt_c(method, target, et, fixref, abcorr, obsrvr, dref, dvec, spoint, trgepc, srfvec, found);  

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}

void subpnt_api( const char* method, 
                 const char* target, 
                 double utc, 
                 const char* fixref, 
                 const char* abcorr, 
                 const char* obsrvr, 
                 double spoint[3], 
                 double* trgepc, 
                 double srfvec[3] )
{
    char MGSO_time[1024];
    SpiceDouble  et;
    strcpy( MGSO_time, SCET_PRINT_char_ptr( utc ));
    /* Convert UTC epoch to ephemeris seconds past J2000 */
    str2et_c ( MGSO_time , &et );
    
    subpnt_c(method, target, et, fixref, abcorr, obsrvr, spoint, trgepc, srfvec);  

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}

void ilumin_api( const char* method, 
                 const char* target, 
                 double utc, 
                 const char* fixref, 
                 const char* abcorr, 
                 const char* obsrvr, 
                 double spoint[3], 
                 double* trgepc,
                 double srfvec[3],
                 double* phase,
                 double* solar,
                 double* emissn )
{
    char MGSO_time[1024];
    SpiceDouble  et;
    strcpy( MGSO_time, SCET_PRINT_char_ptr( utc ));
    /* Convert UTC epoch to ephemeris seconds past J2000 */
    str2et_c ( MGSO_time , &et );
    
    ilumin_c(method, target, et, fixref, abcorr, obsrvr, spoint, trgepc, srfvec, phase, solar, emissn);

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}


void compute_ground_station_state( int flyby_id, int StationId, double time, double* stationstatevec)
{
  char         MGSO_time[1024];
  SpiceDouble  epoch;
  SpiceDouble  lt;

  strcpy( MGSO_time, SCET_PRINT_char_ptr( time ));

  /* Convert UTC epoch to ephemeris seconds past J2000 */
  str2et_c ( MGSO_time , &epoch );
  //(void)fprintf(stdout,"%f %s %f\n",time, MGSO_time, epoch);
  spkez_c  ( StationId, epoch, "j2000", "NONE", -flyby_id, stationstatevec, &lt );

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}


void compute_moon_state( int flyby_id, double time, double* moonstatevec_dim6)
{
  char         MGSO_time[1024];
  SpiceDouble  epoch;
  SpiceDouble  lt;

  strcpy( MGSO_time, SCET_PRINT_char_ptr( time ));

  /* Convert UTC epoch to ephemeris seconds past J2000 */
  str2et_c ( MGSO_time , &epoch );
  //(void)fprintf(stdout,"%f %s %f\n",time, MGSO_time, epoch);
  spkez_c  ( 301, epoch, "j2000", "NONE", -flyby_id, moonstatevec_dim6, &lt );

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}

#define station_range results[0]
#define station_azimuth results[1]
#define station_elevation results[2]
void compute_station_angles( double time, int NaifID, const char *frame, int sc_id, double* results )
{
  char         MGSO_time[1024];
  SpiceDouble  epoch;
  SpiceDouble  lt;
  SpiceDouble  state  [ 6 ];

  strcpy( MGSO_time, SCET_PRINT_char_ptr( time ));

  /* Convert UTC epoch to ephemeris seconds past J2000 */
  str2et_c ( MGSO_time , &epoch );
  //(void)fprintf(stdout,"%f %s %f\n",time, MGSO_time, epoch);
  spkez_c  (-sc_id, epoch, frame, "LT+S",  NaifID, state, &lt );
  recrad_c ( state,  &station_range,  &station_azimuth,     &station_elevation );
  station_elevation = station_elevation * dpr_c();
  station_azimuth = 360.0 - station_azimuth * dpr_c();
  //  (void)fprintf(stdout,"%s %d %s %d %f %f\n",MGSO_time,NaifID,frame,sc_id,station_azimuth,station_elevation);

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}

double compute_light_time( double time, int Observer, const char *Direction, int Target )
{
  char           MGSO_time[1024];
  SpiceDouble    epoch;
  SpiceInt       targ;
  SpiceInt       obs;
  SpiceDouble    ettarg;
  SpiceDouble    elapsd; 

  strcpy( MGSO_time, SCET_PRINT_char_ptr( time ));
  /* Convert UTC epoch to ephemeris seconds past J2000 */
  str2et_c ( MGSO_time , &epoch );
  //(void)fprintf(stdout,"compute_light_time: %f %s %f\n",time, MGSO_time, epoch);
  
  targ = Target;
  obs = Observer;
  ltime_c( epoch, obs, Direction, targ, &ettarg, &elapsd );
  //(void)fprintf(stdout,"exiting compute_light_time: %f\n",elapsd);

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
  return elapsd;
}


double compute_sep_angle( int body1, int body2, int body3, double time )
{
  char         MGSO_time[1024];
  SpiceDouble  epoch;
  SpiceInt     targ;
  SpiceInt     obs;
  SpiceDouble  lt;
  SpiceDouble  Body1State  [ 6 ];
  SpiceDouble  Body3State  [ 6 ];
  SpiceDouble  sep;

  strcpy( MGSO_time, SCET_PRINT_char_ptr( time ));
  /* Convert UTC epoch to ephemeris seconds past J2000 */
  str2et_c ( MGSO_time , &epoch );
  //(void)fprintf(stdout,"%f %s %f\n",time, MGSO_time, epoch);

  obs = body2;
  targ = body1;

  spkez_c  ( targ, epoch, "j2000", "NONE", obs, Body1State, &lt );

  targ = body3;
  spkez_c  ( targ, epoch, "j2000", "NONE", obs, Body3State, &lt );

  std::cout << "Moon-Sun Vector " << Body1State[0] << " " <<  Body1State[1] << " " <<  Body1State[2] <<"\n";
  std::cout << "Moon-S/C Vector "<< Body3State[0] << " " <<  Body3State[1] << " " <<  Body3State[2] <<"\n";

  sep = vsep_c( Body1State, Body3State );

  std::cout << "sep = " << sep <<  " " << sep * 180.0/3.1415926 << "\n";

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
  return sep * 180.0/3.1415926;
}

static  char         MGSO_time[1024];
static SpiceDouble  cmat[3][3],av[3],q[4];
static SpiceDouble  FlybyAttX[3];
static SpiceDouble  FlybyAttY[3];
static SpiceDouble  FlybyAttZ[3];
static SpiceDouble  HGAZAngle;
static SpiceDouble  HGAYAngle;

static SpiceDouble FSWElements[8];

extern void compute_Orb_Elements(double ET, double MU, double x, double y, double z, double xdot, double ydot, double zdot)
{
  double StateVector[6];
  double PI = 3.141592653;
  StateVector[0] = x;
  StateVector[1] = y;
  StateVector[2] = z;
  StateVector[3] = xdot;
  StateVector[4] = ydot;
  StateVector[5] = zdot;

  oscelt_c(StateVector, ET, MU, FSWElements);
  //(void)fprintf(stdout,"FSWElements = %f,%f,%f,%f,%f,%f,%f,%f\n",FSWElements[0],FSWElements[1],FSWElements[2]*180/PI,FSWElements[3]*180/PI,FSWElements[4]*180/PI,FSWElements[5]*180/PI,FSWElements[6],FSWElements[7]);

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}
extern double get_Orb_Elements(int i)
{
  return FSWElements[i];
}
static SpiceDouble FSWState[8];

double get_FSW_state(int i)
{
  return (FSWState[i]);
}

void compute_FSW_state(double RP, double ECC, double INC, double LNODE, double ARGP, double M0, double T0, double MU, double ET )
{
  SpiceDouble ELTS[8],ELTS1[8];
  SpiceDouble  lt;
  double PI = 3.141592653;
  ELTS[0] = RP;
  ELTS[1] = ECC;
  ELTS[2] = INC;
  ELTS[3] = LNODE;
  ELTS[4] = ARGP;
  ELTS[5] = M0;
  ELTS[6] = T0;
  ELTS[7] = MU;
  conics_c( ELTS, ET, FSWState);
  //(void)fprintf(stdout,"ET = %f\n",ET);
  //(void)fprintf(stdout,"ELTS = %f,%f,%f,%f,%f,%f,%f,%f\n",ELTS[0],ELTS[1],ELTS[2]*180/PI,ELTS[3]*180/PI,ELTS[4]*180/PI,ELTS[5]*180/PI,ELTS[6],ELTS[7]);
  //(void)fprintf(stdout,"FSWState = %f,%f,%f,%f,%f,%f\n",FSWState[0],FSWState[1],FSWState[2],FSWState[3],FSWState[4],FSWState[5]);


  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}
void compute_flyby_state(int flyby_id, double time, double* theFlybyStateVec)
{
  char         MGSO_time[1024];
  SpiceDouble  epoch;
  SpiceDouble  lt;

  strcpy( MGSO_time, SCET_PRINT_char_ptr( time ));

  /* Convert UTC epoch to ephemeris seconds past J2000 */
  str2et_c ( MGSO_time , &epoch );
  //(void)fprintf(stdout,"%f %s %f\n",time, MGSO_time, epoch);
  spkez_c  ( -flyby_id, epoch, "j2000", "NONE", 10, theFlybyStateVec, &lt );
  //fprintf(stdout,"%d %s %f %f\n",-flyby_id,MGSO_time, epoch, theFlybyStateVec[0]);


  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}
char * get_ET_time(double time)
{
  char         MGSO_time[1024];
  static char         ET[1024];
  SpiceDouble  epoch;
  const SpiceChar *pictur = "YYYY-DOYTHR:MN:SC.### ::TDB";
  strcpy( MGSO_time, SCET_PRINT_char_ptr( time ));

  str2et_c ( MGSO_time , &epoch );
  //(void)fprintf(stdout,"%f %s %f\n",time, MGSO_time, epoch);
  timout_c ( epoch, pictur, 1024, ET );
  //(void)fprintf(stdout,"%f %s %f %s \n",time,MGSO_time,epoch,ET);

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
  return ET;
}

char * get_Current_UTC_time()
{
  time_t now;
  time_t *tp;
  tp = &now;
  static char s[1024];
  size_t SizeOfS;
  const char *fmt="%Y-%jT%H:%M:%S";
  now = time(tp);
  //(void)fprintf(stdout,"get_Current_UTC_time = %s\n",ctime(tp));
  SizeOfS = strftime(s, 1024, fmt, gmtime(tp));
  //(void)fprintf(stdout,"get_Current_UTC_time = %s\n",s);

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }

  return s;
}

void compute_flyby_orientation(int flyby_id, double time )
{
  //Used when S/C is on launch vehicle and have C-Kernel for ascent  
  SpiceDouble  epoch;
  SpiceDouble  sclkdp;
  SpiceDouble  tol = .1;
  SpiceBoolean found;
  SpiceInt     spacecraft_id = -flyby_id;
  SpiceInt     instid = -flyby_id*1000;
  SpiceDouble  clkout;

  strcpy( MGSO_time, SCET_PRINT_char_ptr( time ));

  /* Convert UTC epoch to ephemeris seconds past J2000 */

  str2et_c ( MGSO_time , &epoch );
  //(void)fprintf(stdout,"%f %s %f\n",time, MGSO_time, epoch);

  sce2c_c ( spacecraft_id, epoch, &sclkdp);

  //fprintf(stdout,"\nID = %d, epoch = %20.6f sclkdp = %20.6f\n",-flyby_id, epoch, sclkdp);
  
    pxform_c("J2000","DIF_SPACECRAFT", epoch, cmat );

    //  ckgpav_c ( instid, sclkdp, tol, "J2000", cmat, av, &clkout, &found);

    //  if ( found ){
    //fprintf(stdout,"cmat\n");
    //fprintf(stdout,"%20.6f, %20.6f, %20.6f\n",cmat[0][0],cmat[0][1],cmat[0][2]);
    //fprintf(stdout,"%20.6f, %20.6f, %20.6f\n",cmat[1][0],cmat[1][1],cmat[1][2]);
    //fprintf(stdout,"%20.6f, %20.6f, %20.6f\n",cmat[2][0],cmat[2][1],cmat[2][2]);
	// Convert the C Matrix to a Quaternion
    m2q_c( cmat,q );
    //fprintf(stdout,"\n q = %20.6f, %20.6f, %20.6f, %20.6f\n",q[0],q[1],q[2],q[3]);
	//  }
	//  else{
	//    fprintf(stdout,"Did not find s/c orientation at %20.6f\n",sclkdp);
	//  }
  //(void)fprintf(stdout, "FlybyX:   %s %f %f %f %d\n",MGSO_time,FlybyAttX[0],FlybyAttX[1],FlybyAttX[2],found);
  //(void)fprintf(stdout, "HRI:     %s %f %f %f\n",MGSO_time,ITSLOS[0],ITSLOS[1],ITSLOS[2]);
  // fprintf(stdout," instid = %d, tol = %20.6f : clkout = %20.6f, found = %d, cmat00 = %20.6f\n",instid,tol,clkout,found,cmat[0][0]);

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}

double get_Flyby_q(int i)
{
  return q[i];
}

// Slew calculations ref DI-341-02-DRAFT, Slew Tool for APGEN, Mike Hughes, 11/14/02
static SpiceDouble SlewAxis[3] = {0.0,0.0,0.0};
static SpiceDouble SlewAngle = 0.0;
static double SlewDuration = 0.0;

double Compute_Boresight_Angle( double time, char *frame, char* axis, int target )
{
  SpiceDouble  epoch;
  SpiceDouble  lt;
  SpiceDouble  TargetState[6];
  SpiceDouble  TargetPosition[3];
  SpiceDouble  UTargetPosition[3];
  SpiceDouble  TargetRange;
  SpiceDouble  TargetAngle;
  int          iaxis;
  int          sign;
  SpiceDouble  az,el,range;
  //(void)fprintf(stdout," %f %s %s %d\n",time,frame,axis,target);
  if ( strcmp( axis,"+X" ) == 0 ){

      iaxis = 0;
      sign = +1;
  }
  else if ( strcmp( axis,"-X" ) == 0 ){

      iaxis = 0;
      sign = -1;
  }
  else if ( strcmp( axis,"+Y" ) == 0 ){

      iaxis = 1;
      sign = +1;
  }
  else if ( strcmp( axis,"-Y" ) == 0 ){

      iaxis = 1;
      sign = -1;
  }
  else if ( strcmp( axis,"+Z" ) == 0 ){

      iaxis = 2;
      sign = +1;
  }

  else if ( strcmp( axis,"-Z" ) == 0 ){

      iaxis = 2;
      sign = -1;
      
  }
  //(void)fprintf(stdout,"%d %d\n",iaxis,sign);
  strcpy( MGSO_time, SCET_PRINT_char_ptr( time ));

  /* Convert UTC epoch to ephemeris seconds past J2000 */

  str2et_c ( MGSO_time , &epoch );
  //(void)fprintf(stdout,"%f %s %f\n",time, MGSO_time, epoch);
  spkez_c  ( target, epoch, frame, "NONE", -140, TargetState, &lt );
  TargetPosition[0] = TargetState[0];
  TargetPosition[1] = TargetState[1];
  TargetPosition[2] = TargetState[2];

  unorm_c( TargetPosition, UTargetPosition, &TargetRange);
  //recrad_c(UTargetPosition,&range,&az,&el);
  //(void)fprintf(stdout,"%s %d %f %f %f %f %f\n",MGSO_time,target,UTargetPosition[0],UTargetPosition[1],UTargetPosition[2],az*dpr_c(),el*dpr_c());
  TargetAngle = acos(UTargetPosition[iaxis])*dpr_c();

  if ( sign == -1 ){

    TargetAngle = 180.0 - TargetAngle;
  }
  //(void)fprintf(stdout,"%s: %f, %f, %f : %f\n",MGSO_time, UTargetPosition[0], UTargetPosition[1], UTargetPosition[2], TargetAngle);

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
  return TargetAngle;
}

// New spice event finding code
#define  CNVTOL  1.e-6
#define  MAXWIN  10000
#define  NINTVL  10000
#define  TIMFMT  "YYYY-DDDTHR:MN:SC.###"
#define  TIMLEN  22
void spkcov_api( int idcode, double *ETWindow )
{
  SpiceInt  niv,Lastniv;
  
  SpiceDouble             b;
  SpiceDouble             e;
  SpiceDouble             bTemp;
  SpiceDouble             eTemp;
  SpiceInt               ic;
  SpiceInt               count;
  SpiceInt               tcount;
  SpiceInt  fillen=256,typlen=256,srclen=256,handle;
  SpiceChar file[256],filtyp[256],source[256];
  SpiceBoolean found;
  SPICEDOUBLE_CELL ( cover, MAXWIN );
  scard_c  ( 0, &cover );

  ktotal_c ( "SPK",
             &tcount );
  fprintf(stdout,"Number of SPK files returned from ktotal_c %d\n",tcount);  

  Lastniv = 0;
  for ( count=0; count<= tcount; count++ ){

    kdata_c ( count,  "SPK", fillen,   typlen,  srclen, file,  filtyp, source,  &handle, &found );
    if ( strcmp( file, "" ) != 0 ){
      //fprintf(stdout,"found spk file ::%s::\n",file);

      spkcov_c ( file, idcode, &cover );
      niv = wncard_c ( &cover );
      //fprintf(stdout,"niv = %d\n",niv);
      if ( niv != Lastniv ){
	Lastniv = niv;
	fprintf(stdout,"SPK found for %d, name = %s, niv = %d\n",idcode,file,niv);
      }
    }
  }
  if ( niv == 0 ){
    fprintf(stdout,"no SPK found for %d\n",idcode);

  }
  else if ( niv == 1 ){

    wnfetd_c ( &cover, 0, &b, &e );
    ETWindow[0] = b;
    ETWindow[1] = e;
    fprintf(stdout,"SPK(s) found for %d, one Window = %f to %f\n",idcode,ETWindow[0],ETWindow[1]);
  }
  else if ( niv > 1 ){

    fprintf(stdout,"\n\n");
    b = 1577880069.184;
    e = 0.0;
    for ( ic=0; ic < niv; ic++){
      wnfetd_c ( &cover, ic, &bTemp, &eTemp );
      //fprintf(stdout,"ic = %d, bTemp = %f,eTemp = %f b = %f,e = %f\n",ic,bTemp,eTemp,b,e);
      if ( bTemp < b ){
	b = bTemp;
      }; 
      if ( eTemp > e ){
	e = eTemp;
      }
    }
    ETWindow[0] = b;
    ETWindow[1] = e;
    fprintf(stdout,"SPK(s) found for %d, more than one Window, there may be gaps... = %f to %f\n",idcode,ETWindow[0],ETWindow[1]);
  }

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}


double et2lst_api( SpiceDouble utc, const char* body, SpiceDouble lon, const char* type  )
{
    
    SpiceDouble  et;
    char         MGSO_time[1024];
    
    SpiceInt hr; 
    SpiceInt mn;
    SpiceInt sc;
    SpiceChar time[MAX_LENGTH];
    SpiceChar ampm[MAX_LENGTH];
    SpiceDouble lst24;
    SpiceInt bod_id;
    SpiceBoolean found;

    strcpy( MGSO_time, SCET_PRINT_char_ptr( utc ));
    str2et_c ( MGSO_time , &et );
    
    bodn2c_c(body, &bod_id, &found);
     
    et2lst_c ( et,
               bod_id,
               lon,
               type,
               MAX_LENGTH,
               MAX_LENGTH,
               &hr,
               &mn,
               &sc,
               time,
               ampm );

    // Convert hr, mn, sec to single hour based floating point number
    lst24 = (SpiceDouble)hr + (SpiceDouble)mn/60.0 + (SpiceDouble)sc/3600.0; 

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
    return lst24;
} 

  
void gfdist_api(  const char* target,              
                  const char* abcorr,
                  const char* obsrvr,
                  const char* relate,
                  SpiceDouble refval,
                  SpiceDouble adjust,
                  SpiceDouble step,
                  int nintvls,
                  SpiceDouble cnfine[2],
                  double *result,
                  int* numWindows )
{
    // Convert UTC times to ET times
    SpiceDouble  et0, et1;
    char         MGSO_timeb[1024];
    char         MGSO_timee[1024];

    strcpy( MGSO_timeb, SCET_PRINT_char_ptr( cnfine[0] ));
    str2et_c ( MGSO_timeb , &et0 );

    strcpy( MGSO_timee, SCET_PRINT_char_ptr( cnfine[1] ));
    str2et_c ( MGSO_timee , &et1 );

    // Create SPICE windows necessary for SPICE call based
    // on inputs.
    SPICEDOUBLE_CELL ( cnfine_spice, MAXWIN );
    SPICEDOUBLE_CELL ( result_spice, MAXWIN );
    scard_c ( 0, &cnfine_spice );
    scard_c ( 0, &result_spice );
    wninsd_c ( et0, et1, &cnfine_spice );
     
    gfdist_c( target,
              abcorr,
              obsrvr,
              relate, 
              refval, 
              adjust, 
              step, 
              nintvls, 
              &cnfine_spice, 
              &result_spice );

    // Place resulting windows into an array
    SpiceInt count = wncard_c(&result_spice);
    *numWindows = count;
    SpiceDouble  start,stop;  
    unsigned int jj = 0;
    for ( unsigned int ii = 0;  ii < count; ii++ ){
        wnfetd_c ( &result_spice, ii, &start, &stop );
	    result[jj] = start;
	    jj++;
	    result[jj] = stop;
	    jj++;
	}

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
	
}

void gfposc_api(  const char* target,  
                const char* frame,             
                const char* abcorr,
                const char* obsrvr,
                const char* crdsys,
                const char* coord,
                const char* relate,
                SpiceDouble refval,
                SpiceDouble adjust,
                SpiceDouble step,
                int nintvls,
                SpiceDouble cnfine[2],
                double *result,
                int* numWindows )
{
    // Convert UTC times to ET times
    SpiceDouble  et0, et1;
    char         MGSO_timeb[1024];
    char         MGSO_timee[1024];

    strcpy( MGSO_timeb, SCET_PRINT_char_ptr( cnfine[0] ));
    str2et_c ( MGSO_timeb , &et0 );

    strcpy( MGSO_timee, SCET_PRINT_char_ptr( cnfine[1] ));
    str2et_c ( MGSO_timee , &et1 );

    // Create SPICE windows necessary for SPICE call based
    // on inputs.
    SPICEDOUBLE_CELL ( cnfine_spice, MAXWIN );
    SPICEDOUBLE_CELL ( result_spice, MAXWIN );
    scard_c ( 0, &cnfine_spice );
    scard_c ( 0, &result_spice );
    wninsd_c ( et0, et1, &cnfine_spice );
     
    gfposc_c( target,
              frame,
              abcorr,
              obsrvr,
              crdsys,
              coord,
              relate, 
              refval, 
              adjust, 
              step, 
              nintvls, 
              &cnfine_spice, 
              &result_spice );

    // Place resulting windows into an array
    SpiceInt count = wncard_c(&result_spice);
    *numWindows = count;
    SpiceDouble  start,stop;  
    unsigned int jj = 0;
    for ( unsigned int ii = 0;  ii < count; ii++ ){
        wnfetd_c ( &result_spice, ii, &start, &stop );
	    result[jj] = start;
	    jj++;
	    result[jj] = stop;
	    jj++;
	}

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
	
}
                  
                    
void gfsep_api(  const char* targ1,
                 const char* shape1,
                 const char* frame1,
                 const char* targ2,
                 const char* shape2,
                 const char* frame2,
                 const char* abcorr,
                 const char* obsrvr,
                 const char* relate,
                 SpiceDouble refval,
                 SpiceDouble adjust,
                 SpiceDouble step,
                 int nintvls,
                 SpiceDouble cnfine[2],
                 double *result,
                 int* numWindows )
{
    // Convert UTC times to ET times
    SpiceDouble  et0, et1;
    char         MGSO_timeb[1024];
    char         MGSO_timee[1024];

    strcpy( MGSO_timeb, SCET_PRINT_char_ptr( cnfine[0] ));
    str2et_c ( MGSO_timeb , &et0 );

    strcpy( MGSO_timee, SCET_PRINT_char_ptr( cnfine[1] ));
    str2et_c ( MGSO_timee , &et1 );

    // Create SPICE windows necessary for SPICE call based
    // on inputs.
    SPICEDOUBLE_CELL ( cnfine_spice, MAXWIN );
    SPICEDOUBLE_CELL ( result_spice, MAXWIN );
    scard_c ( 0, &cnfine_spice );
    scard_c ( 0, &result_spice );
    wninsd_c ( et0, et1, &cnfine_spice );
    //fprintf(stdout," targ1 = %s shape1 = %s frame1 = %s targ2 = %s shape2 = %s frame2 = %s abcorr = %s obsrvr = %s relate = %s refval = %f adjust = %f step = %f nintvls = %d\n",targ1,shape1,frame1,targ2,shape2,frame2,abcorr, obsrvr, relate, refval, adjust, step, nintvls);
    gfsep_c ( targ1, 
              shape1, 
              frame1, 
              targ2, 
              shape2, 
              frame2, 
              abcorr, 
              obsrvr, 
              relate, 
              refval,
              adjust,
              step,
              nintvls,
              &cnfine_spice,
              &result_spice );

    // Place resulting windows into an array
    SpiceInt count = wncard_c(&result_spice);
    //fprintf(stdout," count = %d\n",count);
    *numWindows = count;
    SpiceDouble  start,stop;  
    unsigned int jj = 0;
    for ( unsigned int ii = 0;  ii < count; ii++ ){
        wnfetd_c ( &result_spice, ii, &start, &stop );
	    result[jj] = start;
	    jj++;
	    result[jj] = stop;
	    jj++;
	}

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}

void gfpa_api(   const char* target,
                 const char* illmn,
                 const char* abcorr,
                 const char* obsrvr,
                 const char* relate,
                 SpiceDouble refval,
                 SpiceDouble adjust,
                 SpiceDouble step,
                 int nintvls,
                 SpiceDouble cnfine[2],
                 double *result,
                 int* numWindows )
{
    // Convert UTC times to ET times
    SpiceDouble  et0, et1;
    char         MGSO_timeb[1024];
    char         MGSO_timee[1024];

    strcpy( MGSO_timeb, SCET_PRINT_char_ptr( cnfine[0] ));
    str2et_c ( MGSO_timeb , &et0 );

    strcpy( MGSO_timee, SCET_PRINT_char_ptr( cnfine[1] ));
    str2et_c ( MGSO_timee , &et1 );

    // Create SPICE windows necessary for SPICE call based
    // on inputs.
    SPICEDOUBLE_CELL ( cnfine_spice, MAXWIN );
    SPICEDOUBLE_CELL ( result_spice, MAXWIN );
    scard_c ( 0, &cnfine_spice );
    scard_c ( 0, &result_spice );
    wninsd_c ( et0, et1, &cnfine_spice );

    gfpa_c (  target, 
              illmn,  
              abcorr, 
              obsrvr, 
              relate, 
              refval,
              adjust,
              step,
              nintvls,
              &cnfine_spice,
              &result_spice );

    // Place resulting windows into an array
    SpiceInt count = wncard_c(&result_spice);
    *numWindows = count;
    SpiceDouble  start,stop;  
    unsigned int jj = 0;
    for ( unsigned int ii = 0;  ii < count; ii++ ){
        wnfetd_c ( &result_spice, ii, &start, &stop );
	    result[jj] = start;
	    jj++;
	    result[jj] = stop;
	    jj++;
	}

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}



void gfilum_api(  
                 const char* method,
                 const char* angtyp,
                 const char* target,
                 const char* illmn,
                 const char* fixref,
                 const char* abcorr,
                 const char* obsrvr,
                 double spoint[3],
                 const char* relate,
                 SpiceDouble refval,
                 SpiceDouble adjust,
                 SpiceDouble step,
                 int nintvls,
                 SpiceDouble cnfine[2],
                 double *result,
                 int* numWindows )
{
    // Convert UTC times to ET times
    SpiceDouble  et0, et1;
    char         MGSO_timeb[1024];
    char         MGSO_timee[1024];

    strcpy( MGSO_timeb, SCET_PRINT_char_ptr( cnfine[0] ));
    str2et_c ( MGSO_timeb , &et0 );

    strcpy( MGSO_timee, SCET_PRINT_char_ptr( cnfine[1] ));
    str2et_c ( MGSO_timee , &et1 );

    // Create SPICE windows necessary for SPICE call based
    // on inputs.
    SPICEDOUBLE_CELL ( cnfine_spice, MAXWIN );
    SPICEDOUBLE_CELL ( result_spice, MAXWIN );
    scard_c ( 0, &cnfine_spice );
    scard_c ( 0, &result_spice );
    wninsd_c ( et0, et1, &cnfine_spice );
                   
    gfilum_c(  method,
              angtyp,
              target, 
              illmn, 
              fixref, 
              abcorr, 
              obsrvr,
              spoint, 
              relate, 
              refval,
              adjust,
              step,
              nintvls,
              &cnfine_spice,
              &result_spice );

    // Place resulting windows into an array
    SpiceInt count = wncard_c(&result_spice);
    *numWindows = count;
    SpiceDouble  start,stop;  
    unsigned int jj = 0;
    for ( unsigned int ii = 0;  ii < count; ii++ ){
        wnfetd_c ( &result_spice, ii, &start, &stop );
	    result[jj] = start;
	    jj++;
	    result[jj] = stop;
	    jj++;
	}

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}


char gflst_target[1024];
char gflst_abcorr[1024];
char gflst_obsrvr[1024];
char gflst_fixref[1024];
double gflst_step = 0.0;
void gflst_api(   const char* target, 
                  const char* fixref,              
                  const char* abcorr,
                  const char* obsrvr,
                  const char* relate,
                  SpiceDouble refval,
                  SpiceDouble adjust,
                  SpiceDouble step,
                  int nintvls,
                  SpiceDouble cnfine[2],
                  double *result,
                  int* numWindows )
{    
    strcpy(gflst_target, target);
    strcpy(gflst_abcorr, abcorr);
    strcpy(gflst_obsrvr, obsrvr);
    strcpy(gflst_fixref, fixref);
    gflst_step = step;
    
    //(void)fprintf(stdout,"called gflst_api: gflst_target = %s \n", target);
    
    // Convert UTC times to ET times
    SpiceDouble  et0, et1;
    char         MGSO_timeb[1024];
    char         MGSO_timee[1024];

    strcpy( MGSO_timeb, SCET_PRINT_char_ptr( cnfine[0] ));
    str2et_c ( MGSO_timeb , &et0 );

    strcpy( MGSO_timee, SCET_PRINT_char_ptr( cnfine[1] ));
    str2et_c ( MGSO_timee , &et1 );
    
    //(void)fprintf(stdout,"called gflst_api: MGSO_timeb = %s, MGSO_timee = %s \n", MGSO_timeb, MGSO_timee);
    //(void)fprintf(stdout,"called gflst_api: et0 = %f, et1 = %f \n", et0, et1);

    // Create SPICE windows necessary for SPICE call based
    // on inputs.
    SPICEDOUBLE_CELL ( cnfine_spice, MAXWIN );
    SPICEDOUBLE_CELL ( result_spice, MAXWIN );
    scard_c ( 0, &cnfine_spice );
    scard_c ( 0, &result_spice );
    wninsd_c ( et0, et1, &cnfine_spice );
       
    gfuds_c ( compute_subsc_lst,
                   compute_subsc_lst_decr,
                   relate,
                   refval,
                   adjust,
                   step,
                   nintvls,
                   &cnfine_spice,
                   &result_spice );
     

    // Place resulting windows into an array
    SpiceInt count = wncard_c(&result_spice);
    *numWindows = count;
    SpiceDouble  start,stop;  
    unsigned int jj = 0;
    for ( unsigned int ii = 0;  ii < count; ii++ ){
        wnfetd_c ( &result_spice, ii, &start, &stop );
	    result[jj] = start;
	    jj++;
	    result[jj] = stop;
	    jj++;
	}
	

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}


void compute_subsc_lst( SpiceDouble et, SpiceDouble * value )
{    
    // Constants 
    SpiceInt obs;
    SpiceBoolean found;
    
    SpiceDouble spoint[3];
    SpiceDouble srfvec[3];
    SpiceDouble trgepc;
    SpiceDouble radius;
    SpiceDouble lon;
    SpiceDouble lat;
    SpiceChar time[MAX_LENGTH];
    SpiceChar ampm[MAX_LENGTH];
    SpiceInt hr; 
    SpiceInt mn;
    SpiceInt sc;
    SpiceDouble lst24;
    //(void)fprintf(stdout,"called compute_subsc_lst: et = %15.10f\n", et);
    
    // Compute sub-spacecraft point                                                 
    subpnt_c ( "Intercept: ellipsoid",
               gflst_target,
               et,
               gflst_fixref,
               gflst_abcorr,
               gflst_obsrvr,
               spoint,
               &trgepc,
               srfvec);
    //(void)fprintf(stdout,"called subpnt_c: spoint = %15.10f, %15.10f, %15.10f\n",spoint[0], spoint[1], spoint[2]);
    
    
    // Convert to planetocentric coordinates           
    reclat_c ( spoint,
               &radius,
               &lon,
               &lat  );
    //(void)fprintf(stdout,"called reclat_c: lon = %15.10f\n",lon);
    
    bodn2c_c(gflst_target, &obs, &found);
    
    // Compute LST
    et2lst_c ( et,
               obs,
               lon,
               "PLANETOCENTRIC",
               MAX_LENGTH,
               MAX_LENGTH,
               &hr,
               &mn,
               &sc,
               time,
               ampm );
    
    // Convert hr, mn, sec to single hour based floating point number
    lst24 = (SpiceDouble)hr + (SpiceDouble)mn/60.0 + (SpiceDouble)sc/3600.0; 
    //(void)fprintf(stdout,"called et2lst_c: lst24 = %15.10f\n",lst24);   
   
    *value = lst24;


  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}



void compute_subsc_lst_decr( void ( * udfuns ) ( SpiceDouble    et,
                                         SpiceDouble  * value ),
                            SpiceDouble    et,
                            SpiceBoolean * isdecr )
{
    // Timestep for numerical differentiation
    double dt = gflst_step;
    
    double lst1;
    double lst2;
    double lst3;
    
    double d_lst;
    
    // Calculate lst for three different time points
    ( * udfuns ) ( et - dt, &lst1);
    ( * udfuns ) ( et, &lst2);
    ( * udfuns ) ( et + dt, &lst3);
                                         
    // Calculate lst for three different time points                                     
    d_lst = (lst3-lst1)/(2.0*dt);
    
    if ( d_lst < 0.0) {
        *isdecr = SPICETRUE;
    }
    else {
        *isdecr = SPICEFALSE;
    }

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}

                  
// Custom function for calculating sub-spacecraft emission
// Unfortunately we need to define some global variables for this custom function to 
// access because the custom function signature can not pass in extra parameters such
// as target and abcorr
char gfemi_target[1024];
char gfemi_fixref[1024];
char gfemi_abcorr[1024];
char gfemi_obsrvr[1024];
void gfemi_api(   const char* target, 
                  const char* fixref,              
                  const char* abcorr,
                  const char* obsrvr,
                  const char* relate,
                  SpiceDouble refval,
                  SpiceDouble adjust,
                  SpiceDouble step,
                  int nintvls,
                  SpiceDouble cnfine[2],
                  double *result,
                  int* numWindows )
{
       
    strcpy(gfemi_target, target);
    strcpy(gfemi_fixref, fixref);
    strcpy(gfemi_abcorr, abcorr);
    strcpy(gfemi_obsrvr, obsrvr);
    
    // Convert UTC times to ET times
    SpiceDouble  et0, et1;
    char         MGSO_timeb[1024];
    char         MGSO_timee[1024];

    strcpy( MGSO_timeb, SCET_PRINT_char_ptr( cnfine[0] ));
    str2et_c ( MGSO_timeb , &et0 );

    strcpy( MGSO_timee, SCET_PRINT_char_ptr( cnfine[1] ));
    str2et_c ( MGSO_timee , &et1 );
    
    //(void)fprintf(stdout,"called gflst_api: MGSO_timeb = %s, MGSO_timee = %s \n", MGSO_timeb, MGSO_timee);
    //(void)fprintf(stdout,"called gflst_api: et0 = %f, et1 = %f \n", et0, et1);

    // Create SPICE windows necessary for SPICE call based
    // on inputs.
    SPICEDOUBLE_CELL ( cnfine_spice, MAXWIN );
    SPICEDOUBLE_CELL ( result_spice, MAXWIN );
    scard_c ( 0, &cnfine_spice );
    scard_c ( 0, &result_spice );
    wninsd_c ( et0, et1, &cnfine_spice );
       
    gfuds_c ( compute_subsc_emi,
                   compute_subsc_emi_decr,
                   relate,
                   refval,
                   adjust,
                   step,
                   nintvls,
                   &cnfine_spice,
                   &result_spice );
     

    // Place resulting windows into an array
    SpiceInt count = wncard_c(&result_spice);
    *numWindows = count;
    SpiceDouble  start,stop;  
    unsigned int jj = 0;
    for ( unsigned int ii = 0;  ii < count; ii++ ){
        wnfetd_c ( &result_spice, ii, &start, &stop );
	    result[jj] = start;
	    jj++;
	    result[jj] = stop;
	    jj++;
	}
	

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}



void compute_subsc_emi( SpiceDouble et, SpiceDouble * value )
{    
    // Constants     
    SpiceDouble spoint[3];
    SpiceDouble srfvec[3];
    SpiceDouble trgepc;
    SpiceDouble phase;
    SpiceDouble incdnc;
    SpiceDouble emissn;
    
    //(void)fprintf(stdout,"called compute_subsc_emi: et = %15.10f\n", et);
    
    // Compute sub-spacecraft point                                                 
    subpnt_c ( "Intercept: ellipsoid",
               gfemi_target,
               et,
               gfemi_fixref,
               gfemi_abcorr,
               gfemi_obsrvr,
               spoint,
               &trgepc,
               srfvec);
    //(void)fprintf(stdout,"called subpnt_c: spoint = %15.10f, %15.10f, %15.10f\n",spoint[0], spoint[1], spoint[2]);
    
    // Compute illumination angles
    ilumin_c ( "ELLIPSOID",
               gfemi_target,
               et,
               gfemi_fixref,
               gfemi_abcorr,
               gfemi_obsrvr,
               spoint,
               &trgepc,
               srfvec,
               &phase,
               &incdnc,
               &emissn  );
    
    //(void)fprintf(stdout,"called ilumin_c: emissn = %15.10f\n",emissn);   
   
    *value = emissn;


  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}


void compute_subsc_emi_decr( void ( * udfuns ) ( SpiceDouble    et,
                                         SpiceDouble  * value ),
                            SpiceDouble    et,
                            SpiceBoolean * isdecr )
{
    double PI = 3.141592653;
    
    // Timestep for numerical differentiation
    double dt = 5;
    
    double emi1;
    double emi2;
    double emi3;
    
    double d_emi;
    
    // Calculate lst for three different time points
    ( * udfuns ) ( et - dt, &emi1);
    ( * udfuns ) ( et, &emi2);
    ( * udfuns ) ( et + dt, &emi3);
                                         
    // Calculate lst for three different time points                                      
    d_emi = (emi3-emi1)/(2.0*dt);
    
    if ( d_emi < 0.0 ) {
        *isdecr = SPICETRUE;
    }
    else {
        *isdecr = SPICEFALSE;
    }

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}
                  
// Custom function for calculating sub-spacecraft incidence
char gfinc_target[1024];
char gfinc_fixref[1024];
char gfinc_abcorr[1024];
char gfinc_obsrvr[1024];
void gfinc_api(   const char* target, 
                  const char* fixref,               
                  const char* abcorr,
                  const char* obsrvr,
                  const char* relate,
                  SpiceDouble refval,
                  SpiceDouble adjust,
                  SpiceDouble step,
                  int nintvls,
                  SpiceDouble cnfine[2],
                  double *result,
                  int* numWindows )
{

    strcpy(gfinc_target, target);
    strcpy(gfinc_fixref, fixref);
    strcpy(gfinc_abcorr, abcorr);
    strcpy(gfinc_obsrvr, obsrvr);
    
    // Convert UTC times to ET times
    SpiceDouble  et0, et1;
    char         MGSO_timeb[1024];
    char         MGSO_timee[1024];

    strcpy( MGSO_timeb, SCET_PRINT_char_ptr( cnfine[0] ));
    str2et_c ( MGSO_timeb , &et0 );

    strcpy( MGSO_timee, SCET_PRINT_char_ptr( cnfine[1] ));
    str2et_c ( MGSO_timee , &et1 );
    
    //(void)fprintf(stdout,"called gflst_api: MGSO_timeb = %s, MGSO_timee = %s \n", MGSO_timeb, MGSO_timee);
    //(void)fprintf(stdout,"called gflst_api: et0 = %f, et1 = %f \n", et0, et1);

    // Create SPICE windows necessary for SPICE call based
    // on inputs.
    SPICEDOUBLE_CELL ( cnfine_spice, MAXWIN );
    SPICEDOUBLE_CELL ( result_spice, MAXWIN );
    scard_c ( 0, &cnfine_spice );
    scard_c ( 0, &result_spice );
    wninsd_c ( et0, et1, &cnfine_spice );
       
    gfuds_c ( compute_subsc_inc,
                   compute_subsc_inc_decr,
                   relate,
                   refval,
                   adjust,
                   step,
                   nintvls,
                   &cnfine_spice,
                   &result_spice );
     

    // Place resulting windows into an array
    SpiceInt count = wncard_c(&result_spice);
    *numWindows = count;
    SpiceDouble  start,stop;  
    unsigned int jj = 0;
    for ( unsigned int ii = 0;  ii < count; ii++ ){
        wnfetd_c ( &result_spice, ii, &start, &stop );
	    result[jj] = start;
	    jj++;
	    result[jj] = stop;
	    jj++;
	}
	

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}

void compute_subsc_inc( SpiceDouble et, SpiceDouble * value )
{    
    // Constants     
    SpiceDouble spoint[3];
    SpiceDouble srfvec[3];
    SpiceDouble trgepc;
    SpiceDouble phase;
    SpiceDouble incdnc;
    SpiceDouble emissn;
    
    //(void)fprintf(stdout,"called compute_subsc_emi: et = %15.10f\n", et);
    
    // Compute sub-spacecraft point                                                 
    subpnt_c ( "Intercept: ellipsoid",
               gfinc_target,
               et,
               gfinc_fixref,
               gfinc_abcorr,
               gfinc_obsrvr,
               spoint,
               &trgepc,
               srfvec);
    //(void)fprintf(stdout,"called subpnt_c: spoint = %15.10f, %15.10f, %15.10f\n",spoint[0], spoint[1], spoint[2]);
    
    // Compute illumination angles
    ilumin_c ( "ELLIPSOID",
               gfinc_target,
               et,
               gfinc_fixref,
               gfinc_abcorr,
               gfinc_obsrvr,
               spoint,
               &trgepc,
               srfvec,
               &phase,
               &incdnc,
               &emissn  );
    
    //(void)fprintf(stdout,"called ilumin_c: emissn = %15.10f\n",emissn);   
   
    *value = incdnc;


  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}


void compute_subsc_inc_decr( void ( * udfuns ) ( SpiceDouble    et,
                                         SpiceDouble  * value ),
                            SpiceDouble    et,
                            SpiceBoolean * isdecr )
{
    double PI = 3.141592653;
    
    // Timestep for numerical differentiation
    double dt = 5;
    
    double inc1;
    double inc2;
    double inc3;
    
    double d_inc;
    
    // Calculate lst for three different time points
    ( * udfuns ) ( et - dt, &inc1);
    ( * udfuns ) ( et, &inc2);
    ( * udfuns ) ( et + dt, &inc3);
                                         
    // Calculate lst for three different time points                                     
    d_inc = (inc3-inc1)/(2.0*dt);
    
    if ( d_inc < 0.0 ) {
        *isdecr = SPICETRUE;
    }
    else {
        *isdecr = SPICEFALSE;
    }

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}


// Custom function for calculating lit limb longitude
char gflllon_target[1024];
char gflllon_fixref[1024];
char gflllon_abcorr[1024];
char gflllon_obsrvr[1024];
double gflllon_step = 0.0;
void gflllon_api(   const char* target, 
                  const char* fixref,               
                  const char* abcorr,
                  const char* obsrvr,
                  const char* relate,
                  SpiceDouble refval,
                  SpiceDouble adjust,
                  SpiceDouble step,
                  int nintvls,
                  SpiceDouble cnfine[2],
                  double *result,
                  int* numWindows )
{

    strcpy(gflllon_target, target);
    strcpy(gflllon_fixref, fixref);
    strcpy(gflllon_abcorr, abcorr);
    strcpy(gflllon_obsrvr, obsrvr);
    gflllon_step = step;
       
    // Convert UTC times to ET times
    SpiceDouble  et0, et1;
    char         MGSO_timeb[1024];
    char         MGSO_timee[1024];

    strcpy( MGSO_timeb, SCET_PRINT_char_ptr( cnfine[0] ));
    str2et_c ( MGSO_timeb , &et0 );

    strcpy( MGSO_timee, SCET_PRINT_char_ptr( cnfine[1] ));
    str2et_c ( MGSO_timee , &et1 );
    
    //(void)fprintf(stdout,"called gflllon_api: MGSO_timeb = %s, MGSO_timee = %s \n", MGSO_timeb, MGSO_timee);
    //(void)fprintf(stdout,"called gflllon_api: et0 = %f, et1 = %f \n", et0, et1);
    
    // Create SPICE windows necessary for SPICE call based
    // on inputs.
    SPICEDOUBLE_CELL ( cnfine_spice, MAXWIN );
    SPICEDOUBLE_CELL ( result_spice, MAXWIN );
    scard_c ( 0, &cnfine_spice );
    scard_c ( 0, &result_spice );
    wninsd_c ( et0, et1, &cnfine_spice );
       
    gfuds_c ( compute_subsc_litlimblon,
                   compute_subsc_litlimblon_decr,
                   relate,
                   refval,
                   adjust,
                   step,
                   nintvls,
                   &cnfine_spice,
                   &result_spice );
     	
    // Place resulting windows into an array
    SpiceInt count = wncard_c(&result_spice);
    *numWindows = count;
    SpiceDouble  start,stop;  
    unsigned int jj = 0;
    for ( unsigned int ii = 0;  ii < count; ii++ ){
        wnfetd_c ( &result_spice, ii, &start, &stop );
	    result[jj] = start;
	    jj++;
	    result[jj] = stop;
	    jj++;
	}

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}


void compute_subsc_litlimblon( SpiceDouble et, SpiceDouble * value )
{    
    double PI = 3.141592653;
    
    // Constants     
    SpiceDouble pos[3];
    SpiceDouble sunpos[3];
    SpiceDouble lt;
    SpiceDouble obsdist;
    SpiceDouble radius;
    SpiceDouble sunlon;
    SpiceDouble obslon;
    SpiceDouble lat;
    SpiceDouble delta_lon;
    SpiceDouble lon1;
    SpiceDouble lon2;
    SpiceDouble dff1;
    SpiceDouble dff2;
    SpiceDouble radii[3];
    SpiceInt n;
    
    SpiceDouble litlon;
    
    //(void)fprintf(stdout,"called compute_subsc_litlimblon: et = %f \n", et);
    
    // Compute position and longitude of the Sun wrt target body
    spkpos_c ( "SUN",  
               et, 
               gflllon_fixref, 
               gflllon_abcorr, 
               gflllon_target, 
               sunpos, 
               &lt);
               
    // Convert to planetocentric coordinates           
    reclat_c ( sunpos,
               &radius,
               &sunlon,
               &lat  );
               
    // Keep longitude between 0 and 2 pi
    if ( sunlon < 0.0 ) {
        sunlon = sunlon + 2.0*PI;
    }     
    
    // Compute position and longitude of observer wrt target body           
    spkpos_c ( gflllon_obsrvr,  
               et, 
               gflllon_fixref, 
               gflllon_abcorr, 
               gflllon_target,
               pos, 
               &lt);
               
    obsdist =  vnorm_c(pos);
               
    // Convert to planetocentric coordinates           
    reclat_c ( pos,
               &radius,
               &obslon,
               &lat  );
               
    // Keep longitude between 0 and 2 pi
    if ( obslon < 0.0 ) {
        obslon = obslon + 2.0*PI;
    }  
    
    // Compute observer-target-limb angle
    bodvrd_c ( gflllon_target, "RADII", 3, &n, radii );   
    delta_lon = acos( (radii[0] + radii[1]) / 2.0 / obsdist );

    // Compute both limb longitudes
    lon1 = (obslon + delta_lon);
    lon2 = (obslon - delta_lon);

    // Ensure they are within [0,2*PI)
    if (lon1 < 0.0) lon1 = lon1 + 2.0*PI;
    else if (lon1 > 2.0*PI) lon1 = lon1 - 2.0*PI;
    
    if (lon2 < 0.0) lon2 = lon2 + 2.0*PI;
    else if (lon2 > 2.0*PI) lon2 = lon2 - 2.0*PI;

    // Determine which limb longitude is closer to the Sun
    dff1 = fabs(sunlon - lon1); 
    dff2 = fabs(sunlon - lon2);

    if (dff1 > PI) dff1 = 2.0*PI - dff1;
    if (dff2 > PI) dff2 = 2.0*PI - dff2;

    // Assign the limb longitude that's closer to the Sun to the litlon variable
    // ie, the "Sunlit side of the target's disc"
    if (dff1 < dff2) litlon = lon1;
    else litlon = lon2;
    
    //(void)fprintf(stdout,"  litlon = %f \n", litlon);
    
    *value = litlon;


  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}


void compute_subsc_litlimblon_decr( void ( * udfuns ) ( SpiceDouble    et,
                                         SpiceDouble  * value ),
                            SpiceDouble    et,
                            SpiceBoolean * isdecr )
{

    //(void)fprintf(stdout,"called compute_subsc_litlimblon_decr: et = %f \n", et);
    double PI = 3.141592653;
    
    // Timestep for numerical differentiation
    double dt = gflllon_step;
    
    double inc1;
    double inc2;
    double inc3;
    
    double d_inc;
    
    // Calculate lst for three different time points
    ( * udfuns ) ( et - dt, &inc1);
    ( * udfuns ) ( et, &inc2);
    ( * udfuns ) ( et + dt, &inc3);
                                         
    // Calculate derivative for lit limb longitude                                    
    d_inc = (inc3-inc1)/(2.0*dt);
    
    if ( d_inc < 0.0 ) {
        *isdecr = SPICETRUE;
    }
    else {
        *isdecr = SPICEFALSE;
    }
    //(void)fprintf(stdout,"  isdecr = %i \n", *isdecr);

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}



// Custom function for calculating near terminator longitude
char gftermlon_target[1024];
char gftermlon_fixref[1024];
char gftermlon_abcorr[1024];
char gftermlon_obsrvr[1024];
double gftermlon_step = 0.0;
void gftermlon_api(   const char* target, 
                  const char* fixref,               
                  const char* abcorr,
                  const char* obsrvr,
                  const char* relate,
                  SpiceDouble refval,
                  SpiceDouble adjust,
                  SpiceDouble step,
                  int nintvls,
                  SpiceDouble cnfine[2],
                  double *result,
                  int* numWindows )
{

    strcpy(gftermlon_target, target);
    strcpy(gftermlon_fixref, fixref);
    strcpy(gftermlon_abcorr, abcorr);
    strcpy(gftermlon_obsrvr, obsrvr);
    gftermlon_step = step;
       
    // Convert UTC times to ET times
    SpiceDouble  et0, et1;
    char         MGSO_timeb[1024];
    char         MGSO_timee[1024];

    strcpy( MGSO_timeb, SCET_PRINT_char_ptr( cnfine[0] ));
    str2et_c ( MGSO_timeb , &et0 );

    strcpy( MGSO_timee, SCET_PRINT_char_ptr( cnfine[1] ));
    str2et_c ( MGSO_timee , &et1 );
    
    //(void)fprintf(stdout,"called gftermlon_api: MGSO_timeb = %s, MGSO_timee = %s \n", MGSO_timeb, MGSO_timee);
    //(void)fprintf(stdout,"called gftermlon_api: et0 = %f, et1 = %f \n", et0, et1);
    
    // Create SPICE windows necessary for SPICE call based
    // on inputs.
    SPICEDOUBLE_CELL ( cnfine_spice, MAXWIN );
    SPICEDOUBLE_CELL ( result_spice, MAXWIN );
    scard_c ( 0, &cnfine_spice );
    scard_c ( 0, &result_spice );
    wninsd_c ( et0, et1, &cnfine_spice );
       
    gfuds_c ( compute_subsc_termlon,
                   compute_subsc_termlon_decr,
                   relate,
                   refval,
                   adjust,
                   step,
                   nintvls,
                   &cnfine_spice,
                   &result_spice );
      
    // Place resulting windows into an array
    SpiceInt count = wncard_c(&result_spice);
    *numWindows = count;
    SpiceDouble  start,stop;  
    unsigned int jj = 0;
    for ( unsigned int ii = 0;  ii < count; ii++ ){
        wnfetd_c ( &result_spice, ii, &start, &stop );
      result[jj] = start;
      jj++;
      result[jj] = stop;
      jj++;
  }

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}


void compute_subsc_termlon( SpiceDouble et, SpiceDouble * value )
{    
    double PI = 3.141592653;
    
    // Constants     
    SpiceDouble pos[3];
    SpiceDouble sunpos[3];
    SpiceDouble lt;
    SpiceDouble obsdist;
    SpiceDouble radius;
    SpiceDouble sunlon;
    SpiceDouble obslon;
    SpiceDouble lat;
    SpiceDouble lon1;
    SpiceDouble lon2;
    SpiceDouble dff1;
    SpiceDouble dff2;
    SpiceDouble radii[3];
    SpiceInt n;
    
    SpiceDouble termlon;
    
    //(void)fprintf(stdout,"called compute_subsc_litlimblon: et = %f \n", et);
    
    // Compute position and longitude of the Sun wrt target body
    spkpos_c ( "SUN",  
               et, 
               gftermlon_fixref, 
               gftermlon_abcorr, 
               gftermlon_target, 
               sunpos, 
               &lt);
               
    // Convert to planetocentric coordinates           
    reclat_c ( sunpos,
               &radius,
               &sunlon,
               &lat  );
               
    // Keep longitude between 0 and 2 pi
    if ( sunlon < 0.0 ) {
        sunlon = sunlon + 2.0*PI;
    }     
    
    // Compute position and longitude of observer wrt target body           
    spkpos_c ( gftermlon_obsrvr,  
               et, 
               gftermlon_fixref, 
               gftermlon_abcorr, 
               gftermlon_target,
               pos, 
               &lt);
               
    obsdist =  vnorm_c(pos);
               
    // Convert to planetocentric coordinates           
    reclat_c ( pos,
               &radius,
               &obslon,
               &lat  );
               
    // Keep longitude between 0 and 2 pi
    if ( obslon < 0.0 ) {
        obslon = obslon + 2.0*PI;
    }  
    
    // Compute both terminator longitudes (as seem from sun)
    lon1 = (sunlon + PI/2.0);
    lon2 = (sunlon - PI/2.0);

    // Ensure they are within [0,2*PI)
    if (lon1 < 0.0) lon1 = lon1 + 2.0*PI;
    else if (lon1 > 2.0*PI) lon1 = lon1 - 2.0*PI;
    
    if (lon2 < 0.0) lon2 = lon2 + 2.0*PI;
    else if (lon2 > 2.0*PI) lon2 = lon2 - 2.0*PI;

    // Determine which terminator longitude is closer to the S/C pooint of view
    dff1 = fabs(obslon - lon1); 
    dff2 = fabs(obslon - lon2);

    if (dff1 > PI) dff1 = 2.0*PI - dff1;
    if (dff2 > PI) dff2 = 2.0*PI - dff2;

    // Assign the longitude that's closer to S/C Sun to the term variable
    if (dff1 < dff2) termlon = lon1;
    else termlon = lon2;
    
    //(void)fprintf(stdout,"  litlon = %f \n", litlon);
    
    *value = termlon;


  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}


void compute_subsc_termlon_decr( void ( * udfuns ) ( SpiceDouble    et,
                                         SpiceDouble  * value ),
                            SpiceDouble    et,
                            SpiceBoolean * isdecr )
{

    //(void)fprintf(stdout,"called compute_subsc_litlimblon_decr: et = %f \n", et);
    double PI = 3.141592653;
    
    // Timestep for numerical differentiation
    double dt = gftermlon_step;
    
    double inc1;
    double inc2;
    double inc3;
    
    double d_inc;
    
    // Calculate lst for three different time points
    ( * udfuns ) ( et - dt, &inc1);
    ( * udfuns ) ( et, &inc2);
    ( * udfuns ) ( et + dt, &inc3);
                                         
    // Calculate derivative for lit limb longitude                                    
    d_inc = (inc3-inc1)/(2.0*dt);
    
    if ( d_inc < 0.0 ) {
        *isdecr = SPICETRUE;
    }
    else {
        *isdecr = SPICEFALSE;
    }
    //(void)fprintf(stdout,"  isdecr = %i \n", *isdecr);

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}


void Compute_Closest_Approach_Times(  char *obsrvr, char *target, char* relate, SpiceDouble refval, SpiceDouble IntervalStart, SpiceDouble IntervalEnd, SpiceDouble StepSize, double *Closest_Aproach_Times, int* actualNumberOfWindows)
{

  SpiceDouble  et0,et1;
  char         MGSO_time[1024];
  SpiceDouble  adjust = 0.0;
  SpiceInt     i,j;
  SpiceDouble  start,stop;
  SpiceDouble  dist;
  SpiceDouble  pos[3];
  SpiceDouble  lt;

  SPICEDOUBLE_CELL ( cnfine, MAXWIN );
  SPICEDOUBLE_CELL ( result, MAXWIN );
  scard_c ( 0, &cnfine );
  scard_c ( 0, &result );

  strcpy( MGSO_time, SCET_PRINT_char_ptr( IntervalStart ));
  str2et_c ( MGSO_time , &et0 );

  strcpy( MGSO_time, SCET_PRINT_char_ptr( IntervalEnd ));
  str2et_c ( MGSO_time , &et1 );
  //(void)fprintf(stdout,"Entered Compute_Closest_Approach_Times: %s %s %f %f %f\n",obsrvr, target, et0, et1, StepSize);

  wninsd_c ( et0, et1, &cnfine );

  (void)gfdist_c( target,
		  "NONE",
		  obsrvr,
		  relate,
		  refval,
		  adjust,
		  StepSize,
		  NINTVL,
		  &cnfine,
		  &result );
		  
            if ( (*actualNumberOfWindows = wncard_c(&result)) == 0 ) 
            {
               printf ( "Result window is empty.\n\n" );
            }
            else
            {
	      //printf ( "Result window is wncard_c(&result) = %d\n\n",wncard_c(&result) );
	      //printf( "wncard_c(&result) = %d\n", wncard_c(&result)); 
	      //printf( "actualNumberOfWindows = %d\n", *actualNumberOfWindows); 

	       j = 0;
               for ( i = 0;  i < wncard_c(&result); i++ )
               {
		 wnfetd_c ( &result, i, &start, &stop );
		 //spkpos_c ( target,  start, "J2000", "NONE", obsrvr, pos, &lt);
		 //dist = vnorm_c(pos);
		 Closest_Aproach_Times[j]=start;
		 //et2utc_c ( start, "ISOD", 3, 1024, MGSO_time );
		 //(void)fprintf(stdout,"%d %s       %s Closest Approach %f\n",j,MGSO_time,obsrvr,dist);
		 j++;
		 //et2utc_c ( stop, "ISOD", 3, 1024, MGSO_time );
		 //(void)fprintf(stdout,"%d %s       %s Closest Approach %f\n",j,MGSO_time,obsrvr,dist);
		 Closest_Aproach_Times[j]=stop;
		 j++;
	       }
	    }

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}
void Compute_Range_Windows(  char *obsrvr, char *target, char *targetFrame, char* relate, SpiceDouble refval, SpiceDouble IntervalStart, SpiceDouble IntervalEnd, SpiceDouble StepSize, double *Closest_Aproach_Times, int* actualNumberOfWindows)
{

  SpiceDouble  et0,et1;
  char         MGSO_time[1024];
  SpiceDouble  adjust = 0.0;
  SpiceInt     i,j;
  SpiceDouble  start,stop;
  SpiceDouble  dist;
  SpiceDouble  pos[3];
  SpiceDouble  lt;

  SPICEDOUBLE_CELL ( cnfine, MAXWIN );
  SPICEDOUBLE_CELL ( result, MAXWIN );
  scard_c ( 0, &cnfine );
  scard_c ( 0, &result );

  strcpy( MGSO_time, SCET_PRINT_char_ptr( IntervalStart ));
  str2et_c ( MGSO_time , &et0 );

  strcpy( MGSO_time, SCET_PRINT_char_ptr( IntervalEnd ));
  str2et_c ( MGSO_time , &et1 );
  // (void)fprintf(stdout,"Entered Compute_Closest_Approach_Times: %s %s %f %f %f\n",obsrvr, target, et0, et1, StepSize);

  wninsd_c ( et0, et1, &cnfine );

  (void)gfposc_c( target,
		  targetFrame,
		  "NONE",
		  obsrvr,
		  "GEODETIC",
		  "ALTITUDE",
		  relate,
		  refval,
		  adjust,
		  StepSize,
		  NINTVL,
		  &cnfine,
		  &result );
		  
            if ( (*actualNumberOfWindows = wncard_c(&result)) == 0 ) 
            {
               printf ( "Result window is empty.\n\n" );
            }
            else
            {
	      //printf ( "Result window is wncard_c(&result) = %d\n\n",wncard_c(&result) );
	      //printf( "wncard_c(&result) = %d\n", wncard_c(&result)); 
	      //printf( "actualNumberOfWindows = %d\n", *actualNumberOfWindows); 

	       j = 0;
               for ( i = 0;  i < wncard_c(&result); i++ )
               {
		 wnfetd_c ( &result, i, &start, &stop );
		 //spkpos_c ( target,  start, "J2000", "NONE", obsrvr, pos, &lt);
		 //dist = vnorm_c(pos);
		 Closest_Aproach_Times[j]=start;
		 //et2utc_c ( start, "ISOD", 3, 1024, MGSO_time );
		 //(void)fprintf(stdout,"%d %s       %s Closest Approach %f\n",j,MGSO_time,obsrvr,dist);
		 j++;
		 //et2utc_c ( stop, "ISOD", 3, 1024, MGSO_time );
		 //(void)fprintf(stdout,"%d %s       %s Closest Approach %f\n",j,MGSO_time,obsrvr,dist);
		 Closest_Aproach_Times[j]=stop;
		 j++;
	       }
	    }

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}


// void gfoclt_api( const char* occtyp,
//                  const char* front,
//                  const char* fshape,
//                  const char* fframe,
//                  const char* back,
//                  const char* bshape,
//                  const char* bframe,
//                  const char* abcorr,
//                  const char* obsrvr,
//                  SpiceDouble step,
//                  SpiceDouble cnfine[2],
//                  double *result,
//                  int* numWindows )
// {
//     // Convert UTC times to ET times
//     SpiceDouble  et0, et1;
//     char         MGSO_timeb[1024];
//     char         MGSO_timee[1024];
// 
//     strcpy( MGSO_timeb, SCET_PRINT_char_ptr( cnfine[0] ));
//     str2et_c ( MGSO_timeb , &et0 );
// 
//     strcpy( MGSO_timee, SCET_PRINT_char_ptr( cnfine[1] ));
//     str2et_c ( MGSO_timee , &et1 );
// 
//     // Create SPICE windows necessary for SPICE call based
//     // on inputs.
//     SPICEDOUBLE_CELL ( cnfine_spice, MAXWIN );
//     SPICEDOUBLE_CELL ( result_spice, MAXWIN );
//     scard_c ( 0, &cnfine_spice );
//     scard_c ( 0, &result_spice );
//     wninsd_c ( et0, et1, &cnfine_spice );
// 
//     gfpa_c (  target, 
//               illmn,  
//               abcorr, 
//               obsrvr, 
//               relate, 
//               refval,
//               adjust,
//               step,
//               nintvls,
//               &cnfine_spice,
//               &result_spice );
// 
//     // Place resulting windows into an array
//     SpiceInt count = wncard_c(&result_spice);
//     *numWindows = count;
//     SpiceDouble  start,stop;  
//     unsigned int jj = 0;
//     for ( unsigned int ii = 0;  ii < count; ii++ ){
//         wnfetd_c ( &result_spice, ii, &start, &stop );
// 	    result[jj] = start;
// 	    jj++;
// 	    result[jj] = stop;
// 	    jj++;
// 	}
// }

void Compute_Occultation_Times( char *OccultationType, char *obsrvr, char *target, char *TargetBodyFrame,  char* TargetShape, char *OccultingBody, char *OccultingBodyFrame, SpiceDouble IntervalStart, SpiceDouble IntervalEnd, SpiceDouble StepSize, double *Occultation_Times, int* actualNumberOfWindows )
{

  SpiceDouble  et0,et1;
  char         MGSO_time[1024];
  SpiceDouble  refval = 0.0;
  SpiceDouble  adjust = 0.0;
  SpiceInt     i;
  SpiceInt     j;
  SpiceDouble  start,stop;
  SpiceDouble  dist;
  SpiceDouble  pos[3];
  SpiceDouble  lt;

  SPICEDOUBLE_CELL ( cnfine, MAXWIN );
  SPICEDOUBLE_CELL ( result, MAXWIN );
  scard_c ( 0, &cnfine );
  scard_c ( 0, &result );

  strcpy( MGSO_time, SCET_PRINT_char_ptr( IntervalStart ));
  str2et_c ( MGSO_time , &et0 );

  strcpy( MGSO_time, SCET_PRINT_char_ptr( IntervalEnd ));
  str2et_c ( MGSO_time , &et1 );

  wninsd_c ( et0, et1, &cnfine );
  //(void)fprintf(stdout,"Entered Compute_Occultation_Times: %s %s %s %s %s %s %f %f %f\n",OccultationType,obsrvr, target, TargetShape, TargetBodyFrame, OccultingBody, OccultingBodyFrame, et0, et1, StepSize);

  (void)gfoclt_c( OccultationType,       //occtyp//
		  OccultingBody,         //front//
		  "ellipsoid",           //fshape//
		  OccultingBodyFrame,    //fframe//
		  target,                //back//
		  TargetShape,           //bshape//
		  TargetBodyFrame,       //bframe//
		  "CN",                  //abcorr//
		  obsrvr,                //obsrvr//
		  StepSize,              //step//
		  &cnfine,               //cnfine//
		  &result);              //result//
		  
            if ( (*actualNumberOfWindows = wncard_c(&result)) == 0 ) 
            {
               printf ( "Result window is empty.\n\n" );
            }
            else
            {
	      j = 0;
	      //printf( "wncard_c(&result) = %d\n", wncard_c(&result)); 
	      //printf( "actualNumberOfWindows = %d\n", actualNumberOfWindows); 

               for ( i = 0;  i < wncard_c(&result); i++ )
               {
		 wnfetd_c ( &result, i, &start, &stop );
		 Occultation_Times[j]=start;
		 j++;
		 Occultation_Times[j]=stop;
		 j++;
		 //et2utc_c ( start, "ISOD", 3, 1024, MGSO_time );
		 //(void)fprintf(stdout,"%s Occultation Event\n",MGSO_time);//
		 //et2utc_c ( stop, "ISOD", 3, 1024, MGSO_time );
		 //(void)fprintf(stdout,"%s Occultation Event\n",MGSO_time);//

	       }
	    }

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}

SpiceBoolean Compute_Surface_Intercept(
			double time,
			char *target, char *target_frame, char *obsrvr,
			double dvec0, double dvec1, double dvec2,
			double *dist, double *lat, double *lon, double *radius,
			double *PlanetographicLon, double *PlanetographicLat, double *PlanetographicAlt)
{
  char         MGSO_time[1024];
  SpiceDouble  et;
  SpiceBoolean found;
  SpiceDouble  dvec[3];
  SpiceDouble spoint[3];
  SpiceDouble srfvec[3];
  SpiceDouble trgepc;
  strcpy( MGSO_time, SCET_PRINT_char_ptr( time ));
  SpiceDouble radii[3];
  dvec[0] = dvec0;
  dvec[1] = dvec1;
  dvec[2] = dvec2;

  str2et_c ( MGSO_time , &et );

  //(void)fprintf(stdout,
  //		"about to call sincpt_c: UTC = %s, ET = %10.4f,
  //		target = %s, target_frame = %s, obsrvr = %s,
  //		dvec = %15.10f, %15.10f, %15.10f\n",
  //		MGSO_time, et, target, target_frame, obsrvr, dvec[0], dvec[1], dvec[2] );


  //
  // This method is sometimes 'unreasonable' because of the very
  // stringent accuracy requirements: it shoots a ray from the Sun
  // to the center of the target and computes the intersection with
  // the target's surface.  It would be more natural to shoot a
  // ray from the target's center towards the Sun.  However, this
  // requires some careful re-engineering of the calls to spice.
  // In addition, it is difficult to pinpoint the ratio (distance
  // to the target) over (distance to the Sun) at which the algo-
  // rithm becomes 'unreasonable'.  Therefore, we have decided to
  // leave the original algorithm untouched and live with the
  // high accuracy requirements.
  //
  sincpt_c ( "Ellipsoid", 
             target,
	     et,
	     target_frame,
	     "CN+S",
	     obsrvr,
	     "j2000",
	     dvec,	// given point from observer to target center
	     spoint,	// point on the surface body
	     &trgepc,	// intercept epoch
	     srfvec,	// vector from the observer to the intercept point
	     &found);


  //
  // Extracts target's RADII information from kernel pool.
  // This is a 3-dimensional vector of doubles which are
  // the values of the 3 radii associated with the ellipsoid
  // model of the target.
  //
  SpiceInt    n;
  bodvrd_c ( target, "RADII", 3, &n, radii );

  if ( found ){

    *dist = vnorm_c( srfvec );
    
    //
    // Method 1 provides spoint which lies on the target's surface
    //
    reclat_c ( spoint, radius, lon, lat );

    //
    // dpr = Degrees Per Radian
    //
    *lon *= dpr_c ();
    *lat *= dpr_c ();

    double re,rp,f;
    re  =  radii[0];
    rp  =  radii[2];
    f   =  ( re - rp ) / re;

    //
    // Rectangular to planetographic conversion
    //
    recpgr_c ( target,			// target body
	       spoint,			// rectangular coord. of the point of interest
	       re,			// equatorial radius
	       f,			// flattening coefficient
	       PlanetographicLon,	// output[0]
	       PlanetographicLat,	// output[1]
	       PlanetographicAlt);	// output[2] (altitude)


    //
    // dpr = Degrees Per Radian
    //
    *lon *= dpr_c ();
    *lat *= dpr_c ();
    *PlanetographicLon *= dpr_c();
    *PlanetographicLat *= dpr_c();

    //(void)fprintf(stdout,
    //	" MGSO_time, = %s, target = %s,
    //	lon = %f, lat = %f, dist = %f,
    //	PlanetographicLon = %f, PlanetographicLat = %f, PlanetographicAlt = %f\n",
    //	MGSO_time,target,
    //	*lon,*lat,*dist,
    //	*PlanetographicLon,*PlanetographicLat,*PlanetographicAlt);

  } else {
    *dist = 0.0;
    *lat = 0.0;
    *lon = 0.0;
    *radius = 0.0;
    *PlanetographicLon = 0.0;
    *PlanetographicLat = 0.0;
    *PlanetographicAlt = 0.0;
  }

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
  
  //(void)fprintf(stdout,"called sincpt_c: spoint = %15.10f, %15.10f, %15.10f, trgepc = %15.10f, srfvec = %15.10f, %15.10f, %15.10f, found = %d, dist = %15.10f, lon = %15.10f, lat = %15.10f\n",spoint[0], spoint[1], spoint[2], trgepc, srfvec[0], srfvec[1], srfvec[2],found,*dist, *lon, *lat);
  return found;
}

void Compute_Ang_Sep_Times(  char *targ1, char *shape1, char *frame1, char *targ2, char *shape2, char *frame2, char *obsrvr, char *relate , SpiceDouble refval, SpiceDouble IntervalStart, SpiceDouble IntervalEnd, SpiceDouble step, double *Sep_Times, int* actualNumberOfWindows )
{

  SpiceDouble  et0,et1;
  char         MGSO_timeb[1024];
  char         MGSO_timee[1024];
  SpiceDouble  adjust = 0.0;
  SpiceInt     i;
  SpiceInt     j;
  SpiceDouble  start,stop;
  SpiceDouble  lt;
  SpiceInt     count = 0;
  SPICEDOUBLE_CELL ( result, 2*MAXWIN );
  SPICEDOUBLE_CELL ( cnfine, 2 );
  scard_c ( 0, &cnfine );
  scard_c ( 0, &result );

  strcpy( MGSO_timeb, SCET_PRINT_char_ptr( IntervalStart ));
  str2et_c ( MGSO_timeb , &et0 );

  strcpy( MGSO_timee, SCET_PRINT_char_ptr( IntervalEnd ));
  str2et_c ( MGSO_timee , &et1 );


  wninsd_c ( et0, et1, &cnfine );



  refval = refval/ dpr_c();
  //(void)fprintf(stdout,"Compute_Ang_Sep_Times: IntervalStart=%s : %f IntervalEnd=%s : %f step = %f refval = %f relate = %s targ1 = %s targ2 = %s obsrvr = %s\n",MGSO_timeb,IntervalStart,MGSO_timee,IntervalEnd,step,refval,relate,targ1,targ2,obsrvr);

  (void)gfsep_c ( targ1,
                  shape1,
                  frame1,
                  targ2,
                  shape2,
                  frame2,
                  "LT+S",
                  obsrvr,
                  relate,
                  refval,
                  0.0,
                  step,
                  MAXWIN,
                  &cnfine,
                  &result );
  count = wncard_c(&result);
  *actualNumberOfWindows = count;
  if ( count == 0 ) {
    printf ( "Result window is empty.\n\n" );
  }
  else
    {
      j = 0;
      //printf( "wncard_c(&result) = %d\n", wncard_c(&result)); 
      //printf( "count = %d\n", count); 

      //printf( "actualNumberOfWindows = %d\n", *actualNumberOfWindows); 

      for ( i = 0;  i < count; i++ ){
	wnfetd_c ( &result, i, &start, &stop );
	Sep_Times[j]=start;
	j++;
	Sep_Times[j]=stop;
	j++;
	//et2utc_c ( start, "ISOD", 3, 1024, MGSO_time );
	//(void)fprintf(stdout,"%s Seperation Event Being\n",MGSO_time);//
	//et2utc_c ( stop, "ISOD", 3, 1024, MGSO_time );
	//(void)fprintf(stdout,"%s Seperation Event End\n",MGSO_time);//
	*actualNumberOfWindows = j/2;
      }
    }

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}
void Compute_Pole_Crossing_Times(  char *target, char *fixref, char *obsrvr, char *relate , SpiceDouble refval, SpiceDouble IntervalStart, SpiceDouble IntervalEnd, SpiceDouble step, double *Sep_Times, int* actualNumberOfWindows )
{

  SpiceDouble  et0,et1;
  char         MGSO_timeb[1024];
  char         MGSO_timee[1024];
  SpiceDouble  adjust = 0.0;
  SpiceInt     i;
  SpiceInt     j;
  SpiceDouble  start,stop;
  SpiceDouble  lt;
  SpiceInt     count = 0;
  SPICEDOUBLE_CELL ( result, 2*MAXWIN );
  SPICEDOUBLE_CELL ( cnfine, 2 );
  scard_c ( 0, &cnfine );
  scard_c ( 0, &result );
  SpiceChar       * method = "Near point: ellipsoid";
  SpiceChar       * crdsys = "GEODETIC";
  SpiceChar       * coord  = "LATITUDE";

  strcpy( MGSO_timeb, SCET_PRINT_char_ptr( IntervalStart ));
  str2et_c ( MGSO_timeb , &et0 );

  strcpy( MGSO_timee, SCET_PRINT_char_ptr( IntervalEnd ));
  str2et_c ( MGSO_timee , &et1 );
  wninsd_c ( et0, et1, &cnfine );
  refval = refval/ dpr_c();

  (void)fprintf(stdout,"Compute_Pole_Crossing_Times: IntervalStart=%s : %f IntervalEnd=%s : %f step = %f refval = %f relate = %s target = %s fixref = %s obsrvr = %s\n",MGSO_timeb,IntervalStart,MGSO_timee,IntervalEnd,step,refval,relate,target,fixref, obsrvr);

  gfsubc_c (  target,
	      fixref,
	      method,
	      "CN",
	      obsrvr,
	      crdsys,
	      coord,
	      relate,
	      refval,
	      adjust,
	      step,
	      MAXWIN,
	      &cnfine,
	      &result );

  count = wncard_c(&result);
  *actualNumberOfWindows = count;
  if ( count == 0 ) {
    printf ( "Result window is empty.\n\n" );
  }
  else
    {
      j = 0;
      //printf( "wncard_c(&result) = %d\n", wncard_c(&result)); 
      //printf( "count = %d\n", count); 

      //printf( "actualNumberOfWindows = %d\n", *actualNumberOfWindows); 

      for ( i = 0;  i < count; i++ ){
	wnfetd_c ( &result, i, &start, &stop );
	Sep_Times[j]=start;
	j++;
	Sep_Times[j]=stop;
	j++;
	//et2utc_c ( start, "ISOD", 3, 1024, MGSO_time );
	//(void)fprintf(stdout,"%s Seperation Event Being\n",MGSO_time);//
	//et2utc_c ( stop, "ISOD", 3, 1024, MGSO_time );
	//(void)fprintf(stdout,"%s Seperation Event End\n",MGSO_time);//
	*actualNumberOfWindows = j/2;
      }
    }

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}
void vzfrac_api(  int          frontID, 
		  const char * frontFrame,
		  double       frontRadii [3],
		  SpiceInt     backID, 
		  const char * backFrame,
		  double       backRadii  [3],
		  int          obsID, 
		  const char * abcorr,
		  double       time,
		  double     * frac)  
{
  char         MGSO_time[1024];
  SpiceDouble  et;

  strcpy( MGSO_time, SCET_PRINT_char_ptr( time ));

  /* Convert UTC epoch to ephemeris seconds past J2000 */
  str2et_c ( MGSO_time , &et );
  
  vzfrac(frontID, frontFrame, frontRadii, backID, backFrame, backRadii, obsID, abcorr, et, frac);

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}
void Compute_Ang_Sep_Vec_Times(  char *target,
				 char *frame, 
				 char *abcorr, 
				 char *obsrvr, 
				 char *crdsys, 
				 char *coord, 
				 char *relate , 
				 SpiceDouble refval, 
				 SpiceDouble adjust, SpiceDouble IntervalStart, SpiceDouble IntervalEnd, SpiceDouble step, double *Sep_Times, int* actualNumberOfWindows )
{

  SpiceDouble  et0,et1;
  char         MGSO_timeb[1024];
  char         MGSO_timee[1024];
  SpiceInt     i;
  SpiceInt     j;
  SpiceDouble  start,stop;
  SpiceDouble  lt;
  SpiceInt     count = 0;
  SPICEDOUBLE_CELL ( result, 2*MAXWIN );
  SPICEDOUBLE_CELL ( cnfine, 2 );
  scard_c ( 0, &cnfine );
  scard_c ( 0, &result );

  strcpy( MGSO_timeb, SCET_PRINT_char_ptr( IntervalStart ));
  str2et_c ( MGSO_timeb , &et0 );

  strcpy( MGSO_timee, SCET_PRINT_char_ptr( IntervalEnd ));
  str2et_c ( MGSO_timee , &et1 );

  wninsd_c ( et0, et1, &cnfine );

  refval = refval/ dpr_c();

  //(void)fprintf(stdout,"Compute_Ang_Sep_Vec_Times: IntervalStart=%s : %f IntervalEnd=%s : %f step = %f refval = %f relate = %s target = %s obsrvr = %s\n",MGSO_timeb,IntervalStart,MGSO_timee,IntervalEnd,step,refval,relate,target,obsrvr);

  (void)gfposc_c ( target,
                   frame,
                   abcorr,
                   obsrvr,
                   crdsys,
                   coord,
                   relate,
                   refval,
                   adjust,
                   step,
                   MAXWIN,
                   &cnfine,
                   &result  );

  count = wncard_c(&result);
  *actualNumberOfWindows = count;
  if ( count == 0 ) {
    printf ( "Result window is empty.\n\n" );
  }
  else
    {
      j = 0;
      //printf( "wncard_c(&result) = %d\n", wncard_c(&result)); 
      //printf( "count = %d\n", count); 

      //printf( "actualNumberOfWindows = %d\n", *actualNumberOfWindows); 

      for ( i = 0;  i < count; i++ ){
	wnfetd_c ( &result, i, &start, &stop );
	Sep_Times[j]=start;
	j++;
	Sep_Times[j]=stop;
	j++;
	//et2utc_c ( start, "ISOD", 3, 1024, MGSO_time );
	//(void)fprintf(stdout,"%s Seperation Event Being\n",MGSO_time);//
	//et2utc_c ( stop, "ISOD", 3, 1024, MGSO_time );
	//(void)fprintf(stdout,"%s Seperation Event End\n",MGSO_time);//
	*actualNumberOfWindows = j/2;
      }
    }

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}
void Vector_Cross_Product(double A[3], double B[3], double *C)
{
  vcrss_c(A,B,C);
}
double Vector_Dot_Product(double A[3], double B[3])
{
  return vdot_c(A,B);
}
double Compute_Angle(double A[3], double B[3])
{
  return vsep_c(A,B)*dpr_c();
}
double Vector_Magnitude(double A[3])
{
  return vnorm_c(A);
}
void Unit_Vector(double A[3], double *B)
{
  vhat_c(A,B);
}
void Vector_Subtract(double A[3], double B[3], double *C)
{
  vsub_c(B,A,C);
}
void Vector_Scale(double A[3], double B, double *C)
{
  vscl_c(B,A,C);
}
void qmult(double A[4], double B[4], double *COut)
{
  double AIn[4];
  double BIn[4];
  double C[4];
  AIn[0] = A[3];
  AIn[1] = -A[0];
  AIn[2] = -A[1];
  AIn[3] = -A[2];
  BIn[0] = B[3];
  BIn[1] = -B[0];
  BIn[2] = -B[1];
  BIn[3] = -B[2];
  qxq_c(AIn,BIn,C);
  COut[0] = -C[1];
  COut[1] = -C[2];
  COut[2] = -C[3];
  COut[3] = C[0];
}
void vecrot( double q1to2[4], double r1[3], double *r2)
{
  double D1;
  double D2;
  double D3;
	    
  D1 = q1to2[2]*r1[1]-q1to2[1]*r1[2];
  D2 = q1to2[0]*r1[2]-q1to2[2]*r1[0];
  D3 = q1to2[1]*r1[0]-q1to2[0]*r1[1];
  r2[0] = r1[0] + 2.0*(D1*q1to2[3] + q1to2[2]*D2 - q1to2[1]*D3);
  r2[1] = r1[1] + 2.0*(D2*q1to2[3] + q1to2[0]*D3 - q1to2[2]*D1);
  r2[2] = r1[2] + 2.0*(D3*q1to2[3] + q1to2[1]*D1 - q1to2[0]*D2);
}
void qposcosine( double qIn[4], double *qOut)
{
  if ( qIn[3] < 0.0 ){

    qOut[0] = -qIn[0];
    qOut[1] = -qIn[1];
    qOut[2] = -qIn[2];
    qOut[3] = -qIn[3];
  }
  else{
    qOut[0] = qIn[0];
    qOut[1] = qIn[1];
    qOut[2] = qIn[2];
    qOut[3] = qIn[3];
    
  }
  //(void)fprintf(stdout,"qposcosine: qIn = %f,%f,%f,%f\n",qIn[0],qIn[1],qIn[2],qIn[3]);
  //(void)fprintf(stdout,"qposcosine: qOut = %f,%f,%f,%f\n",qOut[0],qOut[1],qOut[2],qOut[3]);
}
#define EPSILON 1.0e-16
void qnorm( double qIn[4], double *qOut)
{
  double qn[4];
  double n;
  n = sqrt(qIn[0]*qIn[0]+qIn[1]*qIn[1]+qIn[2]*qIn[2]+qIn[3]*qIn[3]);
  if ( n < EPSILON ){
    qn[0] = 0.0;
    qn[1] = 0.0;
    qn[2] = 0.0;
    qn[3] = 1.0;
  }
  else{
    qn[0] = qIn[0]/n;
    qn[1] = qIn[1]/n;
    qn[2] = qIn[2]/n;
    qn[3] = qIn[3]/n;
  }
  // make sure the quaternion has a positive cosine
  //(void)fprintf(stdout,"qnorm: qIn = %f,%f,%f,%f\n",qIn[0],qIn[1],qIn[2],qIn[3]);
  qposcosine(qn,qOut);
  //(void)fprintf(stdout,"qnorm: qOut = %f,%f,%f,%f\n",qOut[0],qOut[1],qOut[2],qOut[3]);

}
void MatrixMultiply(double *m1, double *m2, int nrow1, int ncol1, int ncol2, double *mout)
{
  mxmg_c(m1, m2, nrow1, ncol1, ncol2, mout);

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}
void Cart2RADec( double r[3], double *RADec)
{
  double range;
  double ra;
  double dec;
  recrad_c(r,&range,&ra,&dec);
  RADec[0] = ra;
  RADec[1] = dec;

  if(failed_c()) {
    char errmsg[81];
    getmsg_c("SHORT", 81, errmsg);
    throw(eval_error(errmsg));
  }
}

long int FrameNameToID(const Cstring& name) {
    int ret;
    namfrm_c(*name, &ret);
    return (long int) ret;
}

bool FrameInfo(
	const Cstring&	frame_name,
	long int&	center_ID,
	long int&	frame_class,
	long int&	class_frame_ID) {
    long int	ID = FrameNameToID(frame_name);
    int		cent, frclss, clssid, found;
    frinfo_c(ID, &cent, &frclss, &clssid, &found);
    if(found) {
	center_ID = cent;
	frame_class = frclss;
	class_frame_ID = clssid;
	return true;
    };
    return false;
}

void StateTransform(
	const Cstring&	from_frame,
	const Cstring&	to_frame,
	double		timeval,
	double		transformation[6][6]) {
    sxform_c(*from_frame, *to_frame, timeval, transformation);
    if(failed_c()) {
	char errmsg[81];
	getmsg_c("SHORT", 81, errmsg);
	throw(eval_error(errmsg));
    }
}

void AngularVelocityFromXform(
	double	transformation[6][6],
	double	angular_velocity_vector[3]) {
    double	rotation[3][3];
    xf2rav_c(transformation, rotation, angular_velocity_vector);
    if(failed_c()) {
	char errmsg[81];
	getmsg_c("SHORT", 81, errmsg);
	throw(eval_error(errmsg));
    }
}

void FrameAngularVelocity(
	const Cstring&	from_frame,
	const Cstring&	to_frame,
	double		timeval,
	double		ang_vel_vec[3]) {
    double	transformation[6][6];
    sxform_c(*from_frame, *to_frame, timeval, transformation);
    if(failed_c()) {
	char errmsg[81];
	getmsg_c("SHORT", 81, errmsg);
	throw(eval_error(errmsg));
    }
    double	rotation[3][3];
    xf2rav_c(transformation, rotation, ang_vel_vec);
    if(failed_c()) {
	char errmsg[81];
	getmsg_c("SHORT", 81, errmsg);
	throw(eval_error(errmsg));
    }
}
