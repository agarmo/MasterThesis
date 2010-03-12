/*********************************************************
 *
 * This source code is part of the Carnegie Mellon Robot
 * Navigation Toolkit (CARMEN)
 *
 * CARMEN Copyright (c) 2002 Michael Montemerlo, Nicholas
 * Roy, Sebastian Thrun, Dirk Haehnel, Cyrill Stachniss,
 * and Jared Glover
 *
 * CARMEN is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public 
 * License as published by the Free Software Foundation; 
 * either version 2 of the License, or (at your option)
 * any later version.
 *
 * CARMEN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied 
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more 
 * details.
 *
 * You should have received a copy of the GNU General 
 * Public License along with CARMEN; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, 
 * Suite 330, Boston, MA  02111-1307 USA
 *
 ********************************************************/

#ifndef BD_UTILS_H
#define BD_UTILS_H

#include <carmen/carmen_graphics.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <time.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <termios.h>
#include <fcntl.h>

#include "geometry.h"


#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef struct {
  int           numvectors;
  VECTOR2     * vec;
} VECTOR2_SET;

typedef struct {
  int           numvectors;
  VECTOR3     * vec;
} VECTOR3_SET;

typedef struct {
  double        x;
  double        y;
  double        o;
} RPOS2;

typedef struct {
  double        forward;
  double        sideward;
  double        rotation;
} RMOVE2;

typedef struct {
  double        tv;
  double        rv;
} RVEL2;

typedef struct {
  double        x;
  double        y;
  double        z;
  double        rx;
  double        ry;
  double        rz;
} RPOS3;

typedef struct {
  VECTOR2             min;
  VECTOR2             max;
} BOUNDING_BOX2;

typedef struct {
  /*
    p[0] = xmin, ymin, zmin
    p[1] = xmin, ymax, zmin
    p[2] = xmax, ymax, zmin
    p[3] = xmax, ymin, zmin
    p[4] = xmin, ymin, zmax
    p[5] = xmin, ymax, zmax
    p[6] = xmax, ymax, zmax
    p[7] = xmax, ymin, zmax
  */
  VECTOR3             min;
  VECTOR3             max;
  VECTOR3             p[8];
} BOUNDING_BOX3;

typedef struct {
  VECTOR2             relpt;
  VECTOR2             abspt;
  int                 tag;
  int                 info;
} LASER_COORD2;

typedef struct {
  double              start;
  double              end;
  double              delta;
} LASER_ANGLE_RANGE;

typedef struct {
  RMOVE2              offset;
  LASER_ANGLE_RANGE   range;
} LASER_PROPERTIES2;

typedef struct {
  struct timeval      time;
  int                 numvalues;
  double              anglerange;
  double            * val;
  double            * angle;
  RMOVE2            * offset;
  int               * maxrange;
} LASER_DATA;

typedef struct {
  RPOS2               pos;
  double              vel;
  double              cov_sx;
  double              cov_sy;
  double              cov_sxy;
  VECTOR2             ll;       /* lower left */
  VECTOR2             ur;       /* upper right */
} P_STATE;

typedef struct {
  int                 numhprobs;
  double            * hprob;    /* probability that's the beam is a human */
  int                 numpstates;
  P_STATE           * pstate;
} P_TRACKING;

typedef struct {
  int                 numprobs;
  double            * prob;   
} DISTRIBUTION;

typedef struct {
  RPOS2               estpos;
  LASER_DATA          laser;
  LASER_COORD2      * coord;
  POLYGON             poly;
  BOUNDING_BOX2       bbox;
  P_TRACKING          ptracking;
  DISTRIBUTION        dynamic;
  int                 id;
} LASERSENS2_DATA;

typedef struct {
  struct timeval      time;
  RPOS2               rpos;
  RVEL2               rvel;
} POSSENS2_DATA;

enum ENTRY_TYPE   { POSITION,
		    CORR_POSITION,
		    LASER_VALUES,
		    LASER_VALUES3,
		    HUMAN_PROB };

typedef struct {
  enum ENTRY_TYPE          type;
  int                      index;
  int                      idx1, idx2;
} ENTRY_POSITION;

typedef struct {
  /* entries */
  int                 numentries;
  ENTRY_POSITION    * entry;  
  /* positions */
  int                 numpositions;
  POSSENS2_DATA     * psens;
  /* corrected positions */
  int                 numcpositions;
  POSSENS2_DATA     * cpsens;
  /* laser scans */
  int                 numlaserscans;
  LASERSENS2_DATA   * lsens;
} REC2_DATA;

