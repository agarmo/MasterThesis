
/* Procrustean Shape Analysis */

#ifndef SHAPE_H
#define SHAPE_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
  int num_points;
  double *x;
  double *y;
} carmen_shape_t, *carmen_shape_p;


/*
 * Computes the mean procrustes shape from partial contours
 * --assumes partial contours (shapes) are already scaled and matched appropriately
 * --correspondences[i][j] is the ID of the jth point of the ith shape
 */
carmen_shape_p carmen_shape_compute_mean_shape(carmen_shape_t *shapes, int num_shapes, int **correspondences);


/*
 * Computes a variable number of principal components from partial contours
 * --assumes partial contours (shapes) are already scaled and matched appropriately
 * --correspondences[i][j] is the ID of the jth point of the ith shape
 * --mean_shape is optional
 * --fills in num_pcs principal components and eigenvalues (if eigenvalues != NULL)
 */
int carmen_shape_pca(carmen_shape_t *shapes, int num_shapes, int **correspondences, carmen_shape_t *mean_shape,
		     int num_pcs, carmen_shape_t **pcs, double **eigenvalues);



#ifdef __cplusplus
}
#endif

#endif
