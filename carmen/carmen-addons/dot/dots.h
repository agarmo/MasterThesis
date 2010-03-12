#ifndef DOTS_H
#define DOTS_H

#include "shape.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct dot_filter {
  int id;
  double x;
  double y;
  double vx;
  double vy;
  double vxy;
  int lpiq[500];  // laser point index queue
  int num_lpiq;
  int laser_mask[500];
  carmen_shape_t shape;  // shape of object
  int *counts;
} carmen_dot_filter_t, *carmen_dot_filter_p;


extern carmen_dot_filter_p filters;
extern int num_filters;
extern int current_filter;
extern carmen_map_t static_map;
extern carmen_localize_map_t localize_map;
extern carmen_robot_laser_message laser_msg;
extern carmen_localize_globalpos_message odom;
extern int laser_mask[];
extern double laser_max_range;
extern double robot_width;
extern double contour_resolution;

extern inline int is_in_map(int x, int y) {

  return (x >= 0 && y >= 0 && x < static_map.config.x_size &&
	  y < static_map.config.y_size);
}



#ifdef __cplusplus
}
#endif

#endif
