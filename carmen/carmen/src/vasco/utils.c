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

#include "bd-utils.h"

double
fsgn( double val )
{
  if (val>0.0) {
    return(1);
  } else if (val<0.0) {
    return(-1);
  } else {
    return(0);
  }
}

double
rad2Deg(double x)
{
  return x * 57.29578;
}

double
deg2Rad(double x)
{
  return x * 0.017453293;
}


int
timeCompare ( struct timeval time1, struct timeval time2 )
{
  if (time1.tv_sec<time2.tv_sec) {
    return(-1);
  } else if (time1.tv_sec>time2.tv_sec) {
    return(1);
  } else if (time1.tv_usec<time2.tv_usec) {
    return(-1);
  } else if (time1.tv_usec>time2.tv_usec) {
    return(1);
  } else {
    return(0);
  }
}

double
timeDiff( struct timeval  t1, struct timeval t2)
{
   double diff;
   diff =  (double) (t2.tv_usec - t1.tv_usec) / 1000000.0;
   diff += (double) (t2.tv_sec  - t1.tv_sec);
   return(diff);
}

VECTOR3
rotateAndTranslateVector3( VECTOR3 p, VECTOR3 rot, VECTOR3 trans )
{
  VECTOR3 p2;
  p2.x = trans.x +
    p.x * (cos(rot.y)*cos(rot.z)) +
    p.y * (sin(rot.x)*sin(rot.y)*cos(rot.z)-cos(rot.x)*sin(rot.z)) +
    p.z * (cos(rot.x)*sin(rot.y)*cos(rot.z)+sin(rot.x)*sin(rot.z));
  p2.y = trans.y +
    p.x * (cos(rot.y)*sin(rot.z)) +
    p.y * (sin(rot.x)*sin(rot.y)*sin(rot.z)+cos(rot.x)*cos(rot.z)) +
    p.z * (cos(rot.x)*sin(rot.y)*sin(rot.z)-sin(rot.x)*cos(rot.z));
  p2.z = trans.z +
    p.x * (-sin(rot.y)) +
    p.y * (sin(rot.x)*cos(rot.y)) +
    p.z * (cos(rot.x)*cos(rot.y));
  return(p2);
}

VECTOR2
rotate_vector2( VECTOR2 p, double rot )
{
  VECTOR2 v;
  v.x = p.x*cos(rot)+p.y*sin(rot);
  v.y = p.x*sin(rot)-p.y*cos(rot);
  return(v);
}

VECTOR2
rotate_and_translate_vector2( VECTOR2 p, double rot, VECTOR2 trans )
{
  VECTOR2 v;
  v.x = p.x*cos(rot)+p.y*sin(rot)+trans.x;
  v.y = p.x*sin(rot)-p.y*cos(rot)+trans.y;
  return(v);
}

double
vector2_distance( VECTOR2 p1, VECTOR2 p2 ) {
  return sqrt( (p1.x-p2.x)*(p1.x-p2.x) +
	       (p1.y-p2.y)*(p1.y-p2.y) );
}
  
double
vector3_distance( VECTOR3 v1, VECTOR3 v2 )
{
  return( sqrt( (v1.x-v2.x)*(v1.x-v2.x) +
		(v1.y-v2.y)*(v1.y-v2.y) +
		(v1.z-v2.z)*(v1.z-v2.z) ) );
}


double
vector3_length( VECTOR3 v1 )
{
  return( sqrt( (v1.x*v1.x) + (v1.y*v1.y) + (v1.z*v1.z) ) );
}


VECTOR3
vector3_add( VECTOR3 v1, VECTOR3 v2 )
{
  VECTOR3 v;
  v.x = v1.x + v2.x;
  v.y = v1.y + v2.y;
  v.z = v1.z + v2.z;
  return(v);
}

VECTOR3
vector3_diff( VECTOR3 v1, VECTOR3 v2 )
{
  VECTOR3 v;
  v.x = v1.x - v2.x;
  v.y = v1.y - v2.y;
  v.z = v1.z - v2.z;
  return(v);
}

