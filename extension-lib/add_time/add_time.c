/*
*******************************************************************************
*
* Module:     add_time
*
* File Name:  add_time.c
*
* Purpose:    
*
* Modification History:
*
*   Date     Reason
* --------   ------------------------------------------------------------------
*
*******************************************************************************
*/
/* system includes */

#include	<stdlib.h>
#include	<stdio.h>
#include        <string.h>
#include	<math.h>
#include "SpiceUsr.h"

void seconds_to_duration( double seconds, char *duration)
{
  int days=0,hours=0,minutes=0;
  double secs=0.0,temp_seconds;
  //  temp_seconds = (double)((dint)(seconds*1000.0+.5))/1000.0;

  modf(seconds*1000.0+.5,&temp_seconds);
  temp_seconds = temp_seconds / 1000.0;
  days = (int)(temp_seconds/86400.0);
  temp_seconds = temp_seconds - (double)( days * 86400.0);
  hours = (int)(temp_seconds/3600.0);
  temp_seconds = temp_seconds - (double)( hours * 3600.0);
  minutes = (int)(temp_seconds/60.0);
  secs = temp_seconds - (double)( minutes * 60.0);

  //fprintf(stdout,"seconds = %f\n",seconds);
  //fprintf(stdout,"secs = %f\n",secs);
  if ( secs == 60.0 ){

    secs=0.0;
    minutes=minutes+1;
    if ( minutes == 60 ){
      minutes = 0;
      hours = hours + 1;
      if ( hours == 24 ){
	hours = 0;
	days = days + 1;
      }
    }
  }
  sprintf(duration,"%dT%2.2d:%2.2d:%06.3f",days,hours,minutes,secs);
  
}

int main( 

	int	argc,     /* i : # of command line parameters */
	char	**argv    /* i : array of command line parameter strings */ ) 
{
  char MGSO_Date[25];
  char offset[25];
  char copy_of_offset[100];
  char duration[25];
  double jd,jd1,delta,temp,et,et1,tdt,tdt1;
  char operation = '+';

  int oy,od,oh,om,deltai;
  double os;

  if ( argc != 3 ){
    fprintf( stderr,"Usage: add_time yyyy-dddT00:00:00.000 +/-dddT00:00:00\n");
    exit(1);
  }

  strcpy( MGSO_Date, argv[1]);
  strcpy( offset, argv[2]);

  if ( offset[0] == '+' ){

    operation='+';
    strcpy(copy_of_offset,&(offset[1]));
  }
  else if ( offset[0] == '-' ){

    operation='-';
    strcpy(copy_of_offset,&(offset[1]));

  }
  if ( copy_of_offset[4] == '-' ){

    oy = 1;
  }
  else{
    oy = 0;
    sscanf( copy_of_offset,"%03dT%02d:%02d:%lf",&od,&oh,&om,&os);
  }
  ldpool_c ( getenv("DEFAULT_LEAP_SECONDS_KERNEL") );
  str2et_c( MGSO_Date, &et );
  tdt=unitim_c( et, "ET","TDT");

  if ( operation == '+' ){
    tdt = tdt + (double)od*86400.0 +(double)oh*3600.0 + (double)om*60.0 + os;
    et = unitim_c( tdt,"TDT","ET");
    et2utc_c(et,"ISOD",3,22,MGSO_Date);

    fprintf( stdout, "%s\n",MGSO_Date );

  }
  else{

    if ( oy == 0 ){
      tdt = tdt - ((double)od*86400.0 +(double)oh*3600.0 + (double)om*60.0 + os);
      et = unitim_c( tdt,"TDT","ET");
      et2utc_c(et,"ISOD",3,22,MGSO_Date);

      fprintf( stdout, "%s\n",MGSO_Date );
    }
    else{
      str2et_c( copy_of_offset, &et1 );

      tdt1=unitim_c( et1, "ET","TDT");
 
      delta = (tdt - tdt1);
      
      //fprintf( stdout,"et = %f, et1 = %f\n",et,et1);
      //fprintf( stdout,"delta = %f\n",delta);

      seconds_to_duration (delta, duration);

      fprintf( stdout,"%s\n",duration);
    }
  }
  return 0; }
