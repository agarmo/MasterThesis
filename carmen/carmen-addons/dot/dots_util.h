#ifndef DOTS_UTIL_H
#define DOTS_UTIL_H

#include <carmen/carmen.h>
#include <gsl/gsl_linalg.h>

#ifdef __cplusplus
extern "C" {
#endif


#define MI(M,i,j) ((i)*(M)->tda + (j))

#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

extern inline double dist(double dx, double dy) {

  return sqrt(dx*dx+dy*dy);
}

void rotate2d(double *x, double *y, double theta);

/*
 * inverts 2d matrix [[a, b],[c, d]]
 */
int invert2d(double *a, double *b, double *c, double *d);

double matrix_det(gsl_matrix *M);
void matrix_invert(gsl_matrix *M);
gsl_matrix *matrix_mult(gsl_matrix *A, gsl_matrix *B);

/*
 * computes the orientation of the bivariate normal with variances
 * vx, vy, and covariance vxy.  returns an angle in [-pi/2, pi/2].
 */
double bnorm_theta(double vx, double vy, double vxy);

/*
 * computes the major eigenvalue of the bivariate normal with variances
 * vx, vy, and covariance vxy.
 */
extern inline double bnorm_w1(double vx, double vy, double vxy) {

  return (vx + vy)/2.0 + sqrt(vxy*vxy + (vx-vy)*(vx-vy)/4.0);
}

/*
 * computes the minor eigenvalue of the bivariate normal with variances
 * vx, vy, and covariance vxy.
 */
extern inline double bnorm_w2(double vx, double vy, double vxy) {

  return (vx + vy)/2.0 - sqrt(vxy*vxy + (vx-vy)*(vx-vy)/4.0);
}

double bnorm_f(double x, double y, double ux, double uy,
	       double vx, double vy, double vxy);

double min_masked(double *data, int *mask, int n);
double argmin_imasked(double *data, int *mask, int i, int n);
double argmin_masked(double *data, int *mask, int n);
double argmin(double *data, int n);
double max_masked(double *data, int *mask, int n);
double argmax_imasked(double *data, int *mask, int i, int n);
double argmax_masked(double *data, int *mask, int n);
double argmax(double *data, int n);
double mean_imasked(double *data, int *mask, int i, int n);
double mean_masked(double *data, int *mask, int n);
double mean(double *data, int n);
double var_imasked(double *data, double mean, int *mask, int i, int n);
double cov_imasked(double *xdata, double ux, double *ydata, double uy, int *mask, int i, int n);

extern inline double *new_vector(int n) {

  double *v = (double *) calloc(n, sizeof(double));
  carmen_test_alloc(v);

  return v;
}

extern inline int *new_ivector(int n) {

  int *v = (int *) calloc(n, sizeof(int));
  carmen_test_alloc(v);

  return v;
}

double **new_matrix(int num_rows, int num_cols);
int **new_imatrix(int num_rows, int num_cols);
double **matrix_resize(double **m, int xpos, int ypos, int old_x_size, int old_y_size, int new_x_size, int new_y_size, double pad);
int **imatrix_resize(int **m, int xpos, int ypos, int old_x_size, int old_y_size, int new_x_size, int new_y_size, int pad);


// gift wrapping algorithm for finding convex hull: returns size of hull
int compute_convex_hull(double *xpoints, double *ypoints, int num_points, double *xhull, double *yhull);

int ray_intersect_arg(double rx, double ry, double rtheta,
		      double x1, double y1, double x2, double y2);

int ray_intersect_circle(double rx, double ry, double rtheta,
			 double cx, double cy, double cr,
			 double *px1, double *py1, double *px2, double *py2);

int ray_intersect_convex_polygon(double rx, double ry, double rtheta,
				 double *xpoly, double *ypoly, int np,
				 double *px1, double *py1, double *px2, double *py2);


#ifdef __cplusplus
}
#endif

#endif

