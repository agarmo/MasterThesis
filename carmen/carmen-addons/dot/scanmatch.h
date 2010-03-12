#ifndef SCANMATCH_H
#define SCANMATCH_H

#include <carmen/carmen.h>
#include "shape.h"

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Computes the best (dx,dy,dtheta) shift of new_contour's points to fit mean_contour
 * --shifts new_contour's points by (dx,dy,dtheta)
 * --if match_delta != NULL, puts (dx,dy,dtheta) in match_delta
 * --if odom_delta != NULL, uses odom_delta as a prior on (dx,dy,dtheta)
 * --returns 0 on success, -1 on error
 */
int scan_match(carmen_shape_p new_contour, int *laser_mask, carmen_shape_p mean_contour, double resolution,
	       double laser_stdev, carmen_point_p odom_delta, carmen_point_p match_delta);

extern double **egrid;
extern int egrid_x_size;
extern int egrid_y_size;
extern double egrid_x_offset;
extern double egrid_y_offset;
extern double egrid_resolution;
extern carmen_point_t scan_match_delta;
extern double scan_match_x_step_size;
extern double scan_match_y_step_size;
extern double scan_match_theta_step_size;
extern double scan_match_dx0;
extern double scan_match_dx1;
extern double scan_match_dy0;
extern double scan_match_dy1;
extern double scan_match_dt0;
extern double scan_match_dt1;


#ifdef __cplusplus
}
#endif

#endif