VECTOR3
vector3_cross( VECTOR3 v1, VECTOR3 v2 )
{
  VECTOR3 v;
  v.x = ( v1.y * v2.z ) - ( v1.z * v2.y );
  v.y = ( v1.z * v2.x ) - ( v1.x * v2.z );
  v.z = ( v1.x * v2.y ) - ( v1.y * v2.x );
  return(v);
}

VECTOR3
vector3_scalar_mult( VECTOR3 v1, double m )
{
  VECTOR3 v;
  v.x = v1.x * m;
  v.y = v1.y * m;
  v.z = v1.z * m;
  return(v);
}

double
vector3_dot_mult( VECTOR3 v1, VECTOR3 v2 )
{
  double val;
  val = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
  return(val);
}


double
random_gauss( void )
{
  static int iset = 0;
  static double gset;
  float fac, rsq, v1, v2;
  if(iset == 0) {
    do {
      v1 = 2.0 * rand()/(RAND_MAX+1.0) - 1.0;
      v2 = 2.0 * rand()/(RAND_MAX+1.0) - 1.0;         
      rsq = v1*v1 + v2*v2;
    } while(rsq >= 1.0 || rsq == 0.0);
    fac = sqrt(-2.0*log(rsq)/rsq);
    gset = v1*fac;
    iset = 1;
    return v2*fac;
  }
  else {
    iset = 0;
    return gset;
  }
}

void
compute_bounding_box( VECTOR3_SET vset, BOUNDING_BOX3 *box )
{
  int i;
  VECTOR3 min, max;

  min.x = MAXDOUBLE;
  min.y = MAXDOUBLE;
  min.z = MAXDOUBLE;
  max.x = -MAXDOUBLE;
  max.y = -MAXDOUBLE;
  max.z = -MAXDOUBLE;
  
  for (i=0;i<vset.numvectors;i++) {
    if (vset.vec[i].x<min.x)
      min.x = vset.vec[i].x;
    if (vset.vec[i].y<min.y)
      min.y = vset.vec[i].y;
    if (vset.vec[i].z<min.z)
      min.z = vset.vec[i].z;
    if (vset.vec[i].x>max.x)
      max.x = vset.vec[i].x;
    if (vset.vec[i].y>max.y)
      max.y = vset.vec[i].y;
    if (vset.vec[i].z>max.z)
      max.z = vset.vec[i].z;
  }

  box->min.x = min.x;       box->max.x = max.x;
  box->min.y = min.y;       box->max.y = max.y;
  box->min.z = min.z;       box->max.z = max.z;

  box->p[0].x = min.x;      box->p[1].x = min.x;
  box->p[0].y = min.y;      box->p[1].y = max.y;
  box->p[0].z = min.z;      box->p[1].z = min.z;

  box->p[2].x = max.x;      box->p[3].x = max.x;
  box->p[2].y = max.y;      box->p[3].y = min.y;
  box->p[2].z = min.z;      box->p[3].z = min.z;

  box->p[4].x = min.x;      box->p[5].x = min.x;
  box->p[4].y = min.y;      box->p[5].y = max.y;
  box->p[4].z = max.z;      box->p[5].z = max.z;

  box->p[6].x = max.x;      box->p[7].x = max.x;
  box->p[6].y = max.y;      box->p[7].y = min.y;
  box->p[6].z = max.z;      box->p[7].z = max.z;

}

void
compute_bounding_box_with_min_max( VECTOR3 min, VECTOR3 max,
				   BOUNDING_BOX3 *box )
{
  box->p[0].x = min.x;      box->p[1].x = min.x;
  box->p[0].y = min.y;      box->p[1].y = max.y;
  box->p[0].z = min.z;      box->p[1].z = min.z;

  box->p[2].x = max.x;      box->p[3].x = max.x;
  box->p[2].y = max.y;      box->p[3].y = min.y;
  box->p[2].z = min.z;      box->p[3].z = min.z;

  box->p[4].x = min.x;      box->p[5].x = min.x;
  box->p[4].y = min.y;      box->p[5].y = max.y;
  box->p[4].z = max.z;      box->p[5].z = max.z;

  box->p[6].x = max.x;      box->p[7].x = max.x;
  box->p[6].y = max.y;      box->p[7].y = min.y;
  box->p[6].z = max.z;      box->p[7].z = max.z;

}

