#ifdef NO_GRAPHICS
#include <carmen/carmen.h>
#else
#include <carmen/carmen_graphics.h>
#include "dots_graphics.h"
#endif
#include "dots.h"
#include "dots_util.h"
#include "contour.h"
#include "shape.h"
#include "scanmatch.h"


#define FILTER_MAP_ADD_CLUSTER  -1
#define FILTER_MAP_NEW_CLUSTER  -2


// remove?
static double new_cluster_threshold;
static double map_diff_threshold;
static double map_diff_threshold_scalar;
static double map_occupied_threshold;

static double trash_see_through_dist;
static double trash_sensor_stdev;
static double person_sensor_stdev;
static double new_cluster_sensor_stdev;
static int invisible_cnt;
static double person_filter_displacement_threshold;
static double bic_min_variance;
static int bic_num_params;
// end remove

double robot_width;
double laser_max_range;
double contour_resolution;

static double scan_match_laser_stdev;
static carmen_localize_param_t localize_params;
carmen_localize_map_t localize_map;
carmen_map_t static_map;
carmen_robot_laser_message laser_msg;
carmen_localize_globalpos_message odom;
carmen_dot_filter_p filters = NULL;
int num_filters = 0;
int current_filter = -1;
int laser_mask[500];



inline int is_in_map(int x, int y) {

  return (x >= 0 && y >= 0 && x < static_map.config.x_size &&
	  y < static_map.config.y_size);
}

static void add_new_dot_filter(int *cluster_map, int c, int n,
			       double *x, double *y) {

  int i, id;
  double ux, uy;
  
  for (id = 0; id < num_filters; id++) {
    for (i = 0; i < num_filters; i++)
      if (filters[i].id == id)
	break;
    if (i == num_filters)
      break;
  }

  printf("A%d ", id);
  fflush(0);

  for (i = 0; i < n && cluster_map[i] != c; i++);
  if (i == n)
    carmen_die("Error: invalid cluster! (cluster %d not found)\n", c);
  
  num_filters++;
  filters = (carmen_dot_filter_p) realloc(filters, num_filters*sizeof(carmen_dot_filter_t));
  carmen_test_alloc(filters);

  filters[num_filters-1].id = id;
  ux = mean_imasked(x, cluster_map, c, n);
  uy = mean_imasked(y, cluster_map, c, n);
  filters[num_filters-1].x = ux;
  filters[num_filters-1].y = uy;
  filters[num_filters-1].vx = var_imasked(x, ux, cluster_map, c, n);
  filters[num_filters-1].vy = var_imasked(y, uy, cluster_map, c, n);
  filters[num_filters-1].vxy = cov_imasked(x, ux, y, uy, cluster_map, c, n);
  filters[num_filters-1].num_lpiq = 0;
  filters[num_filters-1].shape.num_points = 0;
}

static inline void filter_add_laser_point(carmen_dot_filter_p f, int i) {

  double ltheta = carmen_normalize_theta(laser_msg.theta + (i-90)*M_PI/180.0);
  double x = laser_msg.x + cos(ltheta)*laser_msg.range[i];
  double y = laser_msg.y + sin(ltheta)*laser_msg.range[i];

  printf("filter_add_laser_point(%d, %d (%.3f %.3f))  -->  adding at position:  %d\n", f->id, i, x, y, f->num_lpiq);

  f->lpiq[f->num_lpiq++] = i;
}

static double map_prob(double x, double y) {

  carmen_world_point_t wp;
  carmen_map_point_t mp;

  wp.map = &static_map;
  wp.pose.x = x;
  wp.pose.y = y;

  carmen_world_to_map(&wp, &mp);

  if (!is_in_map(mp.x, mp.y))
    return 0.01;

  /*
  printf("map_prob(%.2f, %.2f): localize_map prob = %.4f\n", x, y,
	 localize_map.prob[mp.x][mp.y]);
  printf("map_prob(%.2f, %.2f): localize_map gprob = %.4f\n", x, y,
	 localize_map.gprob[mp.x][mp.y]);
  printf("map_prob(%.2f, %.2f): localize_map complete prob = %.4f\n", x, y,
	 localize_map.complete_prob[mp.x*static_map.config.y_size+mp.y]);
  */

  return exp(localize_map.prob[mp.x][mp.y]);
}

