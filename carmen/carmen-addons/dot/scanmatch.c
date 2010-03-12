
#include <carmen/carmen.h>
#include "scanmatch.h"
#include "dots_util.h"

static const int EGRID_MASK_STDEV_MULT = 2;
static const double EGRID_LOW_LOG_PROB_STDEV_MULT = 2 * EGRID_MASK_STDEV_MULT;
// simplification of:
//    d = EGRID_LOW_LOG_PROB_STDEV_MULT * laser_stdev;
//    low_log_prob = -d*d/(2*laser_stdev*laser_stdev);
static const double LOW_POG_PROB = -EGRID_LOW_LOG_PROB_STDEV_MULT * EGRID_LOW_LOG_PROB_STDEV_MULT / 2.0;



double **egrid = NULL;
int **egrid_counts = NULL;
int **egrid_hits = NULL;
int egrid_x_size;
int egrid_y_size;
double egrid_x_offset;
double egrid_y_offset;
double egrid_resolution;
carmen_point_t scan_match_delta;

double scan_match_x_step_size;
double scan_match_y_step_size;
double scan_match_theta_step_size;
double scan_match_dx0;
double scan_match_dx1;
double scan_match_dy0;
double scan_match_dy1;
double scan_match_dt0;
double scan_match_dt1;


static void update_egrid_cell(int x, int y, int h) {

  egrid_counts[x][y]++;
  if (h)
    egrid_hits[x][y]++;
  egrid[x][y] = egrid_hits[x][y] / (double) egrid_counts[x][y];
}

static void trace_laser_beam(double lx, double ly, double ltheta, double ldist) {

#define NE     0
#define SE     1
#define SW     2
#define NW     3
#define EAST   4
#define WEST   5
#define NORTH  6
#define SOUTH  7
  
  int xmap, ymap, xmap2, ymap2, wdir, ldir, h;
  double x1, y1, x2, y2, l, r, res;
  double corner_theta;
  double cos_ltheta, sin_ltheta;

  xmap2 = ymap2 = wdir = ldir = 0;
  x2 = y2 = r = 0.0;

  if (ltheta >= 0.0 && ltheta < M_PI/2.0)
    ldir = NE;
  else if (ltheta >= M_PI/2.0 && ltheta <= M_PI)
    ldir = NW;
  else if (ltheta >= -M_PI/2.0 && ltheta < 0.0)
    ldir = SE;
  else
    ldir = SW;

  cos_ltheta = cos(ltheta);
  sin_ltheta = sin(ltheta);

  res = static_map.config.resolution;

  xmap = (int)(lx/res);
  ymap = (int)(ly/res);

  //printf("cell = (%d, %d)\n", xmap, ymap);

  x1 = lx - xmap*res;
  y1 = ly - ymap*res;

  l = 0.0;
  while (l < ldist) {
    // get next wall
    switch(ldir) {
    case NE:
      corner_theta = atan2(res - y1, res - x1);
      wdir = (ltheta < corner_theta ? EAST : NORTH);
      break;
    case NW:
      corner_theta = atan2(res - y1, -x1);
      wdir = (ltheta > corner_theta ? WEST : NORTH);
      break;
    case SE:
      corner_theta = atan2(-y1, res - x1);
      wdir = (ltheta > corner_theta ? EAST : SOUTH);
      break;
    case SW:
      corner_theta = atan2(-y1, -x1);
      wdir = (ltheta < corner_theta ? WEST : SOUTH);
    }
    // get r, x2, y2, xmap2, ymap2
    switch(wdir) {
    case WEST:
      //printf("WEST\n");
      r = -x1/cos_ltheta;
      x2 = res;
      y2 = y1 + r*sin_ltheta;
      xmap2 = xmap-1;
      ymap2 = ymap;
      break;
    case SOUTH:
      //printf("SOUTH\n");
      r = -y1/sin_ltheta;
      x2 = x1 + r*cos_ltheta;
      y2 = res;
      xmap2 = xmap;
      ymap2 = ymap-1;
      break;
    case EAST:
      //printf("EAST\n");
      r = (res-x1)/cos_ltheta;
      y2 = y1 + r*sin_ltheta;
      x2 = 0.0;
      xmap2 = xmap+1;
      ymap2 = ymap;
      break;
    case NORTH:
      //printf("NORTH\n");
      r = (res-y1)/sin_ltheta;
      x2 = x1 + r*cos_ltheta;
      y2 = 0.0;
      xmap2 = xmap;
      ymap2 = ymap+1;
    }

    //printf("r = %f\n", r);
    
    h = 0;
    if (l + r > ldist) {  // laser ray stops in current cell
      r = ldist - l;
      h = 1;
    }
    update_egrid_cell(xmap, ymap, h);

    xmap = xmap2;
    ymap = ymap2;
    x1 = x2;
    y1 = y2;
    l += r;
  }
}

