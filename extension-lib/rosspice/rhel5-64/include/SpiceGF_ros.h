/*

-Header_File SpiceGF_ros.h ( Rosetta GF-specific definitions )

-Abstract

   Perform Rosetta GF-specific definitions.

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

   GF

-Keywords

   GEOMETRY
   SEARCH

-Exceptions

   None

-Files

   None

-Particulars

   This header defines macros that may be referenced in application
   code that calls Rosetta specific CSPICE GF functions.


   Macros
   ======

      Workspace parameters
      --------------------

      CSPICE applications normally don't declare workspace arguments
      and therefore don't directly reference workspace size parameters.
      However, CSPICE GF APIs dealing with numeric constraints
      dynamically allocate workspace memory; the amount allocated
      depends on the number of intervals the workspace windows can
      hold. This amount is an input argument to the GF numeric quantity
      APIs.

      The parameters below are used to calculate the amount of memory
      required. Each workspace window contains 6 double precision
      numbers in its control area and 2 double precision numbers for
      each interval it can hold.


         Name                  Description
         ----                  ----------
         SPICE_GF_NWAAS        Number of workspce windows used by
                               gfias_ros, gfpias_ros and the underlying 
                               SPICELIB routines GFIAS, GFPIAS.

         SPICE_GF_NWALRL       Number of workspace windows used by
                               gfalrl_c and the underlying SPICELIB
                               routine GFALRL.

         SPICE_GF_NWFVCV       Number of workspace windows used by
                               gffvcv_c and the underlying SPICELIB
                               routine GFFVCV.

         SPICE_GF_NWLMB        Number of workspace windows used by
                               gflorn_c and the underlying SPICELIB
                               routine GFLORN.

         SPICE_GF_NWOD         Number of workspce windows used by
                               gfob_c, gfop_c and the underlying SPICELIB
                               routines GFORB, GFORP.

         SPICE_GF_NWSPED       Number of workspace windows used by
                               gfsd_c, gfpsd_c and the underlying SPICELIB
                               routines GFSD, GFPSD.

         SPICE_GF_NWSPRL       Number of workspace windows used by
                               gfsprl_c and the underlying SPICELIB
                               routine GFSPRL.


      Parameters specific to gffvcv_ros
      ---------------------------------

         SPICE_GFFVCV_NPOLY    Number of polygon sides used to approximate
                               a circular or elliptical FOV.

         SPICE_GFFVCV_MAXPC    Maximum number of vertices of a polygonal
                               chain representing a surface feature.


-Examples

   None

-Restrictions

   None.

-Literature_References

   None.

-Author_and_Institution

   N.J. Bachman    (JPL)
   E.D. Wright     (JPL)

-Version

   -Rosetta Version 1.1.0, 27-MAR-2014 (NJB) (EDW)

      Added parameters:

         SPICE_GF_NWSPRL

   -Rosetta Version 1.0.0, 09-DEC-2013 (NJB) (EDW)

      Added parameters:

         SPICE_GF_NWAAS
         SPICE_GF_NWALRL
         SPICE_GF_NWFVCV
         SPICE_GF_NWLMB
         SPICE_GF_NWOD
         SPICE_GF_NWSPED

*/


#ifndef HAVE_ROS_GF_H

   #define HAVE_ROS_GF_H


   /*
   See the Particulars section above for parameter descriptions.
   */

   /*
   Workspace parameters
   */
   #define SPICE_GF_NWAAS          6
   #define SPICE_GF_NWALRL         6
   #define SPICE_GF_NWFVCV         5
   #define SPICE_GF_NWLMB          (SPICE_GF_NWMAX + 1)
   #define SPICE_GF_NWOD           6
   #define SPICE_GF_NWSPED         6
   #define SPICE_GF_NWSPRL         5


   /*
   gffvcv_ros parameters
   */
   #define SPICE_GFFVCV_NPOLY      50
   #define SPICE_GFFVCV_MAXPC      100000


#endif


/*
   End of header file SpiceGF_ros.h
*/
