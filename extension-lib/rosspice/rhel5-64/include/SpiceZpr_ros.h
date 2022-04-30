/*

-Header_File SpiceZpr.h ( Rosetta prototypes )

-Abstract

   Define prototypes for Rosetta user-interface-level functions.

-Disclaimer

   THIS SOFTWARE AND ANY RELATED MATERIALS WERE CREATED BY THE
   CALIFORNIA INSTITUTE OF TECHNOLOGY (CALTECH) UNDER A U.S.
   GOVERNMENT CONTRACT WITH THE NATIONAL AERONAUTICS AND SPACE
   ADMINISTRATION (NASA). THE SOFTWARE IS TECHNOLOGY AND SOFTWARE
   PUBLICLY AVAILABLE UNDER U.S. EXPORT LAWS AND IS PROVIDED "AS-IS"
   TO THE RECIPIENT WITHOUT WARRANTY OF ANY KIND, INCLUDING ANY
   WARRANTIES OF PERFORMANCE OR MERCHANTABILITY OR FITNESS FOR A
   PARTICULAR USE OR PURPOSE (AS SET FORTH IN UNITED STATES UCC
   SECTIONS 2312-2313) OR FOR ANY PURPOSE WHATSOEVER, FOR THE
   SOFTWARE AND RELATED MATERIALS, HOWEVER USED.

   IN NO EVENT SHALL CALTECH, ITS JET PROPULSION LABORATORY, OR NASA
   BE LIABLE FOR ANY DAMAGES AND/OR COSTS, INCLUDING, BUT NOT
   LIMITED TO, INCIDENTAL OR CONSEQUENTIAL DAMAGES OF ANY KIND,
   INCLUDING ECONOMIC DAMAGE OR INJURY TO PROPERTY AND LOST PROFITS,
   REGARDLESS OF WHETHER CALTECH, JPL, OR NASA BE ADVISED, HAVE
   REASON TO KNOW, OR, IN FACT, SHALL KNOW OF THE POSSIBILITY.

   RECIPIENT BEARS ALL RISK RELATING TO QUALITY AND PERFORMANCE OF
   THE SOFTWARE AND ANY RELATED MATERIALS, AND AGREES TO INDEMNIFY
   CALTECH AND NASA FOR ALL THIRD-PARTY CLAIMS RESULTING FROM THE
   ACTIONS OF RECIPIENT IN THE USE OF THE SOFTWARE.

-Required_Reading

   None.

-Literature_References

   None.

-Particulars

   This C header file contains prototypes for Rosetta CSPICE user-level
   C routines.  Prototypes for the underlying f2c'd SPICELIB routines
   are contained in the separate header file SpiceZfc.  However, those
   routines are not part of the official CSPICE API.

-Author_and_Institution

   N.J. Bachman       (JPL)
   E.D. Wright        (JPL)

-Version

   -CSPICE Version 1.0.0, 10-DEC-2013 (NJB) (EDW)

      Added prototypes for

         gfalrl_ros
         gffvcv_ros
         gfias_ros
         gfiroi_ros
         gflorn_ros
         gforb_ros
         gforp_ros
         gfpdist_ros
         gfpfov_ros
         gfpias_ros
         gfpsd_ros
         gfrayc_ros
         gfsd_ros
         gfsprb_ros
         gfsprl_ros
         gfsprp_ros
         gfsprr_ros
         rlalt_c
         rlasep_c

-Index_Entries

   prototypes of Rosetta CSPICE functions

*/


/*
Include Files:
*/


#ifndef  HAVE_SPICEDEFS_H
#include "SpiceZdf.h"
#endif

#ifndef  HAVE_SPICE_EK_H
#include "SpiceEK.h"
#endif

#ifndef  HAVE_SPICE_PLANES_H
#include "SpicePln.h"
#endif

#ifndef  HAVE_SPICE_ELLIPSES_H
#include "SpiceEll.h"
#endif

#ifndef  HAVE_SPICE_CELLS_H
#include "SpiceCel.h"
#endif

#ifndef  HAVE_SPICE_SPK_H
#include "SpiceSPK.h"
#endif

#ifndef HAVE_ROSWRAPPERS_H
#define HAVE_ROSWRAPPERS_H