void
compute_bounding_box_plus_dist( VECTOR3_SET vset,
				BOUNDING_BOX3 *box,
				double dist)
{
  compute_bounding_box( vset, box );

  box->p[0].x -= dist;      box->p[1].x -= dist;
  box->p[0].y -= dist;      box->p[1].y += dist;
  box->p[0].z -= dist;      box->p[1].z -= dist;

  box->p[2].x += dist;      box->p[3].x += dist;
  box->p[2].y += dist;      box->p[3].y -= dist;
  box->p[2].z -= dist;      box->p[3].z -= dist;

  box->p[4].x -= dist;      box->p[5].x -= dist;
  box->p[4].y -= dist;      box->p[5].y += dist;
  box->p[4].z += dist;      box->p[5].z += dist;

  box->p[6].x += dist;      box->p[7].x += dist;
  box->p[6].y += dist;      box->p[7].y -= dist;
  box->p[6].z += dist;      box->p[7].z += dist;
  
}

double
compute_triangle3_area( TRIANGLE3 tri )
{
  double a = vector3_distance( tri.point0, tri.point1 );
  double b = vector3_distance( tri.point1, tri.point2 );
  double c = vector3_distance( tri.point2, tri.point0 );
  double s = (a+b+c/2.0);
  return( sqrt( s*(s-a)*(s-b)*(s-c) ) );
}

LINE2_LSQFT
compute_lsqf_line( VECTOR2_SET p )
{
  int i;
  LINE2_LSQFT line;
  double oxm, oym, Sx, Sy, Sxy, cphi, sphi;
  
  line.numpoints  = p.numvectors;
  line.xm         = 0;
  line.ym         = 0;
  line.error      = 0;
 
  if (line.numpoints<2)
    return(line);
  
  for (i=0; i<p.numvectors; i++) {
    line.xm += p.vec[i].x; 
    line.ym += p.vec[i].y; 
  }
  
  oxm = line.xm / (double) p.numvectors;
  oym = line.ym / (double) p.numvectors;

  Sx = 0; Sy = 0; Sxy = 0;
  for (i=0; i<p.numvectors; i++) {
    Sx  += (p.vec[i].x-oxm) * (p.vec[i].x-oxm);
    Sy  += (p.vec[i].y-oym) * (p.vec[i].y-oym);
    Sxy += (p.vec[i].x-oxm) * (p.vec[i].y-oym);
  }
  line.phi = 0.5 * atan2( (-2*Sxy), (Sy-Sx) );
  cphi = cos(line.phi);
  sphi = sin(line.phi);
  line.ndist = oxm * cphi + oym * sphi;
  for (i=0; i<p.numvectors; i++) {
    line.error += 
      ( ( p.vec[i].x * cphi + p.vec[i].y * sphi - line.ndist ) *
	( p.vec[i].x * cphi + p.vec[i].y * sphi - line.ndist ) );
  }
  return(line);
}  

double
gauss_function( double x, double mu, double sigma )
{
 return( (1/sqrt(2.0*M_PI*sigma*sigma)) *
	 exp(-(((x-mu)*(x-mu))/(2*sigma*sigma))) );  
}

GAUSS_KERNEL
compute_gauss_kernel_with_function( int length )
{
  GAUSS_KERNEL kernel;
  
  int center = (length-1) / 2;
  double sigma = sqrt( length/(2*M_PI) );
  int i;

  kernel.len = length;
  kernel.val = (double *) malloc( length * sizeof(double) );
  carmen_test_alloc(kernel.val);
  for (i=0;i<length;i++) {
    kernel.val[i] = gauss_function( (i-center), 0.0, sigma );
  }
  return(kernel);
}

