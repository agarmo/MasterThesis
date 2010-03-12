#include <carmen/carmen.h>
#include "contour.h"
#include "scanmatch.h"


carmen_shape_t last_partial_contour;


static inline double dist(double x1, double y1, double x2, double y2) {

  double dx, dy;

  dx = x1-x2;
  dy = y1-y2;

  return sqrt(dx*dx+dy*dy);
}

static inline double *new_vector(int n) {

  double *v = (double *) calloc(n, sizeof(double));
  carmen_test_alloc(v);

  return v;
}

static inline int *new_ivector(int n) {

  int *v = (int *) calloc(n, sizeof(int));
  carmen_test_alloc(v);

  return v;
}

static void resolution_filter(carmen_shape_p contour, int *counts, double resolution) {

  int i, last;
  double *x, *y;

  last = 0;
  x = contour->x;
  y = contour->y;
  for (i = 1; i < contour->num_points; i++) {
    if (dist(x[last], y[last], x[i], y[i]) < resolution ||
	(last > 0 && dist(x[last-1], y[last-1], x[i], y[i]) < resolution))
      continue;
    else {
      last++;
      x[last] = x[i];
      y[last] = y[i];
      counts[last] = counts[i];
    }
  }
  
  contour->x = (double *) realloc(contour->x, (last+1)*sizeof(double));
  carmen_test_alloc(contour->x);
  contour->y = (double *) realloc(contour->y, (last+1)*sizeof(double));
  carmen_test_alloc(contour->y);
  contour->num_points = last+1;

  counts = (int *) realloc(counts, (last+1)*sizeof(int));
  carmen_test_alloc(counts);  
}

static void print_contour(carmen_shape_p contour) {

  int j;

  printf("\t[ ");
  for (j = 0; j < contour->num_points; j++) {
    if (j%4==0&&j>0)
      printf("\n\t  ");
    printf("(%.3f %.3f) ", contour->x[j], contour->y[j]);
  }
  printf("]\n");
}