static void update_evidence_grid(carmen_shape_p contour, int *laser_mask, double resolution, double laser_stdev) {

  double xmin, xmax, ymin, ymax;
  double dx, dy, dmax;
  double *ux, *uy;
  double **mask;
  int i, j, x, y, x0, x1, y0, y1;
  int new_x_size, new_y_size;
  int mask_x_size, mask_y_size, mask_ux, mask_uy;
  int mask_x0, mask_x1, mask_y0, mask_y1;
  int x_offset, y_offset;
  double ltheta;

  printf("update_evidence_grid()\n");

  ux = contour->x;
  uy = contour->y;

  // find bounds for contour's evidence grid
  xmin = xmax = ux[0];
  ymin = ymax = uy[0];
  for (i = 1; i < contour->num_points; i++) {
    if (ux[i] > xmax)
      xmax = ux[i];
    else if (ux[i] < xmin)
      xmin = ux[i];
    if (uy[i] > ymax)
      ymax = uy[i];
    else if (uy[i] < ymin)
      ymin = uy[i];
  }

  // give a buffer for the egrid
  xmin -= 2 * EGRID_MASK_STDEV_MULT * laser_stdev;
  ymin -= 2 * EGRID_MASK_STDEV_MULT * laser_stdev;
  xmax += 2 * EGRID_MASK_STDEV_MULT * laser_stdev;
  ymax += 2 * EGRID_MASK_STDEV_MULT * laser_stdev;

  new_x_size = floor((xmax-xmin) / resolution);
  new_y_size = floor((ymax-ymin) / resolution);

  //egrid_x_offset = -xmin;
  //egrid_y_offset = -ymin;

  if (egrid == NULL) {  // new egrid
    egrid = new_matrix(egrid_x_size, egrid_y_size);
    egrid_counts = new_imatrix(egrid_x_size, egrid_y_size);
    egrid_hits = new_imatrix(egrid_x_size, egrid_y_size);
    for (i = 0; i < egrid_x_size; i++) {
      for (j = 0; j < egrid_y_size; j++) {
	egrid_counts[i][j] = egrid_hits[i][j] = 0;
	egrid[i][j] = -1.0;
      }
    }
    egrid_x_size = new_x_size;
    egrid_y_size = new_y_size;
    egrid_x_offset = -xmin;
    egrid_y_offset = -ymin;
  }
  else {  // resize
    x0 = floor((xmin - egrid_x_offset)/resolution);
    x1 = floor((xmax - egrid_x_offset)/resolution) + 1;
    y0 = floor((ymin - egrid_y_offset)/resolution);
    y1 = floor((ymax - egrid_y_offset)/resolution) + 1;
    if (x0 < 0 || x1 > egrid_x_size || y0 < 0 || y1 > egrid_y_size) {
      x0 = MAX(-x0, 0);
      x1 = MAX(x1, egrid_x_size);
      y0 = MAX(-y0, 0);
      y1 = MAX(y1, egrid_y_size);
      egrid = matrix_resize(egrid, x0, y0, egrid_x_size, egrid_y_size, x0+x1, y0+y1, -1.0);
      egrid_counts = imatrix_resize(egrid_counts, x0, y0, egrid_x_size, egrid_y_size, x0+x1, y0+y1, 0);
      egrid_hits = imatrix_resize(egrid_hits, x0, y0, egrid_x_size, egrid_y_size, x0+x1, y0+y1, 0);
      egrid_x_size = x0 + x1;
      egrid_y_size = y0 + y1;
      egrid_x_offset -= x0*resolution;
      egrid_y_offset -= y0*resolution;
    }
  }

  // trace laser
  for (i = 0; i < laser_msg.num_readings; i++) {
    ltheta = carmen_normalize_theta(laser->theta + (i-90)*M_PI/180.0);
    trace_laser_beam(laser->x, laser->y, ltheta, laser->range[i], laser_mask[i]);
  }

  /***************************** dbug **********************************

  for (i = 0; i < egrid_x_size; i++)
    for (j = 0; j < egrid_y_size; j++)
      egrid[i][j] = LOW_LOG_PROB;

  // create a log prob mask of odd dimensions (so there will be a unique center cell)
  mask_x_size = floor(2 * EGRID_MASK_STDEV_MULT * laser_stdev / resolution);
  if (mask_x_size % 2 == 0)
    mask_x_size++;
  mask_y_size = floor(2 * EGRID_MASK_STDEV_MULT * laser_stdev / resolution);
  if (mask_y_size % 2 == 0)
    mask_y_size++;

  mask_ux = floor(mask_x_size / 2.0);
  mask_uy = floor(mask_y_size / 2.0);

  //printf("mask_size = (%d %d)\n", mask_x_size, mask_y_size);

  mask = new_matrix(mask_x_size, mask_y_size);

  dmax = mask_x_size*mask_x_size*resolution*resolution/4.0;
  for (i = 0; i < mask_x_size; i++) {
    dx = (i - mask_ux) * resolution;
    for (j = 0; j < mask_y_size; j++) {
      dy = (j - mask_uy) * resolution;
      if (dx*dx+dy*dy <= dmax)  // circular mask
	mask[i][j] = -(dx*dx+dy*dy) / (2 * laser_stdev * laser_stdev);
      else
	mask[i][j] = LOW_LOG_PROB;
    }
  }

  for (i = 0; i < contour->num_points; i++) {
    x = floor((ux[i] - xmin) / resolution);
    y = floor((uy[i] - ymin) / resolution);
    if (x < 0 || x >= egrid_x_size || y < 0 || y >= egrid_y_size) {
      carmen_warn("Warning: contour point out of evidence grid bounds in get_evidence_grid()\n");
      continue;
    }
    mask_x0 = MAX(0, x + (mask_ux+1) - mask_x_size);
    mask_x1 = MIN(egrid_x_size - 1, x - mask_ux + mask_x_size);
    mask_y0 = MAX(0, y + (mask_uy+1) - mask_y_size);
    mask_y1 = MIN(egrid_y_size - 1, y - mask_uy + mask_y_size);
    x_offset = mask_ux - x;
    y_offset = mask_uy - y;
    for (x = mask_x0; x < mask_x1; x++)
      for (y = mask_y0; y < mask_y1; y++)
	if (egrid[x][y] < mask[x+x_offset][y+y_offset])
	  egrid[x][y] = mask[x+x_offset][y+y_offset];
  }

  free(mask);

  //*egrid_x_offset = -xmin;
  //*egrid_y_offset = -ymin;
  //*egrid_x_size_ptr = egrid_x_size;
  //*egrid_y_size_ptr = egrid_y_size;

  ***************************** end dbug *****************************/

  return egrid;
}