typedef struct {
  double              pan;
  double              tilt;
} DECL_TYPE;

typedef struct {
  VECTOR3             relpt;
  VECTOR3             abspt;
  int                 tag;
  int                 info;
} LASER_COORD3;

typedef struct {
  RPOS3               estpos;
  VECTOR3             laseroffset;
  DECL_TYPE           declination;
  LASER_DATA          laser;
  int                 id;
  VECTOR3             origin;
  LASER_COORD3      * coord;
} LASERSENS3_DATA;

typedef struct {
  int                 numswscans;
  LASERSENS3_DATA   * swscan;
} LASERSWEEP3_DATA;

typedef struct {
  int                 numsweeps;
  LASERSWEEP3_DATA  * sweep;
  int                 numscans;
  LASERSENS2_DATA   * scan;
} LASERSCAN3_DATA;


typedef struct {
  struct timeval      time;
  RPOS3               rpos;
} POSSENS3_DATA;

typedef struct {
  /* entries */
  int                 numentries;
  ENTRY_POSITION    * entry;  
  /* positions */
  int                 numpositions;
  POSSENS3_DATA     * psens;
  /* corrected positions */
  int                 numcpositions;
  POSSENS3_DATA     * cpsens;
  /* laser scans */
  LASERSCAN3_DATA     lsens;
} REC3_DATA;

typedef struct {
  int                numfaces;
  int              * facept;
} POLYGON_REF;

typedef struct {
  int                numvectors;
  int              * vecref;
} VECTORREF_SET;

typedef struct {
  int                numpoints;
  int              * ptref;
} POINTREF_SET;

typedef struct {
  int                numpoints;
  int              * planept;
  PLANE_PARAMS       param;
} PLANE_POINTREF;

typedef struct {
  int                sweep;
  int                scan;
  int                beam;
} BEAMREF;

typedef struct {
  int                numpoints;
  BEAMREF          * planebeam;
  PLANE_PARAMS       param;
} PLANE_BEAMREF;

typedef struct {
  int                numpoints;
  int              * plane;
} POINT_PLANEREF;

typedef struct {
  int                numpoints;
  POINT3           * point;
  int                numpolygons;
  POLYGON_REF      * polygon;
  int                numplanes;
  PLANE_POINTREF   * plane;
} SPF_POINTS_DATA;

typedef struct {
  int                numplanes;
  PLANE_BEAMREF    * plane;
} PLANE_BEAM_DATA;

typedef struct {
  BEAMREF            ref0;
  BEAMREF            ref1;
  BEAMREF            ref2;
} TRIANGLE3_BEAMREF;

typedef struct {
  int                pt0;
  int                pt1;
  int                pt2;
} TRIANGLEREF;

typedef struct {
  int                 numtriangles;
  TRIANGLE3_BEAMREF * triangle;
} TRIANGLE3_BEAMREF_SET;

typedef struct {
  int                 numtriangles;
  TRIANGLEREF       * triangle;
} TRIANGLEREF_SET;

typedef struct {
  int                 numlines;
  LINE3             * line;
} LINE3_SET;

typedef struct {
  int          numpoints;
  double       xm;
  double       ym;
  double       phi;
  double       ndist;
  double       error;
  VECTOR2      pos;
} LINE2_LSQFT;

typedef struct {
  int idx0;
  int idx1;
} RANGE;

typedef struct {
  int          numranges;
  RANGE      * range;
} RANGE_SET;

typedef struct {
  int       len;
  double  * val;
} GAUSS_KERNEL;

typedef struct {
  double    y;
  double    u;
  double    v;
} YUV;

typedef struct {
  double    r;
  double    g;
  double    b;
} RGB;

typedef struct {
  double    h;
  double    s;
  double    v;
} HSV;

#define rad2deg rad2Deg
double        rad2Deg(double val);

#define deg2rad deg2Rad
double        deg2Rad(double val);

double        fsgn( double val );

double        random_gauss( void );

int           timeCompare ( struct timeval time1, struct timeval time2 );

double        timeDiff( struct timeval  t1, struct timeval t2);

void *        mdalloc(int ndim, int width, ...);

void          mdfree(void *tip, int ndim);

