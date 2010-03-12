#include "dots_util.h"


inline double dist(double dx, double dy) {

  return sqrt(dx*dx+dy*dy);
}

void rotate2d(double *x, double *y, double theta) {

  double x2, y2;

  x2 = *x;
  y2 = *y; 

  *x = x2*cos(theta) - y2*sin(theta);
  *y = x2*sin(theta) + y2*cos(theta);
}

/*
 * inverts 2d matrix [[a, b],[c, d]]
 */
int invert2d(double *a, double *b, double *c, double *d) {

  double det;
  double a2, b2, c2, d2;

  det = (*a)*(*d) - (*b)*(*c);
  
  if (fabs(det) <= 0.00000001)
    return -1;

  a2 = (*d)/det;
  b2 = -(*b)/det;
  c2 = -(*c)/det;
  d2 = (*a)/det;

  *a = a2;
  *b = b2;
  *c = c2;
  *d = d2;

  return 0;
}

double matrix_det(gsl_matrix *M) {

  gsl_matrix *A;
  gsl_permutation *p;
  int signum;
  double d;

  A = gsl_matrix_alloc(M->size1, M->size2);
  carmen_test_alloc(A);
  gsl_matrix_memcpy(A, M);
  p = gsl_permutation_alloc(A->size1);
  carmen_test_alloc(p);
  gsl_linalg_LU_decomp(A, p, &signum);
  d = gsl_linalg_LU_det(A, signum);
  gsl_matrix_free(A);
  gsl_permutation_free(p);

  return d;
}

void matrix_invert(gsl_matrix *M) {

  gsl_matrix *A;
  gsl_permutation *p;
  int signum;

  A = gsl_matrix_alloc(M->size1, M->size2);
  carmen_test_alloc(A);
  p = gsl_permutation_alloc(A->size1);
  carmen_test_alloc(p);
  gsl_linalg_LU_decomp(M, p, &signum);
  gsl_linalg_LU_invert(M, p, A);
  gsl_matrix_memcpy(M, A);
  gsl_matrix_free(A);
  gsl_permutation_free(p);
}

gsl_matrix *matrix_mult(gsl_matrix *A, gsl_matrix *B) {

  gsl_matrix *C;
  unsigned int i, j, k;

  if (A->size2 != B->size1)
    carmen_die("Can't multiply %dx%d matrix A with %dx%d matrix B",
	       A->size1, A->size2, B->size1, B->size2);

  C = gsl_matrix_calloc(A->size1, B->size2);
  carmen_test_alloc(C);

  for (i = 0; i < A->size1; i++)
    for (j = 0; j < B->size2; j++)
      for (k = 0; k < A->size2; k++)
	C->data[MI(C,i,j)] += A->data[MI(A,i,k)]*B->data[MI(B,k,j)];
  
  return C;
}

/*
 * computes the orientation of the bivariate normal with variances
 * vx, vy, and covariance vxy.  returns an angle in [-pi/2, pi/2].
 */
double bnorm_theta(double vx, double vy, double vxy) {

  double theta;
  double e;  // major eigenvalue
  double ex, ey;  // major eigenvector

  e = (vx + vy)/2.0 + sqrt(vxy*vxy + (vx-vy)*(vx-vy)/4.0);
  ex = vxy;
  ey = e - vx;

  theta = atan2(ey, ex);

  return theta;
}

/*
 * computes the major eigenvalue of the bivariate normal with variances
 * vx, vy, and covariance vxy.
 */
inline double bnorm_w1(double vx, double vy, double vxy) {

  return (vx + vy)/2.0 + sqrt(vxy*vxy + (vx-vy)*(vx-vy)/4.0);
}

/*
 * computes the minor eigenvalue of the bivariate normal with variances
 * vx, vy, and covariance vxy.
 */
inline double bnorm_w2(double vx, double vy, double vxy) {

  return (vx + vy)/2.0 - sqrt(vxy*vxy + (vx-vy)*(vx-vy)/4.0);
}

double bnorm_f(double x, double y, double ux, double uy,
		      double vx, double vy, double vxy) {
  
  double z, p;
  
  p = carmen_clamp(-0.999, vxy/sqrt(vx*vy), 0.999);  //dbug
  z = (x - ux)*(x - ux)/vx - 2.0*p*(x - ux)*(y - uy)/sqrt(vx*vy) + (y - uy)*(y - uy)/vy;

  return exp(-z/(2.0*(1 - p*p)))/(2.0*M_PI*sqrt(vx*vy*(1 - p*p)));
}

double min_masked(double *data, int *mask, int n) {

  int i;
  double x;

  i = 0;
  if (mask) {
    for (; i < n; i++)
      if (mask[i])
	break;
    if (i == n)
      return n/0.0;
  }
  x = data[i++];
  for (; i < n; i++)
    if ((!mask || mask[i]) && data[i] < x)
      x = data[i];

  return x;
}