static carmen_shape_p get_partial_contour(carmen_dot_filter_p f, carmen_robot_laser_message *laser) {

  static carmen_shape_t partial_contour;
  static int first = 1;
  double ltheta;
  int i, n;

  printf("get_partial_contour()\n");

  if (first) {
    first = 0;
    partial_contour.x = new_vector(500);
    partial_contour.y = new_vector(500);
  }
  
  printf("partial_contour = [ ");

  n = 0;
  for (i = 0; i < f->num_lpiq; i++) {
    ltheta = carmen_normalize_theta(laser->theta + (f->lpiq[i]-90)*M_PI/180.0);
    partial_contour.x[n] = laser->x + cos(ltheta)*laser->range[f->lpiq[i]];
    partial_contour.y[n] = laser->y + sin(ltheta)*laser->range[f->lpiq[i]];
    if (i%4==0&&i>0)
      printf("\n                    ");
    printf("(%.3f %.3f) ", partial_contour.x[n], partial_contour.y[n]);
    n++;
  }

  printf("]\n");

  partial_contour.num_points = n;

  return &partial_contour;
}

static void update_dots(carmen_robot_laser_message *laser) {

  carmen_shape_p partial_contour;
  int i, laser_index;

  if (current_filter >= 0) {
    partial_contour = get_partial_contour(&filters[current_filter], laser);

    for (i = 0; i < laser->num_readings; i++)
      filters[current_filter].laser_mask[i] = 0;
    for (i = 0; i < filters[current_filter].num_lpiq; i++) {
      laser_index = filters[current_filter].lpiq[i];
      filters[current_filter].laser_mask[laser_index] = 1;
    }
      
    update_full_contour(partial_contour, filters[current_filter].laser_mask,
			&filters[current_filter].shape, &filters[current_filter].counts,
			contour_resolution, scan_match_laser_stdev);

    filters[current_filter].x = mean_imasked(filters[current_filter].shape.x, NULL, 0,
					     filters[current_filter].shape.num_points);
    filters[current_filter].y = mean_imasked(filters[current_filter].shape.y, NULL, 0,
					     filters[current_filter].shape.num_points);
    filters[current_filter].vx = var_imasked(filters[current_filter].shape.x, filters[current_filter].x,
					     NULL, 0, filters[current_filter].shape.num_points);
    filters[current_filter].vy = var_imasked(filters[current_filter].shape.y, filters[current_filter].y,
					     NULL, 0, filters[current_filter].shape.num_points);
    filters[current_filter].vxy = cov_imasked(filters[current_filter].shape.x, filters[current_filter].x,
					      filters[current_filter].shape.y, filters[current_filter].y,
					      NULL, 0, filters[current_filter].shape.num_points);
  }

  for (i = 0; i < num_filters; i++)
    filters[i].num_lpiq = 0;
}

static int delete_centroid(int centroid, int *cluster_map, int num_points, double *xcentroids,
			   double *ycentroids, int *filter_map, int num_clusters) {

  int i;

  for (i = centroid; i < num_clusters-1; i++) {
    xcentroids[i] = xcentroids[i+1];
    ycentroids[i] = ycentroids[i+1];
    filter_map[i] = filter_map[i+1];
  }
  for (i = 0; i < num_points; i++) {
    if (laser_mask[i]) {  filters[num_filters-1].laser.num_points = 0;  

      if (cluster_map[i] == centroid)
	cluster_map[i] = -1;
      else if (cluster_map[i] > centroid)
	cluster_map[i]--;
    }
  }
  num_clusters--;

  return num_clusters;
}