VECTOR3       rotateAndTranslateVector3( VECTOR3 p, VECTOR3 rot,
					 VECTOR3 trans );

VECTOR2       rotate_vector2( VECTOR2 p, double angle );

VECTOR2       rotate_and_translate_vector2( VECTOR2 p, double rot,
					    VECTOR2 trans );

double        vector3_length( VECTOR3 v1 );

double        vector2_distance( VECTOR2 p1, VECTOR2 p2 );

double        vector3_distance( VECTOR3 v1, VECTOR3 v2 );

VECTOR3       vector3_add( VECTOR3 v1, VECTOR3 v2 );

VECTOR3       vector3_diff( VECTOR3 v1, VECTOR3 v2 );

VECTOR3       vector3_cross( VECTOR3 v1, VECTOR3 v2 );

VECTOR3       vector3_scalar_mult( VECTOR3 v1, double m );

double        vector3_dot_mult( VECTOR3 v1, VECTOR3 v2 );


void          compute_alloc_rec3_coordpts( REC3_DATA *rec );

void          compute_rel_rec3_coordpts( REC3_DATA *rec );

void          compute_abs_rec3_coordpts( REC3_DATA *rec );

void          compute_estpos( REC3_DATA *rec );

void          mark_maxrange( REC3_DATA *rec, double MAX_RANGE );

void          compute_rec3_triangles( REC3_DATA rec,
				      TRIANGLE3_BEAMREF_SET *tri,
				      double minsize, double maxsize );

void          compute_vec2_convex_hull( VECTOR2_SET pts,
					VECTORREF_SET *hull );

void          compute_bounding_box( VECTOR3_SET vset, BOUNDING_BOX3 *box );

void          compute_bounding_box_with_min_max( VECTOR3 min, VECTOR3 max,
						 BOUNDING_BOX3 *box );

void          compute_bounding_box_plus_dist( VECTOR3_SET vset,
					      BOUNDING_BOX3 *box, double dist);

double        compute_triangle3_area( TRIANGLE3 tri );


double        distance_vec3_to_plane( PLANE_PARAMS pl, VECTOR3 p );

double        distance_vec3_to_line3( VECTOR3 p, LINE3 l );

double        distance_vec2_to_line2( VECTOR2 p, LINE2 l );

double        compute_factor_to_line( VECTOR3 p, LINE3 l );

VECTOR3       projection_vec3_to_line( VECTOR3 p, LINE3 l );

VECTOR2       projection_vec3_to_vec2( PLANE_PARAMS pl, VECTOR3 pt );

void          projection_vec3set_to_vec2set( PLANE_PARAMS pl, VECTOR3_SET pts,
					     VECTOR2_SET *pset );

VECTOR3       projection_vec3_to_plane( PLANE_PARAMS pl, VECTOR3 p );

void          projection_vec3set_to_plane( PLANE_PARAMS pl, VECTOR3_SET pts,
					    VECTOR3_SET *pset );

PLANE_PARAMS  compute_plane_from_three_vec3( VECTOR3 p1, VECTOR3 p2,
					     VECTOR3 p3 );

int           intersection_two_planes( PLANE_PARAMS pl1, PLANE_PARAMS pl2,
				       LINE3* L );

double        gauss_function( double x, double mu, double sigma );

GAUSS_KERNEL  compute_gauss_kernel( int length );

RMOVE2        compute_movement2_between_rpos2( RPOS2 s, RPOS2 e );

RPOS2         compute_rpos2_with_movement2( RPOS2 start, RMOVE2 move );

RPOS2         compute_rpos2_backwards_with_movement2( RPOS2 start,
						      RMOVE2 move );

double        convert_orientation_to_range( double angle );

double        compute_orientation_diff( double start, double end );

void          robot2map( RPOS2 pos, RPOS2 corr, RPOS2 *map );

void          map2robot( RPOS2 map, RPOS2 corr, RPOS2 *pos );

void          computeCorr( RPOS2 pos, RPOS2 map, RPOS2 *corr );

VECTOR3       compute_triangle_normal( TRIANGLE3 tri );

double        angle_between_two_vec3( VECTOR3 v1, VECTOR3 v2 );

LINE2_LSQFT   compute_lsqf_line( VECTOR2_SET p );


YUV           convert_from_rgb( RGB color );

RGB           convert_from_yuv( YUV color );

#endif /* ifdef BD_UTILS_H */

