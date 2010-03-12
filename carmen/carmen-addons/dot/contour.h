

#ifndef CONTOUR_H
#define CONTOUR_H

#include "shape.h"

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Computes the mean full contour from partial contours
 */
int update_full_contour(carmen_shape_p new_contour, int *laser_mask, carmen_shape_p mean_contour,
			int **counts_ptr, double resolution, double laser_stdev);

extern carmen_shape_t last_partial_contour;


#ifdef __cplusplus
}
#endif

#endif