int update_full_contour(carmen_shape_p new_contour, int *laser_mask, carmen_shape_p mean_contour,
			int **counts_ptr, double resolution, double laser_stdev) {

  int *ids, *cnts, *new_cnts, *shift, *new_point_ids, *new_points_shift;
  int i, j, j0, j1, imin, num_new_points, n;
  double dmin, d, d2, *new_ux, *new_uy, x, y;

  printf("update_full_contour()\n");

  if (new_contour == NULL || mean_contour == NULL || counts_ptr == NULL) {
    carmen_warn("Error: bad input in update_full_contour()\n");
    return -1;
  }

  scan_match(new_contour, laser_mask, mean_contour, resolution, laser_stdev, NULL, NULL);

  if (mean_contour->num_points == 0) {  // first update
    *counts_ptr = new_ivector(new_contour->num_points);
    mean_contour->x = new_vector(new_contour->num_points);
    mean_contour->y = new_vector(new_contour->num_points);
    mean_contour->num_points = new_contour->num_points;
    for (i = 0; i < mean_contour->num_points; i++) {
      (*counts_ptr)[i] = 1;
      mean_contour->x[i] = new_contour->x[i];
      mean_contour->y[i] = new_contour->y[i];
    }

    printf("mean_contour (first alloc) = \n");
    print_contour(mean_contour);

    resolution_filter(mean_contour, *counts_ptr, resolution);
    printf("mean_contour (after resolution_filter()) = \n");
    print_contour(mean_contour);

    return 0;
  }

  last_partial_contour = *new_contour;

  // get correspondences
  cnts = *counts_ptr;
  ids = new_ivector(new_contour->num_points);
  num_new_points = 0;
  for (i = 0; i < new_contour->num_points; i++) {
    // find closest point on mean_contour; O(n*m)
    imin = 0;
    dmin = dist(new_contour->x[i], new_contour->y[i], mean_contour->x[0], mean_contour->y[0]);
    for (j = 1; j < mean_contour->num_points; j++) {
      d = dist(new_contour->x[i], new_contour->y[i], mean_contour->x[j], mean_contour->y[j]);
      if (d < dmin) {
	dmin = d;
	imin = j;
      }
    }
    if (dmin < resolution)  // correspondence
      ids[i] = imin;
    else {                  // new point
      ids[i] = ~imin;
      num_new_points++;
    }
  }

  printf("num_new_points = %d\n", num_new_points);

  new_ux = new_vector(mean_contour->num_points);
  new_uy = new_vector(mean_contour->num_points);
  new_cnts = new_ivector(mean_contour->num_points);

  // average similarly matched points
  for (i = 0; i < new_contour->num_points; i++) {
    j = ids[i];
    if (j >= 0) {
      new_ux[j] += new_contour->x[i];
      new_uy[j] += new_contour->y[i];
      new_cnts[j]++;
    }
  }
  for (j = 0; j < mean_contour->num_points; j++) {
    if (new_cnts[j]) {
      new_ux[j] /= (double)new_cnts[j];
      new_uy[j] /= (double)new_cnts[j];
    }
    // add back in old mean_contour points, weighted by cnts[j]
    new_ux[j] += cnts[j]*mean_contour->x[j];
    new_uy[j] += cnts[j]*mean_contour->y[j];
    if (new_cnts[j])
      cnts[j]++;
    new_ux[j] /= (double)cnts[j];
    new_uy[j] /= (double)cnts[j];
  }

  printf("new_point_ids = ");

  // next, we need to add new points
  // first, figure out where to insert them
  new_point_ids = new_ivector(num_new_points);
  shift = new_ivector(mean_contour->num_points + 1);
  n = 0;
  for (i = 0; i < new_contour->num_points; i++) {
    j = ids[i];
    if (j < 0) {  // new point
      j = ~j;
      new_point_ids[n++] = i;

      printf("%d ", new_point_ids[n-1]);

      j0 = (j > 0 ? j-1 : mean_contour->num_points-1);  // prev point index
      j1 = (j < mean_contour->num_points-1 ? j+1 : 0);  // next point index
      x = new_contour->x[i];
      y = new_contour->y[i];
      d = dist(new_ux[j0], new_uy[j0], x, y)
	+ dist(x, y, new_ux[j], new_uy[j])
	+ dist(new_ux[j], new_uy[j], new_ux[j1], new_uy[j1]);
      d2 = dist(new_ux[j0], new_uy[j0], new_ux[j], new_uy[j])
	+ dist(new_ux[j], new_uy[j], x, y)
	+ dist(x, y, new_ux[j1], new_uy[j1]);

      if (d < d2) {  // insert before
	ids[i] = j;
	shift[j]++;
      }
      else {         // insert after
	ids[i] = j+1;
	shift[j+1]++;
      }
    }
  }

  printf("\n");
  
  printf("shift = ");

  n = 0;
  for (j = 0; j < mean_contour->num_points + 1; j++) {
    shift[j] += n;

    printf("%d ", shift[j]);

    n = shift[j];
  }

  printf("\n");

  // now we've got new_point_ids[] and shift[], so allocate some space and add points
  mean_contour->num_points += num_new_points;
  mean_contour->x = (double *) realloc(mean_contour->x, mean_contour->num_points * sizeof(double));
  carmen_test_alloc(mean_contour->x);
  mean_contour->y = (double *) realloc(mean_contour->y, mean_contour->num_points * sizeof(double));
  carmen_test_alloc(mean_contour->y);

  // insert old points
  for (j = 0; j < mean_contour->num_points - num_new_points; j++) {
    j1 = j + shift[j];
    mean_contour->x[j1] = new_ux[j];
    mean_contour->y[j1] = new_uy[j];
  }

  // insert new points
  new_points_shift = new_ivector(mean_contour->num_points - num_new_points + 1);
  for (n = 0; n < num_new_points; n++) {
    i = new_point_ids[n];
    j = ids[i];
    j1 = j + new_points_shift[j];
    if (j > 0)
      j1 += shift[j-1];
    mean_contour->x[j1] = new_contour->x[i];
    mean_contour->y[j1] = new_contour->y[i];
    new_points_shift[j]++;
  }

  // next, update counts
  *counts_ptr = (int *) realloc(*counts_ptr, mean_contour->num_points * sizeof(int));
  carmen_test_alloc(*counts_ptr);
  cnts = *counts_ptr;

  // insert old point counts (in place)
  for (j = mean_contour->num_points - num_new_points - 1; j >= 0; j--) {
    j1 = j + shift[j];
    cnts[j1] = cnts[j];
  }

  // set all new point counts to 1
  for (j1 = 0; j1 < mean_contour->num_points; j1++)
    if (cnts[j1] == 0)
      cnts[j1] = 1;

  printf("mean_contour = \n");
  print_contour(mean_contour);

  free(ids);
  free(new_ux);
  free(new_uy);
  free(new_cnts);
  free(new_point_ids);
  free(shift);
  free(new_points_shift);

  resolution_filter(mean_contour, *counts_ptr, resolution);
  return 0;  
}