double argmin_imasked(double *data, int *mask, int i, int n) {

  int j, jmin;
  double x;

  j = 0;
  if (mask) {
    for (; j < n; j++)
      if (mask[j] == i)
	break;
    if (j == n)
      return n/0.0;
  }
  jmin = j;
  x = data[j++];
  for (; j < n; j++) {
    if ((!mask || mask[j] == i) && data[j] < x) {
      jmin = j;
      x = data[j];
    }
  }

  return jmin;
}

double argmin_masked(double *data, int *mask, int n) {

  int j, jmin;
  double x;

  j = 0;
  if (mask) {
    for (; j < n; j++)
      if (mask[j])
	break;
    if (j == n)
      return n/0.0;
  }
  jmin = j;
  x = data[j++];
  for (; j < n; j++) {
    if ((!mask || mask[j]) && data[j] < x) {
      jmin = j;
      x = data[j];
    }
  }

  return jmin;
}

double argmin(double *data, int n) {

  int i, imin;
  double x;

  x = data[0];
  imin = 0;
  for (i = 1; i < n; i++) {
    if (data[i] < x) {
      x = data[i];
      imin = i;
    }
  }

  return imin;
}

double max_masked(double *data, int *mask, int n) {

  int i;
  double x;

  i = 0;
  if (mask) {
    for (; i < n; i++)
      if (mask[i])
	break;
    if (i == n)
      return n/0.0;
  }
  x = data[i++];
  for (; i < n; i++)
    if ((!mask || mask[i]) && data[i] > x)
      x = data[i];

  return x;
}

double argmax_imasked(double *data, int *mask, int i, int n) {

  int j, jmax;
  double x;

  j = 0;
  if (mask) {
    for (; j < n; j++)
      if (mask[j] == i)
	break;
    if (j == n)
      return n/0.0;
  }
  jmax = j;
  x = data[j++];
  for (; j < n; j++) {
    if ((!mask || mask[j] == i) && data[j] > x) {
      jmax = j;
      x = data[j];
    }
  }

  return jmax;
}

double argmax_masked(double *data, int *mask, int n) {

  int j, jmax;
  double x;

  j = 0;
  if (mask) {
    for (; j < n; j++)
      if (mask[j])
	break;
    if (j == n)
      return n/0.0;
  }
  jmax = j;
  x = data[j++];
  for (; j < n; j++) {
    if ((!mask || mask[j]) && data[j] > x) {
      jmax = j;
      x = data[j];
    }
  }

  return jmax;
}

double argmax(double *data, int n) {

  int i, imax;
  double x;

  x = data[0];
  imax = 0;
  for (i = 1; i < n; i++) {
    if (data[i] > x) {
      x = data[i];
      imax = i;
    }
  }

  return imax;
}

double mean_imasked(double *data, int *mask, int i, int n) {

  int j, cnt;
  double m;
  
  m = 0.0;
  cnt = 0;
  for (j = 0; j < n; j++) {
    if (!mask || mask[j] == i) {
      m += data[j];
      cnt++;
    }
  }
  m /= (double)cnt;

  return m;

}

/* mask[i] == 0 means ignore data[i] */
double mean_masked(double *data, int *mask, int n) {

  int i, cnt;
  double m;

  m = 0.0;

  if (mask == NULL) {
    for (i = 0; i < n; i++)
      m += data[i];
    m /= (double)n;
  }
  else {
    cnt = 0;
    for (i = 0; i < n; i++) {
      if (mask[i]) {
	m += data[i];
	cnt++;
      }
    }
    m /= (double)cnt;
  }

  return m;
}

double mean(double *data, int n) {

  int i;
  double m;

  m = 0.0;

  for (i = 0; i < n; i++)
    m += data[i];
  m /= (double)n;

  return m;
}

double var_imasked(double *data, double mean, int *mask, int i, int n) {

  int j, cnt;
  double v;
  
  v = 0.0;
  cnt = 0;
  for (j = 0; j < n; j++) {
    if (!mask || mask[j] == i) {
      v += (data[j]-mean)*(data[j]-mean);
      cnt++;
    }
  }
  v /= (double)cnt;

  return v;
}

double cov_imasked(double *xdata, double ux, double *ydata, double uy, int *mask, int i, int n) {

  int j, cnt;
  double v;
  
  v = 0.0;
  cnt = 0;
  for (j = 0; j < n; j++) {
    if (!mask || mask[j] == i) {
      v += (xdata[j]-ux)*(ydata[j]-uy);
      cnt++;
    }
  }
  v /= (double)cnt;

  return v;
}