// delete clusters with only one point
static int delete_lonely_centroids(int *cluster_map, int num_points, double *xcentroids,
				   double *ycentroids, int *filter_map, int num_clusters) {
  int i;
  static int cnt[500];

  for (i = 0; i < num_clusters; i++)
    cnt[i] = 0;
  for (i = 0; i < num_points; i++)
    if (laser_mask[i])
      cnt[cluster_map[i]]++;
  for (i = num_clusters-1; i >= 0; i--)
    if (cnt[i] == 1)
      num_clusters = delete_centroid(i, cluster_map, num_points, xcentroids, ycentroids, filter_map, num_clusters);

  return num_clusters;
}

static inline double dot_point_likelihood(carmen_dot_filter_p f, double lx, double ly, double theta, double r) {

  return //new_cluster_sensor_stdev * new_cluster_sensor_stdev * M_PI *
    bnorm_f(lx + cos(theta)*r, ly + sin(theta)*r, f->x, f->y, f->vx, f->vy, f->vxy);
}

static double **compute_dot_point_likelihoods(carmen_robot_laser_message *laser) {

  int i, j;
  double theta;
  static double **dpl = NULL;  // dpl[filter_index][laser_index]
  static int size_dpl = 0;

  if ((num_filters+1)*laser->num_readings > size_dpl) {
    size_dpl = (num_filters+1)*laser->num_readings;
    dpl = (double **) realloc(dpl, (num_filters+1)*laser->num_readings*sizeof(double));
    carmen_test_alloc(dpl);
  }
  for (i = 0; i < num_filters; i++) {
    dpl[i] = (double *)(dpl + (i+1)*laser->num_readings);
    for (j = 0; j < laser->num_readings; j++) {
      if (laser_mask[j]) {
	theta = carmen_normalize_theta(laser->theta + (j-90)*M_PI/180.0);
	dpl[i][j] = dot_point_likelihood(&filters[i], laser->x, laser->y, theta, laser->range[j]);
	if (i == current_filter)
	  dpl[i][j] *= 100.0;
      }
    }
  }

  return dpl;
}