int scan_match(carmen_shape_p new_contour, int *laser_mask, carmen_shape_p mean_contour,
	       double resolution, double laser_stdev,
	       carmen_point_p odom_delta __attribute__ ((unused)), carmen_point_p match_delta) {

  double *new_x, *new_y, *new_x2, *new_y2;
  double x_centroid, y_centroid, x, y, best_prob, log_prob;
  double best_dx, best_dy, best_dt, dx, dy, dt, xmin, xmax, ymin, ymax;
  double dx0, dx1, dy0, dy1, dt0, dt1;
  double x_step_size, y_step_size, theta_step_size;
  int i;
  double cos_dt, sin_dt;
  static int first = 1;

  printf("scan_match()\n");

  resolution = egrid_resolution;  //dbug

  if (mean_contour->num_points > 0) {
    new_x = new_contour->x;
    new_y = new_contour->y;
    x_centroid = mean(new_x, new_contour->num_points);
    y_centroid = mean(new_y, new_contour->num_points);
    
    // get initial log likelihood
    log_prob = 0.0;
    for (i = 0; i < new_contour->num_points; i++) {
      x = floor((new_x[i] + egrid_x_offset) / resolution);
      y = floor((new_y[i] + egrid_y_offset) / resolution);
      if (x < 0 || x >= egrid_x_size || y < 0 || y >= egrid_y_size)
	log_prob += LOW_LOG_PROB;
      else
	log_prob += MAX(log(egrid[(int)x][(int)y]), LOW_LOG_PROB);
    }
    
    best_prob = log_prob;
    best_dx = best_dy = best_dt = 0.0;
    
    // compute new_contour bounds
    xmin = xmax = new_x[0];
    ymin = ymax = new_y[0];
    for (i = 1; i < new_contour->num_points; i++) {
      if (new_x[i] > xmax)
	xmax = new_x[i];
      else if (new_x[i] < xmin)
	xmin = new_x[i];
      if (new_y[i] > ymax)
	ymax = new_y[i];
      else if (new_y[i] < ymin)
	ymin = new_y[i];
    }
    
    dx0 = -(xmax + egrid_x_offset);
    dx1 = egrid_x_size*resolution - (xmin + egrid_x_offset);
    dy0 = -(ymax + egrid_y_offset);
    dy1 = egrid_y_size*resolution - (ymin + egrid_y_offset);
    
    //dbug
    dx0 = scan_match_dx0;
    dx1 = scan_match_dx1;
    dy0 = scan_match_dy0;
    dy1 = scan_match_dy1;
    
    //x_step_size = y_step_size = ceil(laser_stdev/resolution);
    x_step_size = scan_match_x_step_size;
    y_step_size = scan_match_y_step_size;
    
    //theta_step_size = M_PI/64.0;
    theta_step_size = carmen_degrees_to_radians(scan_match_theta_step_size);
    
    dt0 = -M_PI;
    dt1 = M_PI;
    
    //dbug
    dt0 = carmen_degrees_to_radians(scan_match_dt0);
    dt1 = carmen_degrees_to_radians(scan_match_dt1);
    
    new_x2 = new_vector(new_contour->num_points);
    new_y2 = new_vector(new_contour->num_points);
    
    if (!first) {
      for (i = 0; i < new_contour->num_points; i++) {
	new_x[i] = new_x[i] + scan_match_delta.x;
	new_y[i] = new_y[i] + scan_match_delta.y;
      }
      dt0 += scan_match_delta.theta;
      dt1 += scan_match_delta.theta;
    }
    else
      first = 0;
    
    // iterate through (dx,dy,dt)'s to find match_delta
    for (dt = dt0; dt < dt1; dt += theta_step_size) {
      cos_dt = cos(dt);
      sin_dt = sin(dt);
      for (i = 0; i < new_contour->num_points; i++) {
	x = new_x[i] - x_centroid;
	y = new_y[i] - y_centroid;
	new_x2[i] = cos_dt*x - sin_dt*y + x_centroid + dx0;
	new_y2[i] = sin_dt*x + cos_dt*y + y_centroid;
      }
      for (dx = dx0; dx < dx1; dx += x_step_size) {
	for (i = 0; i < new_contour->num_points; i++)
	  new_x2[i] += x_step_size;
	for (dy = dy0; dy < dy1; dy += y_step_size) {
	  log_prob = 0.0;
	  for (i = 0; i < new_contour->num_points; i++) {
	    x = new_x2[i];
	    y = new_y2[i] + dy;
	    x = floor((x + egrid_x_offset) / resolution);
	    y = floor((y + egrid_y_offset) / resolution);
	    if (x < 0 || x >= egrid_x_size || y < 0 || y >= egrid_y_size)
	      log_prob += low_log_prob;
	    else
	      log_prob += egrid[(int)x][(int)y];
	  }
	  if (log_prob > best_prob) {
	    best_prob = log_prob;
	    best_dx = dx;
	    best_dy = dy;
	    best_dt = dt;
	  }
	}
      }
    }

    // shift new_contour points
    cos_dt = cos(best_dt);
    sin_dt = sin(best_dt);
    for (i = 0; i < new_contour->num_points; i++) {
      x = new_x[i] - x_centroid;
      y = new_y[i] - y_centroid;
      new_x[i] = cos_dt*x - sin_dt*y + x_centroid + best_dx;
      new_y[i] = sin_dt*x + cos_dt*y + y_centroid + best_dy;
    }
    
    scan_match_delta.x = best_dx;
    scan_match_delta.y = best_dy;
    scan_match_delta.theta = best_dt;
    
    if (match_delta)
      *match_delta = scan_match_delta;

    printf("match_delta = (%.3f, %.3f, %.3f)\n", best_dx, best_dy, best_dt);
  }

  update_evidence_grid(new_contour, laser_mask, resolution, laser_stdev,
		       &low_log_prob);

  return 0;
}