/*
   Function prototypes for Rosetta CSPICE functions are listed below.
   Each prototype is accompanied by a function abstract and brief I/O
   description.

   See the headers of the C wrappers for detailed descriptions of the
   routines' interfaces.

   The list below should be maintained in alphabetical order.
*/


   void              gfalrl_ros ( ConstSpiceChar     * target,
                                ConstSpiceChar     * fixref,
                                ConstSpiceChar     * abcorr,
                                ConstSpiceChar     * obsrvr,
                                ConstSpiceChar     * dref,
                                ConstSpiceDouble     dvec[3],
                                ConstSpiceChar     * relate,
                                SpiceDouble          refval,
                                SpiceDouble          adjust,
                                SpiceDouble          step,
                                SpiceInt             nintvls,
                                SpiceCell          * cnfine,
                                SpiceCell          * result  );


   void           gffvcv_ros  ( ConstSpiceChar    * inst,
                                ConstSpiceChar    * covtyp,
                                ConstSpiceChar    * target,
                                ConstSpiceChar    * fixref,
                                ConstSpiceChar    * abcorr,
                                ConstSpiceChar    * obsrvr,
                                SpiceInt            nverts,
                                ConstSpiceDouble    featur [][3],
                                ConstSpiceChar    * relate,
                                SpiceDouble         refval,
                                SpiceDouble         adjust,
                                SpiceDouble         step,
                                SpiceInt            nintvls,
                                SpiceCell         * cnfine,
                                SpiceCell         * result  );


   void             gfias_ros ( ConstSpiceChar     * inst,
                                ConstSpiceChar     * target,
                                ConstSpiceChar     * abcorr,
                                ConstSpiceChar     * obsrvr,
                                ConstSpiceChar     * relate,
                                SpiceDouble          refval,
                                SpiceDouble          adjust,
                                SpiceDouble          step,
                                SpiceInt             nintvls,
                                SpiceCell          * cnfine,
                                SpiceCell          * result  );


   void              gfiroi_ros ( ConstSpiceChar     * inst,
                                ConstSpiceChar     * target,
                                ConstSpiceChar     * trgref,
                                ConstSpiceDouble     roi[][3],
                                ConstSpiceInt        nroi,
                                ConstSpiceChar     * abcorr,
                                ConstSpiceChar     * obsrvr,
                                SpiceDouble          step,
                                SpiceCell          * cnfine,
                                SpiceCell          * result  );


   void              gflorn_ros  ( ConstSpiceChar    * inst,
                                   ConstSpiceChar    * target,
                                   ConstSpiceChar    * fixref,
                                   ConstSpiceChar    * abcorr,
                                   ConstSpiceChar    * obsrvr,
                                   ConstSpiceChar    * relate,
                                   SpiceDouble         refval,
                                   SpiceDouble         adjust,
                                   SpiceDouble         step,
                                   SpiceInt            nintvls,
                                   SpiceCell         * cnfine,
                                   SpiceCell         * result  );


   void              gforb_ros  ( ConstSpiceDouble     dvec[3],
                                ConstSpiceChar     * dref,
                                ConstSpiceChar     * body,
                                ConstSpiceChar     * abcorr,
                                ConstSpiceChar     * obsrvr,
                                ConstSpiceChar     * relate,
                                SpiceDouble          refval,
                                SpiceDouble          adjust,
                                SpiceDouble          step,
                                SpiceInt             nintvls,
                                SpiceCell          * cnfine,
                                SpiceCell          * result  );



   void              gforp_ros ( ConstSpiceDouble     dvec[3],
                                ConstSpiceChar     * dref,
                                ConstSpiceDouble     trgpos[3],
                                ConstSpiceChar     * trgctr,
                                ConstSpiceChar     * trgfrm,
                                ConstSpiceChar     * abcorr,
                                ConstSpiceChar     * obsrvr,
                                ConstSpiceChar     * relate,
                                SpiceDouble          refval,
                                SpiceDouble          adjust,
                                SpiceDouble          step,
                                SpiceInt             nintvls,
                                SpiceCell          * cnfine,
                                SpiceCell          * result  );


   void           gfpdist_ros ( ConstSpiceDouble     trgpos[3],
                              ConstSpiceChar     * trgctr, 
                              ConstSpiceChar     * trgref,
                              ConstSpiceChar     * abcorr,
                              ConstSpiceChar     * obsrvr,
                              ConstSpiceChar     * relate,
                              SpiceDouble          refval,
                              SpiceDouble          adjust,
                              SpiceDouble          step,
                              SpiceInt             nintvls,
                              SpiceCell          * cnfine,
                              SpiceCell          * result  );


   void         gfpias_ros (  ConstSpiceChar     * inst,
                              ConstSpiceDouble     trgpos [3],
                              ConstSpiceChar     * trgctr,
                              ConstSpiceChar     * trgref,
                              ConstSpiceChar     * abcorr,
                              ConstSpiceChar     * obsrvr,
                              ConstSpiceChar     * relate,
                              SpiceDouble          refval,
                              SpiceDouble          adjust,
                              SpiceDouble          step,
                              SpiceInt             nintvls,
                              SpiceCell          * cnfine,
                              SpiceCell          * result     );


   void           gfpsd_ros ( ConstSpiceDouble     trgpos [3],
                              ConstSpiceChar     * trgctr,
                              ConstSpiceChar     * trgref,
                              ConstSpiceChar     * outref,
                              ConstSpiceChar     * abcorr,
                              ConstSpiceChar     * obsrvr,
                              ConstSpiceChar     * relate,
                              SpiceDouble          refval,
                              SpiceDouble          adjust,
                              SpiceDouble          step,
                              SpiceInt             nintvls,
                              SpiceCell          * cnfine,
                              SpiceCell          * result     );
                    

   void            gfrayc_ros ( ConstSpiceChar     * dref,
                                ConstSpiceDouble     dvec [3],
                                ConstSpiceChar     * frame,
                                ConstSpiceChar     * abcorr,
                                ConstSpiceChar     * obsrvr,
                                ConstSpiceChar     * crdsys,
                                ConstSpiceChar     * coord,
                                ConstSpiceChar     * relate,
                                SpiceDouble          refval,
                                SpiceDouble          adjust,
                                SpiceDouble          step,
                                SpiceInt             nintvls,
                                SpiceCell          * cnfine,
                                SpiceCell          * result  );


   void              gfsd_ros ( ConstSpiceChar     * target,
                                ConstSpiceChar     * outref,
                                ConstSpiceChar     * abcorr,
                                ConstSpiceChar     * obsrvr,
                                ConstSpiceChar     * relate,
                                SpiceDouble          refval,
                                SpiceDouble          adjust,
                                SpiceDouble          step,
                                SpiceInt             nintvls,
                                SpiceCell          * cnfine,
                                SpiceCell          * result  );


   void              gfpfov_ros ( ConstSpiceChar     * inst,
                                ConstSpiceDouble     trgpos[3],
                                ConstSpiceChar     * trgctr,
                                ConstSpiceChar     * trgref,
                                ConstSpiceChar     * abcorr,
                                ConstSpiceChar     * obsrvr,
                                SpiceDouble          step,
                                SpiceCell          * cnfine,
                                SpiceCell          * result  );


   void              gfsprb_ros ( ConstSpiceDouble     ray1[3],
                                ConstSpiceChar     * frame1,
                                ConstSpiceChar     * body,
                                ConstSpiceChar     * shape,
                                ConstSpiceChar     * obsrvr,
                                ConstSpiceChar     * abcorr,
                                ConstSpiceChar     * relate,
                                SpiceDouble          refval,
                                SpiceDouble          adjust,
                                SpiceDouble          step,
                                SpiceInt             nintvls,
                                SpiceCell          * cnfine,
                                SpiceCell          * result  );

   void             gfsprl_ros  ( ConstSpiceChar     * extrem,
                                ConstSpiceChar     * target,
                                ConstSpiceChar     * fixref,
                                ConstSpiceChar     * abcorr,
                                ConstSpiceChar     * obsrvr,
                                ConstSpiceChar     * dref,
                                ConstSpiceDouble     dvec[3],
                                ConstSpiceChar     * relate,
                                SpiceDouble          refval,
                                SpiceDouble          adjust,
                                SpiceDouble          step,
                                SpiceInt             nintvls,
                                SpiceCell          * cnfine,
                                SpiceCell          * result  );

   void             gfsprp_ros (  ConstSpiceDouble     ray1[3],
                                ConstSpiceChar     * frame1,
                                ConstSpiceDouble     trgpos[3],
                                ConstSpiceChar     * trgctr,
                                ConstSpiceChar     * trgfrm,
                                ConstSpiceChar     * obs,
                                ConstSpiceChar     * abcorr,
                                ConstSpiceChar     * relate,
                                SpiceDouble          refval,
                                SpiceDouble          adjust,
                                SpiceDouble          step,
                                SpiceInt             nintvls,
                                SpiceCell          * cnfine,
                              SpiceCell          * result  );

   void              gfsprr_ros ( ConstSpiceDouble     ray1[3],
                                ConstSpiceChar     * frame1,
                                ConstSpiceDouble     ray2[3],
                                ConstSpiceChar     * frame2,
                                ConstSpiceChar     * relate,
                                SpiceDouble          refval,
                                SpiceDouble          adjust,
                                SpiceDouble          step,
                                SpiceInt             nintvls,
                                SpiceCell          * cnfine,
                                SpiceCell          * result  );


   void              rlalt_c  ( ConstSpiceChar    * target,
                                SpiceDouble         et,
                                ConstSpiceChar    * fixref,
                                ConstSpiceChar    * abcorr,
                                ConstSpiceChar    * obsrvr,
                                ConstSpiceChar    * dref,
                                ConstSpiceDouble    dvec   [ 3 ],
                                SpiceDouble         extpt  [ 3 ],
                                SpiceDouble       * alt,
                                SpiceDouble       * trgepc,
                                SpiceDouble         srfvec [ 3 ],
                                SpiceBoolean      * found         );


   void              rlasep_c ( ConstSpiceChar    * extrem,
                                ConstSpiceChar    * target,
                                SpiceDouble         et,
                                ConstSpiceChar    * fixref,
                                ConstSpiceChar    * abcorr,
                                ConstSpiceChar    * obsrvr,
                                ConstSpiceChar    * dref,
                                ConstSpiceDouble    dvec   [ 3 ],
                                SpiceDouble         extpt  [ 3 ],
                                SpiceDouble       * angle,
                                SpiceDouble       * trgepc,
                                SpiceDouble         srfvec [ 3 ]  );

#endif