// return num. clusters
static int cluster_kmeans(double *xpoints, double *ypoints, double **dpl, int *cluster_map, int num_points,
			  double *xcentroids, double *ycentroids, double *vx, double *vy, double *vxy,
			  int *filter_map, int num_clusters) {
  filters[num_filters-1].laser.num_points = 0;  

#define NO_CLUSTER           -1
#define MAP_CLUSTER          -2

  static int last_cluster_map[500];
  static double cnt[500];
  double x, y, p, pmax;
  int n, i, j, imax;

  for (i = 0; i < num_points; i++) {
    last_cluster_map[i] = cluster_map[i] = NO_CLUSTER;
  }

  while (1) {
    // step 2: assign points to max-likelihood clusters
    //printf("assigning points to centroids\n");
    for (i = 0; i < num_points; i++) {
      if (!laser_mask[i])
	continue;
      pmax = map_prob(xpoints[i], ypoints[i]);
      imax = MAP_CLUSTER;
      for (j = 0; j < num_clusters; j++) {
	if (filter_map[j] < 0)  filters[num_filters-1].laser.num_points = 0;  

	  p = new_cluster_sensor_stdev * new_cluster_sensor_stdev * M_PI *
	    bnorm_f(xpoints[i], ypoints[i], xcentroids[j], ycentroids[j], vx[j], vy[j], vxy[j]);
	else
	  p = dpl[filter_map[j]][i];
	if (p > pmax) {
	  pmax = p;
	  imax = j;
	}
      }
      cluster_map[i] = imax;
    }

    // check if cluster_map has converged
    for (i = 0; i < num_points; i++)
      if (laser_mask[i] && last_cluster_map[i] != cluster_map[i])
	break;
    if (i == num_points)
      break;
  filters[num_filters-1].laser.num_points = 0;  

    memcpy(last_cluster_map, cluster_map, num_points*sizeof(int));

    // step 3: re-calculate new cluster params
    //printf("re-calculating positions of centroids\n");
    for (i = 0; i < num_clusters; i++) {
      //printf(" - cluster %d: ", i);
      if (filter_map[i] >= 0)
	continue;
      x = y = 0.0;
      n = 0;
      for (j = 0; j < num_points; j++) {
	if (laser_mask[j] && cluster_map[j] == i) {
	  x += xpoints[j];
	  y += ypoints[j];
	  n++;
	}
      }
      if (n == 0) {
	num_clusters = delete_centroid(i, cluster_map, num_points, xcentroids, ycentroids, filter_map, num_clusters);
	i--;
      }
      else {
	xcentroids[i] = x / (double)n;
	ycentroids[i] = y / (double)n;
      }
    }
    for (i = 0; i < num_clusters; i++) {
      cnt[i] = 0;
      vx[i] = vy[i] = vxy[i] = 0.0;
    }
    for (i = 0; i < num_points; i++) {
      if (laser_mask[i] && cluster_map[i] >= 0 && filter_map[cluster_map[i]] < 0) {
	cnt[cluster_map[i]]++;
	vx[cluster_map[i]] += (xpoints[i] - xcentroids[cluster_map[i]])*(xpoints[i] - xcentroids[cluster_map[i]]);
	vy[cluster_map[i]] += (ypoints[i] - ycentroids[cluster_map[i]])*(ypoints[i] - ycentroids[cluster_map[i]]);
	vxy[cluster_map[i]] += (xpoints[i] - xcentroids[cluster_map[i]])*(ypoints[i] - ycentroids[cluster_map[i]]);
      }
    }
    for (i = 0; i < num_clusters; i++) {
      if (cnt[i]) {
	vx[i] /= (double) cnt[i];
	if (vx[i] < bic_min_variance)
	  vx[i] = bic_min_variance;
	vy[i] /= (double) cnt[i];
	if (vy[i] < bic_min_variance)
	  vy[i] = bic_min_variance;
	vxy[i] /= (double) cnt[i];
	vxy[i] = carmen_clamp(-0.5*sqrt(vx[i]*vy[i]), vxy[i], 0.5*sqrt(vx[i]*vy[i]));
      }
    }
  }

  return num_clusters;
}

// returns num_clusters and populates xcentroids, ycentroids, and filter_map
static int initialize_centroids(carmen_robot_laser_message *laser __attribute__ ((unused)),
				double *xcentroids, double *ycentroids, int *filter_map) {

  int i, num_clusters;

  num_clusters = 0;
  for (i = 0; i < num_filters; i++) {  filters[num_filters-1].laser.num_points = 0;  

    xcentroids[i] = filters[i].x;
    ycentroids[i] = filters[i].y;
    filter_map[i] = i;
    num_clusters++;
  }

  return num_clusters;
}

