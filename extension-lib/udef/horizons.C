#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#ifdef HAVE_VALUES_H
#include <values.h>
#endif

#include <iostream>

#include "horizons.h"

using namespace std;

double get_horizon_elevation( const char *dss, double azimuth)
{
  int          dss_number;
  int          dss_index;
  int          lower_azimuth_index;
  int          upper_azimuth_index;
  double       elevation;
  sscanf(dss,"%d",&dss_number);
  
  dss_index = 0;
  while ( dss_number != dss_ids[dss_index] && dss_index < NUMDSS){
    //(void)fprintf(stdout,"dss_number = %d, dss_index = %d, dss_ids[dss_index] = %d\n",dss_number,dss_index,dss_ids[dss_index]);
    dss_index = dss_index+1;
  }
  if ( dss_index >= NUMDSS ){
    cout << dss << " is not in station horizon mask list\n";
    return 10.0;
  }


  lower_azimuth_index = (int)azimuth;
  if ( lower_azimuth_index == NUMPOINTS-1 ){
    upper_azimuth_index = lower_azimuth_index;
  }
  else{
    upper_azimuth_index = lower_azimuth_index+1;
  }
  elevation = elevation_array[dss_index][lower_azimuth_index] + (elevation_array[dss_index][upper_azimuth_index] - elevation_array[dss_index][lower_azimuth_index])*( azimuth - (double)lower_azimuth_index);
  //    cout << lower_azimuth_index << " " << upper_azimuth_index << " " <<  elevation << "\n";
  return elevation;
}
