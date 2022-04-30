
/*
   Estimate the fraction of the area of the visible cap of an
   ellipsoidal target that is not occulted by a second ellipsoidal
   target.


   Version 1.0.0 13-MAY-2014 (NJB) 
*/

#include <math.h>
#include <stdio.h>
#include "SpiceUsr.h"
#include "SpiceZst.h"
#include "SpiceZfc.h"


void vzfrac ( SpiceInt             frontID, 
	      ConstSpiceChar     * frontFrame,
	      ConstSpiceDouble     frontRadii [3],
	      SpiceInt             backID, 
	      ConstSpiceChar     * backFrame,
	      ConstSpiceDouble     backRadii  [3],
	      SpiceInt             obsID, 
	      ConstSpiceChar     * abcorr,
	      SpiceDouble          et,
	      SpiceDouble        * frac    )
{
   /*
   Prototypes 
   */
   int zzellpol_(doublereal *ellips, integer *fit,       integer *n,
                 doublereal *poly                                  );


   int zzfeatar_(doublereal *a,      doublereal *b,      doublereal *c__,
                 integer    *nverts, doublereal *featur, 
                 doublereal *ftaxis, doublereal *area);

   int zzfeatvz_(integer    *insfid, integer    *nbds,   doublereal *bounds, 
                 doublereal *axis,   doublereal *fovrad, doublereal *a,
                 doublereal *b,      doublereal *c__,    integer    *targid, 
                 doublereal *et,     integer    *fixfid, char       *abcorr,
                 integer    *obsid,  integer    *nverts, doublereal *featur,
                 doublereal *ftaxis, doublereal *area,   ftnlen      abcorr_len);

   /*
   Local constants 
   */

   /*
   J2000 frame ID 
   */
   #define J2CODE          1

   /*
   Number of vertices used for limb polygons 
   */
   #define NVERTS          100

   /*
   ZZELLPOL polygon fit "style" codes 
   */
   /*
      INTEGER               INSCRB
      PARAMETER           ( INSCRB = 1 )

      INTEGER               CRSCRB
      PARAMETER           ( CRSCRB = 2 )

      INTEGER               EQAREA
      PARAMETER           ( EQAREA = 3 )
   */
   #define INSCRB           1
   #define CRSCRB           2
   #define EQAREA           3


   /*
   Local variables 
   */
   SpiceBoolean            found;

   SpiceDouble             FOVAxis        [3];
   SpiceDouble             FOVRad;
   SpiceDouble             backLimbArray  [9];
   SpiceDouble             backLimbAxis   [3];
   SpiceDouble             backLimbPoly   [NVERTS][3];
   SpiceDouble             backLt;
   SpiceDouble             backPos        [3];
   SpiceDouble             backViewPt     [3];
   SpiceDouble             capArea;
   SpiceDouble             center         [3];
   SpiceDouble             frontET;
   SpiceDouble             frontFOVArray  [NVERTS][3];
   SpiceDouble             frontLimbArray [9];
   SpiceDouble             frontLimbPoly  [NVERTS][3];
   SpiceDouble             frontLt;
   SpiceDouble             frontPos       [3];
   SpiceDouble             frontViewPt    [3];
   SpiceDouble             occultedArea;
   SpiceDouble             xform          [3][3];

   SpiceEllipse            backLimb;
   SpiceEllipse            frontLimb;

   SpiceInt                backFrameID;
   SpiceInt                fitType;
   SpiceInt                frontFrameID;
   SpiceInt                i;
   SpiceInt                insFrameID;
   SpiceInt                nVerts;

   SpicePlane              cutPlane;


   chkin_c( "vzfrac"  );

   /*
   Get frame IDs of front and back frames. 
   */
   namfrm_c ( frontFrame, &frontFrameID );

   if ( frontFrameID == 0 )
   {
      setmsg_c ( "Could not convert front frame name # to ID." );
      errch_c  ( "#", frontFrame                               );
      sigerr_c ( "SPICE(NOTRANSLATION)"                        );
      chkout_c ( "vzfrac"                                      );
      return;
   }

   namfrm_c ( backFrame, &backFrameID );

   if ( backFrameID == 0 )
   {
      setmsg_c ( "Could not convert back frame name # to ID." );
      errch_c  ( "#", backFrame                               );
      sigerr_c ( "SPICE(NOTRANSLATION)"                       );
      chkout_c ( "vzfrac"                                     );
      return;
   }


   /*
   Get limb polygon of front (occulting) body. The polygon's vertices
   are expressed in the frame `frontFrame'.
   */
   spkezp_c ( frontID, et, frontFrame, abcorr, obsID, frontPos, &frontLt );

   vminus_c ( frontPos, frontViewPt );

   edlimb_c ( frontRadii[0], frontRadii[1], frontRadii[2], 
              frontViewPt,   &frontLimb                   );

   el2cgv_c ( &frontLimb, frontLimbArray, frontLimbArray+3, frontLimbArray+6 );

   fitType = INSCRB;
   nVerts  = NVERTS;

   zzellpol_ ( (doublereal *) frontLimbArray,
               (integer    *) &fitType,
               (integer    *) &nVerts,
               (doublereal *) frontLimbPoly );

   /*
   Generate the FOV boundary vectors corresponding to the front body's
   limb polygon. 
   */
   for ( i = 0;  i < nVerts;  i++ )
   {
      vsub_c ( frontLimbPoly[i], frontViewPt, frontFOVArray[i] );
   }

   /*
   The upcoming call to ZZFEATVZ will treat the instrument frame as though
   it's centered at the observer. This will give us the wrong evaluation
   epoch. To avoid problems, we'll make the instrument frame J2000. 
   */
   frontET = et - frontLt;

   pxform_c ( frontFrame, "J2000", frontET, xform );

   for ( i = 0;  i < nVerts;  i++ )
   {
      /*
      mxv_c supports overwriting the input vector. 
      */
      mxv_c ( xform, frontFOVArray[i], frontFOVArray[i] );
   }

   /*
   Transform the observer-target vector as well. This is the
   FOV axis vector. 
   */
   mxv_c ( xform, frontPos, FOVAxis );

   /*
   Compute the FOV angular radius upper bound.
   */
   FOVRad = 0.0;

   for ( i = 0;  i < nVerts;  i++ )
   {
      FOVRad = maxd_c (  2,  FOVRad,  vsep_c( frontFOVArray[i], FOVAxis )  );
   }


   /*
   Now compute the limb polygon for the back (occulted) body. First,
   find the limb.
   */
   spkezp_c ( backID, et, backFrame, abcorr, obsID, backPos, &backLt );

   vminus_c ( backPos, backViewPt );

   edlimb_c ( backRadii[0], backRadii[1], backRadii[2], 
              backViewPt,   &backLimb                   );

   /*
   In this case, we need to move the limb toward the observer by 
   a small amount, so the limb "feature" is on the near side of
   the limb. 
   */
   el2cgv_c ( &backLimb, backLimbArray, backLimbArray+3, backLimbArray+6 );

   /*
   We need a central axis for the limb "feature." The limb center vector
   will suffice.
   */
   vhat_c ( backLimbArray, backLimbAxis );

   vlcom_c ( 1.0, backLimbArray, 1.e-3, backLimbAxis, center );

   psv2pl_c ( center, backLimbArray+3, backLimbArray+6, &cutPlane );

   /*
   Intersect the shifted plane with the ellipsoid to find the
   feature ellipse. 
   */
   inedpl_c (  backRadii[0], backRadii[1], backRadii[2],
               &cutPlane,    &backLimb,    &found       );

   if ( !found )
   {
      setmsg_c ( "Intersection of cutPlane and "
                 "ellipsoid was not found."      );
      sigerr_c ( "SPICE(BUG)"                    );
      chkout_c ( "vzfrac"                        );
      return;

   }
   el2cgv_c ( &backLimb, backLimbArray, backLimbArray+3, backLimbArray+6 );


   fitType = INSCRB;
   nVerts  = NVERTS;

   zzellpol_ ( (doublereal *) backLimbArray,
               (integer    *) &fitType,
               (integer    *) &nVerts,
               (doublereal *) backLimbPoly );


   /*
   Compute the area of the source cap. 
   */
   zzfeatar_ ( (doublereal   *) backRadii, 
               (doublereal   *) backRadii+1,
               (doublereal   *) backRadii+2, 
               (integer      *) &nVerts,
               (doublereal   *) backLimbPoly,
               (doublereal   *) backLimbAxis,
               (doublereal   *) &capArea      );

   /*printf ( "cap area = %e\n", capArea );*/

   /*
   Compute the area of the portion of the cap within the FOV. 
   */

   insFrameID = J2CODE;

   zzfeatvz_ ( (integer       *) &insFrameID,
               (integer       *) &nVerts,
               (doublereal    *) frontFOVArray,
               (doublereal    *) FOVAxis,
               (doublereal    *) &FOVRad,
               (doublereal    *) backRadii,
               (doublereal    *) backRadii+1,
               (doublereal    *) backRadii+2,
               (integer       *) &backID,
               (doublereal    *) &et,
	       (integer       *) &backFrameID,
               (char          *) abcorr,
	       (integer       *) &obsID,
	       (integer       *) &nVerts,
               (doublereal    *) backLimbPoly,
               (doublereal    *) backLimbAxis,
               (doublereal    *) &occultedArea,
               (ftnlen         ) strlen(abcorr) );

   /*
   Make sure cap area is non-zero. 
   */
   if ( capArea == 0.0 )
   {
      setmsg_c ( "Error! capArea is zero" );
      sigerr_c ( "SPICE(BUG)"             );
      chkout_c ( "vzfra c"                );
      return;

   }

   /*printf ( "occulted area = %e\n", occultedArea );*/


   *frac  =  1.0 - ( occultedArea / capArea );

   chkout_c ( "vzfrac" );
   return;
}