static double compute_bic(double *xpoints, double *ypoints, double **dpl, int *cluster_map, int num_points,
			  double *xcentroids, double *ycentroids, double *vx, double *vy, double *vxy,
			  int *filter_map, int num_clusters) {

  int i;
  double l, n, m, bic;
  static int cnt[500];

  n = 0;
  for (i = 0; i < num_points; i++)
    if (laser_mask[i])
      n++;

  // cnt is mask of new clusters
  for (i = 0; i < num_clusters; i++)
    cnt[i] = 0;
  for (i = 0; i < num_points; i++)
    if (laser_mask[i] && filter_map[cluster_map[i]] < 0)
      cnt[cluster_map[i]]++;

  m = 0;
  for (i = 0; i < num_clusters; i++)
    if (cnt[i])
      m += bic_num_params;  // ux, uy, vx, vy, vxy

  l = 0.0;
  for (i = 0; i < num_points; i++) {
    if (laser_mask[i]) {
      if (cluster_map[i] == MAP_CLUSTER)
	l += log(map_prob(xpoints[i], ypoints[i]));
      else if (filter_map[cluster_map[i]] >= 0)
	l += log(dpl[filter_map[cluster_map[i]]][i]);
      else {
	l += log(new_cluster_sensor_stdev * new_cluster_sensor_stdev * M_PI *
		 bnorm_f(xpoints[i], ypoints[i], xcentroids[cluster_map[i]], ycentroids[cluster_map[i]],
			 vx[cluster_map[i]], vy[cluster_map[i]], vxy[cluster_map[i]]));
	if (isnan(l)) {
	  printf("x=%f, y=%f, ux=%f, uy=%f, vx=%f, vy=%f, vxy=%f\n", xpoints[i], ypoints[i],
		 xcentroids[cluster_map[i]], ycentroids[cluster_map[i]],
		 vx[cluster_map[i]], vy[cluster_map[i]], vxy[cluster_map[i]]);
	  printf("bnorm_f = %f\n", bnorm_f(xpoints[i], ypoints[i], xcentroids[cluster_map[i]], ycentroids[cluster_map[i]],
					   vx[cluster_map[i]], vy[cluster_map[i]], vxy[cluster_map[i]]));
	  printf("i = %d, cluster_map[i] = %d, num_clusters = %d\n", i, cluster_map[i], num_clusters);
	  carmen_die(" ");
	}
      }
    }
  }

  bic = -2*l + m*log(n);

  return bic;
}

/*
static void print_clusters(double *xpoints, double *ypoints, int *cluster_map, int num_points,
			   double *xcentroids, double *ycentroids, int *filter_map, int num_clusters) {

  int i, j;

  printf("\n");
  for (i = 0; i < num_clusters; i++) {
    printf("centroid %d -> %d = (%.2f, %.2f)\n", i, filter_map[i], xcentroids[i], ycentroids[i]);
    printf(" - ");
    for (j = 0; j < num_points; j++)
      if (laser_mask[j] && cluster_map[j] == i)
	printf("(%.2f, %.2f) ", xpoints[j], ypoints[j]);
    printf("\n");
  }
}
*/