inline double *new_vector(int n) {

  double *v = (double *) calloc(n, sizeof(double));
  carmen_test_alloc(v);

  return v;
}

inline int *new_ivector(int n) {

  int *v = (int *) calloc(n, sizeof(int));
  carmen_test_alloc(v);

  return v;
}

/*
 * allocates single (un-zeroed) block of memory; free with only one call to free(matrix)
 */
double **new_matrix(int num_rows, int num_cols) {

  double **m;
  int i;

  m = (double **) malloc(num_rows*sizeof(double *) + num_rows*num_cols*sizeof(double));
  carmen_test_alloc(m);
  
  for (i = 0; i < num_rows; i++)
    m[i] = (double *) (((char *)m) + num_rows*sizeof(double *) + i*num_cols*sizeof(double));

  return m;
}

/*
 * allocates single (un-zeroed) block of memory; free with only one call to free(matrix)
 */
int **new_imatrix(int num_rows, int num_cols) {

  int **m;
  int i;

  m = (int **) malloc(num_rows*sizeof(int *) + num_rows*num_cols*sizeof(int));
  carmen_test_alloc(m);
  
  for (i = 0; i < num_rows; i++)
    m[i] = (int *) (((char *)m) + num_rows*sizeof(int *) + i*num_cols*sizeof(int));

  return m;
}

double **matrix_resize(double **m, int xpos, int ypos, int old_x_size, int old_y_size, int new_x_size, int new_y_size, double pad) {

  double **m2;
  int i, j;

  if (xpos < 0 || ypos < 0) {
    carmen_warn("Error: bad input in matrix_resize()\n");
    return NULL;
  }

  m2 = new_matrix(new_x_size, new_y_size);
  for (i = 0; i < new_x_size; i++) {
    for (j = 0; j < new_y_size; j++) {
      if (i >= xpos && i < xpos + old_x_size && j >= ypos && j < ypos + old_y_size)
	m2[i][j] = m[i-xpos][j-ypos];
      else
	m2[i][j] = pad;
    }
  }

  free(m);
  return m2;
}

int **imatrix_resize(int **m, int xpos, int ypos, int old_x_size, int old_y_size, int new_x_size, int new_y_size, int pad) {

  int **m2;
  int i, j;

  if (xpos < 0 || ypos < 0) {
    carmen_warn("Error: bad input in matrix_resize()\n");
    return NULL;
  }

  m2 = new_imatrix(new_x_size, new_y_size);
  for (i = 0; i < new_x_size; i++) {
    for (j = 0; j < new_y_size; j++) {
      if (i >= xpos && i < xpos + old_x_size && j >= ypos && j < ypos + old_y_size)
	m2[i][j] = m[i-xpos][j-ypos];
      else
	m2[i][j] = pad;
    }
  }

  free(m);
  return m2;
}



// gift wrapping algorithm for finding convex hull: returns size of hull
int compute_convex_hull(double *xpoints, double *ypoints, int num_points, double *xhull, double *yhull) {
  
  int a, b, i, hull_size;
  static int cluster_mask[500];
  double last_angle, max_angle, theta, dx, dy;

  for (i = 0; i < num_points; i++)
    cluster_mask[i] = 0;

  a = argmin(ypoints, num_points);
  cluster_mask[a] = 1;
  xhull[0] = xpoints[a];
  yhull[0] = ypoints[a];
  hull_size = 1;

  last_angle = 0.0;  // 0 for clockwise, pi for counter-clockwise wrapping
  while (1) {
    max_angle = -1.0;
    b = -1;
    for (i = 0; i < num_points; i++) {
      if (i == a)
	continue;
      dx = xpoints[i] - xpoints[a];
      dy = ypoints[i] - ypoints[a];
      if (dist(dx, dy) == 0.0)
	continue;
      theta = fabs(carmen_normalize_theta(atan2(dy, dx) - last_angle));
      if (theta > max_angle) {
	max_angle = theta;
	b = i;
      }
    }
    if (cluster_mask[b])
      break;
    cluster_mask[b] = 1;
    xhull[hull_size] = xpoints[b];
    yhull[hull_size] = ypoints[b];
    hull_size++;
    dx = xpoints[a] - xpoints[b];
    dy = ypoints[a] - ypoints[b];
    last_angle = atan2(dy, dx);
    a = b;
  }

  return hull_size;
}