GAUSS_KERNEL
compute_gauss_kernel(int length )
{
  GAUSS_KERNEL  kernel;
  int i, j, *store, sum = 0;
  store = (int *) malloc( length*sizeof(int) );
  carmen_test_alloc(store);
  store[0] = 1;
  for ( i=0; i<length-1; i++ ) {
    store[i+1] = 1;
    for ( j=i; j>0; j-- ) {
      store[j] = store[j] + store[j-1];
    }
  }
  for ( i=0; i<length; i++ ) {
    sum += store[i];
  }
  kernel.len = length;
  kernel.val = (double *) malloc( length * sizeof(double) );
  carmen_test_alloc(kernel.val);
  for (i=0;i<length;i++) {
    kernel.val[i] = store[i] / (double) sum;
  }
  free( store );
  return(kernel);
}


double
convert_orientation_to_range( double angle ) {
  if (angle<-M_PI) {
    return(2*M_PI -  fmod( fabs(angle),2*M_PI ));
  } else if (angle>M_PI) {
    return(-2*M_PI + fmod( angle,2*M_PI ));
  } else {
    return(angle);
  }
}

double
compute_orientation_diff( double start, double end ) {
  double diff;
  diff =
    convert_orientation_to_range( start ) -
    convert_orientation_to_range( end );
  if (diff<-M_PI) {
    return(2*M_PI+diff);
  } else if (diff>M_PI) {
    return(-2*M_PI+diff);
  } else {
    return(diff);
  }
}

RMOVE2
compute_movement2_between_rpos2( RPOS2 start, RPOS2 end )
{
  RMOVE2 move;

  /* compute forward and sideward sensing_MOVEMENT */
  move.forward =
    + (end.y - start.y) * sin(start.o)
    + (end.x - start.x) * cos(start.o);
  
  move.sideward =
    - (end.y - start.y) * cos(start.o)
    + (end.x - start.x) * sin(start.o);
  
  move.rotation = compute_orientation_diff( end.o, start.o );

  return( move );
}

RPOS2
compute_rpos2_with_movement2( RPOS2 start, RMOVE2 move ) {
  RPOS2 end;
  if ( (move.forward==0.0) && (move.sideward==0.0) && (move.rotation==0.0) )
    return (start);
  end.x = start.x + cos(start.o) * move.forward + sin(start.o) * move.sideward;
  end.y = start.y + sin(start.o) * move.forward - cos(start.o) * move.sideward;
  end.o = convert_orientation_to_range( start.o + move.rotation );
  return(end);
}

RPOS2
compute_rpos2_backwards_with_movement2( RPOS2 start, RMOVE2 move ) {
  RPOS2 end;
  if ( (move.forward==0.0) && (move.sideward==0.0) && (move.rotation==0.0) )
    return (start);
  end.o = convert_orientation_to_range( start.o - move.rotation );
  end.x = start.x - cos(end.o) * move.forward - sin(end.o) * move.sideward;
  end.y = start.y - sin(end.o) * move.forward + cos(end.o) * move.sideward;
  return(end);
}

int
inside_polygon( POLYGON polygon, POINT2 p )
{
  int counter = 0;
  int i;
  double xinters;
  POINT2 p1,p2;

  p1 = polygon.pt[0];
  for (i=1;i<=polygon.numpoints;i++) {
    p2 = polygon.pt[i % polygon.numpoints];
    if (p.y > MIN(p1.y,p2.y)) {
      if (p.y <= MAX(p1.y,p2.y)) {
        if (p.x <= MAX(p1.x,p2.x)) {
          if (p1.y != p2.y) {
            xinters = (p.y-p1.y)*(p2.x-p1.x)/(p2.y-p1.y)+p1.x;
            if (p1.x == p2.x || p.x <= xinters)
              counter++;
          }
        }
      }
    }
    p1 = p2;
  }

  if (counter % 2 == 0)
    return( FALSE);
  else
    return( TRUE );
}

double
rpos2_dist( RPOS2 pos1, RPOS2 pos2 )
{
  return(
	 sqrt( (pos1.x - pos2.x) * (pos1.x - pos2.x) +
	       (pos1.y - pos2.y) * (pos1.y - pos2.y) )
	 );
}