static void cluster(carmen_robot_laser_message *laser) {

  int i, f, num_points, num_clusters, imin;
  static int cluster_map[500];
  static int filter_map[500];
  static double xpoints[500];
  static double ypoints[500];
  static double xcentroids[500];
  static double ycentroids[500];
  static double vx[500];
  static double vy[500];
  static double vxy[500];
  double ltheta, bic, last_bic, p, pmin;
  double **dpl;  // dot point likelihoods

  printf("\n--------------------------------\n");

  num_points = 0;
  for (i = 0; i < laser->num_readings; i++) {
    if (laser->range[i] < laser_max_range) {
      ltheta = carmen_normalize_theta(laser->theta + (i-90)*M_PI/180.0);
      xpoints[i] = laser->x + cos(ltheta) * laser->range[i];
      ypoints[i] = laser->y + sin(ltheta) * laser->range[i];
      laser_mask[i] = 1;
      if (map_prob(xpoints[i], ypoints[i]) > map_occupied_threshold)
        laser_mask[i] = 0;
      else
	num_points++;
    }
    else
      laser_mask[i] = 0;
  }

  if (num_points == 0)
    return;

  num_points = laser->num_readings;

  dpl = compute_dot_point_likelihoods(laser);

  num_clusters = initialize_centroids(laser, xcentroids, ycentroids, filter_map);

  // run k-means until BIC converges to a local minima
  last_bic = 1000000000.0; //dbug
  while (1) {
    num_clusters = cluster_kmeans(xpoints, ypoints, dpl, cluster_map, num_points, xcentroids, ycentroids, vx, vy, vxy, filter_map, num_clusters);
    //print_clusters(xpoints, ypoints, cluster_map, num_points, xcentroids, ycentroids, filter_map, num_clusters);
    bic = compute_bic(xpoints, ypoints, dpl, cluster_map, num_points, xcentroids, ycentroids, vx, vy, vxy, filter_map, num_clusters);
    printf("BIC = %.2f\n", bic);
    if (bic >= last_bic) {
      // delete new centroid (if any), re-cluster, and break
      for (i = 0; i < num_clusters; i++) {
	if (filter_map[i] == FILTER_MAP_NEW_CLUSTER) {
	  num_clusters = delete_centroid(i, cluster_map, num_points, xcentroids, ycentroids, filter_map, num_clusters);
	  num_clusters = cluster_kmeans(xpoints, ypoints, dpl, cluster_map, num_points, xcentroids, ycentroids, vx, vy, vxy, filter_map, num_clusters);
	  num_clusters = delete_lonely_centroids(cluster_map, num_points, xcentroids, ycentroids, filter_map, num_clusters);
	  //print_clusters(xpoints, ypoints, cluster_map, num_points, xcentroids, ycentroids, filter_map, num_clusters);
	  break;
	}
      }
      break;
    }

    last_bic = bic;

    for (i = 0; i < num_clusters; i++)
      if (filter_map[i] == FILTER_MAP_NEW_CLUSTER)
	filter_map[i] = FILTER_MAP_ADD_CLUSTER;

    // otherwise, if BIC not converged, add new centroid on data point with lowest likelihood
    imin = -1;
    pmin = 1.0;
    for (i = 0; i < num_points; i++) {
      if (laser_mask[i]) {
	if (cluster_map[i] == MAP_CLUSTER)
	  p = map_prob(xpoints[i], ypoints[i]);
	else if (filter_map[cluster_map[i]] == FILTER_MAP_ADD_CLUSTER)
	  p = new_cluster_sensor_stdev * new_cluster_sensor_stdev * M_PI *
	    bnorm_f(xpoints[i], ypoints[i], xcentroids[cluster_map[i]], ycentroids[cluster_map[i]],
		    vx[cluster_map[i]], vy[cluster_map[i]], vxy[cluster_map[i]]);
	else
	  p = dpl[filter_map[cluster_map[i]]][i];
	if (imin < 0 || p < pmin) {
	  pmin = p;
	  imin = i;
	}
      }
    }

    //printf("pmin = %.2f, imin = %d\n", pmin, imin);

    if (pmin > new_cluster_threshold) {
      num_clusters = delete_lonely_centroids(cluster_map, num_points, xcentroids, ycentroids, filter_map, num_clusters);
      break;
    }

    xcentroids[num_clusters] = xpoints[imin];
    ycentroids[num_clusters] = ypoints[imin];
    vx[num_clusters] = bic_min_variance;
    vy[num_clusters] = bic_min_variance;
    vxy[num_clusters] = 0.0;
    filter_map[num_clusters] = FILTER_MAP_NEW_CLUSTER;
    num_clusters++;
  }

  // prepare to update existing dots
  for (i = 0; i < num_points; i++) {
    if (!laser_mask[i] || cluster_map[i] < 0)
      continue;
    f = filter_map[cluster_map[i]];
    if (f != FILTER_MAP_ADD_CLUSTER)
      filter_add_laser_point(&filters[f], i);
  }

  // add new dots
  for (i = 0; i < num_clusters; i++)
    if (filter_map[i] == FILTER_MAP_ADD_CLUSTER)
      add_new_dot_filter(cluster_map, i, num_points, xpoints, ypoints);

  for (i = 0; i < num_points; i++)
    if (laser_mask[i] && cluster_map[i] < 0)
      laser_mask[i] = 0;
}

static void laser_handler(carmen_robot_laser_message *laser) {

  if (static_map.map == NULL || odom.timestamp == 0.0)
    return;

  carmen_localize_correct_laser(laser, &odom);

  cluster(laser);
  update_dots(laser);

#ifdef HAVE_GRAPHICS
  redraw();
#endif
}