int ray_intersect_arg(double rx, double ry, double rtheta,
		      double x1, double y1, double x2, double y2) {

  static double epsilon = 0.01;  //dbug: param?

  if (fabs(rtheta) < M_PI/2.0 - epsilon) {
    if ((x1 < x2 || x2 < rx) && x1 >= rx)
      return 1;
    else if (x2 >= rx)
      return 2;
  }
  else if (fabs(rtheta) > M_PI/2.0 + epsilon) {
    if ((x1 > x2 || x2 > rx) && x1 <= rx)
      return 1;
    else if (x2 <= rx)
      return 2;    
  }
  else if (rtheta > 0.0) {
    if ((y1 < y2 || y2 < ry) && y1 >= ry)
      return 1;
    else if (y2 >= ry)
      return 2;
  }
  else {
    if ((y1 > y2 || y2 > ry) && y1 <= ry)
      return 1;
    else if (y2 <= ry)
      return 2;
  }

  return 0;
}

int ray_intersect_circle(double rx, double ry, double rtheta,
			 double cx, double cy, double cr,
			 double *px1, double *py1, double *px2, double *py2) {

  double epsilon = 0.000001;
  double a, b, c, tan_rtheta;
  double x1, y1, x2, y2;

  if (fabs(cos(rtheta)) < epsilon) {
    if (cr*cr - (rx-cx)*(rx-cx) < 0.0)
      return 0;
    x1 = x2 = rx;
    y1 = cy + sqrt(cr*cr-(rx-cx)*(rx-cx));
    y2 = cy - sqrt(cr*cr-(rx-cx)*(rx-cx));
    switch (ray_intersect_arg(rx, ry, rtheta, x1, y1, x2, y2)) {
    case 0:
      return 0;
    case 2:
      a = y1;
      y1 = y2;
      y2 = a;
    }
  }
  else {
    tan_rtheta = tan(rtheta);
    a = 1.0 + tan_rtheta*tan_rtheta;
    b = 2.0*tan_rtheta*(ry - rx*tan_rtheta - cy) - 2.0*cx;
    c = cx*cx + (ry - rx*tan_rtheta - cy)*(ry - rx*tan_rtheta - cy) - cr*cr;
    if (b*b - 4.0*a*c < 0.0)
      return 0;
    x1 = (-b + sqrt(b*b - 4.0*a*c))/(2.0*a);
    y1 = x1*tan_rtheta + ry - rx*tan_rtheta;
    x2 = (-b - sqrt(b*b - 4.0*a*c))/(2.0*a);
    y2 = x2*tan_rtheta + ry - rx*tan_rtheta;
    switch (ray_intersect_arg(rx, ry, rtheta, x1, y1, x2, y2)) {
    case 0:
      return 0;
    case 2:
      a = x1;
      x1 = x2;
      x2 = a;
      a = y1;
      y1 = y2;
      y2 = a;
    }
  }

  *px1 = x1;
  *py1 = y1;
  *px2 = x2;
  *py2 = y2;

  return 1;
}

int ray_intersect_convex_polygon(double rx, double ry, double rtheta,
				 double *xpoly, double *ypoly, int np,
				 double *px1, double *py1, double *px2, double *py2) {
  int i, cnt;
  double x, y, x1, y1, x2, y2, theta;
  double a, b, c, d;

  x1 = y1 = x2 = y2 = 0.0;

  a = tan(rtheta);
  b = ry - a*rx;

  cnt = 0;
  for (i = 0; i < np && cnt < 2; i++) {
    c = (ypoly[(i+1)%np] - ypoly[i]) / (xpoly[(i+1)%np] - xpoly[i]);
    d = ypoly[i] - c*xpoly[i];
    x = (d - b) / (a - c);
    y = a*x + b;
    theta = atan2(ypoly[(i+1)%np] - ypoly[i], xpoly[(i+1)%np] - xpoly[i]);
    if (ray_intersect_arg(xpoly[i], ypoly[i], theta, x, y, x, y) &&
	ray_intersect_arg(xpoly[(i+1)%np], ypoly[(i+1)%np],
			  carmen_normalize_theta(theta+M_PI), x, y, x, y) &&
	ray_intersect_arg(rx, ry, rtheta, x, y, x, y)) {
      if (cnt == 0) {
	x1 = x;
	y1 = y;
      }
      else {
	x2 = x;
	y2 = y;
      }
      cnt++;
    }
  }

  if (cnt == 0)
    return 0;

  if (cnt == 1) {
    //carmen_warn("ray_intersect_convex_polygon() found cnt = 1!\n");
    x2 = x1;
    y2 = y1;
  }
  
  switch (ray_intersect_arg(rx, ry, rtheta, x1, y1, x2, y2)) {
  case 0:
    carmen_die("ray_intersect_convex_polygon() error!");
  case 2:
    a = x1;
    x1 = x2;
    x2 = a;
    a = y1;
    y1 = y2;
    y2 = a;
  }

  *px1 = x1;
  *py1 = y1;
  *px2 = x2;
  *py2 = y2;

  return 1;
}

