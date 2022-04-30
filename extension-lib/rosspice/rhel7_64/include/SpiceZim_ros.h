/*

-Header_File SpiceZim_ros.h ( Rosetta interface macros )

-Abstract

   Define interface macros to be called in place of Rosetta CSPICE
   user-interface-level functions.  These macros are generally used
   to compensate for compiler deficiencies.

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

   This header file defines interface macros to be called in place of
   Rosetta CSPICE user-interface-level functions.  Currently, the sole
   purpose of these macros is to implement automatic type casting under
   some environments that generate compile-time warnings without the
   casts. The typical case that causes a problem is a function argument
   list containing an input formal argument of type

      const double [3][3]

   Under some compilers, a non-const actual argument supplied in a call
   to such a function will generate a spurious warning due to the
   "mismatched" type.  These macros generate type casts that will
   make such compilers happy.

   Examples of compilers that generate warnings of this type are

      gcc version 2.2.2, hosted on NeXT workstations running
      NeXTStep 3.3

      Sun C compiler, version 4.2, running under Solaris.

-Author_and_Institution

   N.J. Bachman       (JPL)
   E.D. Wright        (JPL)

-Version

   -CSPICE Version 1.0.0, 09-DEC-2013 (NJB) (EDW)

       Added macros for

          gfalrl_ros
          gffvcv_ros
          gfiroi_ros
          gfpfov_ros
          gfrayc_ros
          gfsprl_ros
          rlalt_c
          rlasep_c

-Index_Entries

   interface macros for Rosetta CSPICE functions

*/


/*
Include Files:
*/


/*
Include the type definitions prior to defining the interface macros.
The macros reference the types.
*/
#ifndef  HAVE_SPICEDEFS_H
#include "SpiceZdf.h"
#endif


/*
Include those rename assignments for routines whose symbols will
collide with other libraries.
*/
#ifndef   HAVE_SPICERENAME_H
#include "SpiceZrnm.h"
#endif


#ifndef HAVE_ROSIFMACROS_H
#define HAVE_ROSIFMACROS_H


/*
Macros that substitute for function calls:
*/
     
   #define  gfalrl_ros( target, fixref,  abcorr, obsrvr,               \
                        dref,   dvec,    relate, refval, adjust,       \
                        step,   nintvls, cnfine, result          )     \
                                                                       \
        (   gfalrl_ros( (target),        (fixref),                     \
                      (abcorr),        (obsrvr), (dref),               \
                      CONST_VEC(dvec), (relate), (refval),             \
                      (adjust),        (step),   (nintvls),            \
                      (cnfine),        (result)             )   )


   #define  gffvcv_ros( inst,   covtyp, target,  fixref, abcorr,       \
                        obsrvr, nverts, featur,  relate, refval,       \
                        adjust, step,   nintvls, cnfine, result )      \
                                                                       \
        (   gffvcv_ros( (inst),   (covtyp), (target),  (fixref),       \
                        (abcorr), (obsrvr), (nverts),                  \
                        CONST_VEC3(featur), (relate),  (refval),       \
                        (adjust), (step),   (nintvls), (cnfine),       \
                        (result)                                 )   )


   #define  gfiroi_ros( inst, target, trgref, roi, nroi,  abcorr,     \
                      obsrvr, step, cnfine, result  )                 \
                                                                      \
        (    gfiroi_ros( (inst), (target), (trgref), CONST_VEC3(roi), \
                         (nroi), (abcorr),                            \
                         (obsrvr), (step), (cnfine), (result)  ) )


   #define   gfpfov_ros( inst,   trgpos, trgctr, trgref,               \
                       abcorr, obsrvr, step,  cnfine, result )         \
                                                                       \
        (    gfpfov_ros( (inst), CONST_VEC(trgpos), (trgctr),          \
                       (trgref), (abcorr), (obsrvr), (step),           \
                       (cnfine), (result)  ) )


   #define   gfrayc_ros( dref,   dvec,  frame,  abcorr, obsrvr,    \
                         crdsys, coord, relate, refval, adjust,    \
                         step,   nintvls, cnfine, result  )        \
                                                                   \
        (    gfrayc_ros( (dref),   CONST_VEC(dvec), (frame),       \
                         (abcorr), (obsrvr),        (crdsys),      \
                         (coord),  (relate),        (refval),      \
                         (adjust), (step),          (nintvls),     \
                         (cnfine), (result)                      ) )


   #define  gfsprl_ros( extrem, target,  fixref, abcorr, obsrvr,       \
                      dref,   dvec,    relate, refval, adjust,         \
                      step,   nintvls, cnfine, result          )       \
                                                                       \
        (   gfsprl_ros( (extrem),        (target), (fixref),           \
                      (abcorr),        (obsrvr), (dref),               \
                      CONST_VEC(dvec), (relate), (refval),             \
                      (adjust),        (step),   (nintvls),            \
                      (cnfine),        (result)             )   )


   #define  rlalt_c(  target, start,   fixref, abcorr,                 \
                      obsrvr, dref,    dvec,   extpt,                  \
                      alt,    trgepc,  srfvec, found  )                \
                                                                       \
        (   rlalt_c(  (target), (start),  (fixref),        (abcorr),   \
                      (obsrvr), (dref),   CONST_VEC(dvec), (extpt),    \
                      (alt),    (trgepc), (srfvec),        (found) )  )


   #define  rlasep_c( extrem,  target, start,   fixref,                \
                      abcorr,  obsrvr, dref,    dvec,                  \
                      extpt,   angle,  trgepc,  srfvec )               \
                                                                       \
        (   rlasep_c( (extrem), (target), (start),  (fixref),          \
                      (abcorr), (obsrvr), (dref),   CONST_VEC(dvec),   \
                      (extpt),  (angle),  (trgepc), (srfvec) )     )


#endif