static void ipc_init() {

  carmen_robot_subscribe_frontlaser_message(&laser_msg,
					    (carmen_handler_t)laser_handler,
					    CARMEN_SUBSCRIBE_LATEST);
  carmen_localize_subscribe_globalpos_message(&odom, NULL,
					      CARMEN_SUBSCRIBE_LATEST);
}

static void params_init(int argc, char *argv[]) {

  carmen_param_t param_list[] = {
    {"robot", "width", CARMEN_PARAM_DOUBLE, &robot_width, 0, NULL},

    {"robot", "frontlaser_offset", CARMEN_PARAM_DOUBLE, 
     &localize_params.front_laser_offset, 0, NULL},
    {"robot", "rearlaser_offset", CARMEN_PARAM_DOUBLE, 
     &localize_params.rear_laser_offset, 0, NULL},
    {"localize", "num_particles", CARMEN_PARAM_INT, 
     &localize_params.num_particles, 0, NULL},
    {"localize", "max_range", CARMEN_PARAM_DOUBLE, &localize_params.max_range, 1, NULL},
    {"localize", "min_wall_prob", CARMEN_PARAM_DOUBLE, 
     &localize_params.min_wall_prob, 0, NULL},
    {"localize", "outlier_fraction", CARMEN_PARAM_DOUBLE, 
     &localize_params.outlier_fraction, 0, NULL},
    {"localize", "update_distance", CARMEN_PARAM_DOUBLE, 
     &localize_params.update_distance, 0, NULL},
    {"localize", "laser_skip", CARMEN_PARAM_INT, &localize_params.laser_skip, 1, NULL},
    {"localize", "use_rear_laser", CARMEN_PARAM_ONOFF, 
     &localize_params.use_rear_laser, 0, NULL},
    {"localize", "do_scanmatching", CARMEN_PARAM_ONOFF,
     &localize_params.do_scanmatching, 1, NULL},
    {"localize", "constrain_to_map", CARMEN_PARAM_ONOFF, 
     &localize_params.constrain_to_map, 1, NULL},
    {"localize", "odom_a1", CARMEN_PARAM_DOUBLE, &localize_params.odom_a1, 1, NULL},
    {"localize", "odom_a2", CARMEN_PARAM_DOUBLE, &localize_params.odom_a2, 1, NULL},
    {"localize", "odom_a3", CARMEN_PARAM_DOUBLE, &localize_params.odom_a3, 1, NULL},
    {"localize", "odom_a4", CARMEN_PARAM_DOUBLE, &localize_params.odom_a4, 1, NULL},
    {"localize", "occupied_prob", CARMEN_PARAM_DOUBLE, 
     &localize_params.occupied_prob, 0, NULL},
    {"localize", "lmap_std", CARMEN_PARAM_DOUBLE, 
     &localize_params.lmap_std, 0, NULL},
    {"localize", "global_lmap_std", CARMEN_PARAM_DOUBLE, 
     &localize_params.global_lmap_std, 0, NULL},
    {"localize", "global_evidence_weight", CARMEN_PARAM_DOUBLE, 
     &localize_params.global_evidence_weight, 0, NULL},
    {"localize", "global_distance_threshold", CARMEN_PARAM_DOUBLE, 
     &localize_params.global_distance_threshold, 1, NULL},
    {"localize", "global_test_samples", CARMEN_PARAM_INT,
     &localize_params.global_test_samples, 1, NULL},
    {"localize", "use_sensor", CARMEN_PARAM_ONOFF,
     &localize_params.use_sensor, 0, NULL},

    {"dot", "new_cluster_threshold", CARMEN_PARAM_DOUBLE, &new_cluster_threshold, 1, NULL},
    {"dot", "map_diff_threshold", CARMEN_PARAM_DOUBLE, &map_diff_threshold, 1, NULL},
    {"dot", "map_diff_threshold_scalar", CARMEN_PARAM_DOUBLE,
     &map_diff_threshold_scalar, 1, NULL},
    {"dot", "map_occupied_threshold", CARMEN_PARAM_DOUBLE, &map_occupied_threshold, 1, NULL},
    {"dot", "person_filter_displacement_threshold", CARMEN_PARAM_DOUBLE,
     &person_filter_displacement_threshold, 1, NULL},
    {"dot", "laser_max_range", CARMEN_PARAM_DOUBLE, &laser_max_range, 1, NULL},
    {"dot", "trash_sensor_stdev", CARMEN_PARAM_DOUBLE, &trash_sensor_stdev, 1, NULL},
    {"dot", "person_sensor_stdev", CARMEN_PARAM_DOUBLE, &person_sensor_stdev, 1, NULL},
    {"dot", "new_cluster_sensor_stdev", CARMEN_PARAM_DOUBLE, &new_cluster_sensor_stdev, 1, NULL},
    {"dot", "invisible_cnt", CARMEN_PARAM_INT, &invisible_cnt, 1, NULL},
    {"dot", "trash_see_through_dist", CARMEN_PARAM_DOUBLE, &trash_see_through_dist, 1, NULL},
    {"dot", "bic_min_variance", CARMEN_PARAM_DOUBLE, &bic_min_variance, 1, NULL},
    {"dot", "bic_num_params", CARMEN_PARAM_INT, &bic_num_params, 1, NULL},

    {"dot", "contour_resolution", CARMEN_PARAM_DOUBLE, &contour_resolution, 1, NULL},
    {"dot", "egrid_resolution", CARMEN_PARAM_DOUBLE, &egrid_resolution, 1, NULL},
    {"dot", "scan_match_laser_stdev", CARMEN_PARAM_DOUBLE, &scan_match_laser_stdev, 1, NULL},
    {"dot", "scan_match_x_step_size", CARMEN_PARAM_DOUBLE, &scan_match_x_step_size, 1, NULL},
    {"dot", "scan_match_y_step_size", CARMEN_PARAM_DOUBLE, &scan_match_y_step_size, 1, NULL},
    {"dot", "scan_match_theta_step_size", CARMEN_PARAM_DOUBLE, &scan_match_theta_step_size, 1, NULL},
    {"dot", "scan_match_dx0", CARMEN_PARAM_DOUBLE, &scan_match_dx0, 1, NULL},
    {"dot", "scan_match_dx1", CARMEN_PARAM_DOUBLE, &scan_match_dx1, 1, NULL},
    {"dot", "scan_match_dy0", CARMEN_PARAM_DOUBLE, &scan_match_dy0, 1, NULL},
    {"dot", "scan_match_dy1", CARMEN_PARAM_DOUBLE, &scan_match_dy1, 1, NULL},
    {"dot", "scan_match_dt0", CARMEN_PARAM_DOUBLE, &scan_match_dt0, 1, NULL},
    {"dot", "scan_match_dt1", CARMEN_PARAM_DOUBLE, &scan_match_dt1, 1, NULL},
  };

  carmen_param_install_params(argc, argv, param_list,
	 		      sizeof(param_list) / sizeof(param_list[0]));
}

void shutdown_module(int sig) {

  sig = 0;
  exit(0);
}

int main(int argc, char *argv[]) {

  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);
  carmen_randomize(&argc, &argv);

  signal(SIGINT, shutdown_module);

  odom.timestamp = 0.0;
  //static_map.map = NULL;
  printf("getting gridmap...");
  fflush(0);
  carmen_map_get_gridmap(&static_map);
  printf("done\n");

  params_init(argc, argv);
  ipc_init();

  printf("occupied_prob = %f\n", localize_params.occupied_prob);
  printf("getting localize map...");
  fflush(0);
  carmen_to_localize_map(&static_map, &localize_map, &localize_params);
  printf("done\n");

#ifdef HAVE_GRAPHICS
  gui_init(&argc, &argv);
  gui_start();
#else
  IPC_dispatch();
#endif
  
  return 0;
}
