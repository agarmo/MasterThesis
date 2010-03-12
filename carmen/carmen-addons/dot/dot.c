#ifdef NO_GRAPHICS
#include <carmen/carmen.h>
#else
#include <carmen/carmen_graphics.h>
#endif
#include "dot_messages.h"
#include "dot.h"
#include <gsl/gsl_linalg.h>


#define MI(M,i,j) ((i)*(M)->tda + (j))

#define FILTER_MAP_ADD_CLUSTER  -1
#define FILTER_MAP_NEW_CLUSTER  -2

typedef struct dot_person_filter {
  carmen_list_t *sensor_update_list;
  int hidden_cnt;
  double x0;
  double y0;
  double x;
  double y;
  double logr;  // natural log of circle radius
  gsl_matrix *P;  // estimated state covariance matrix: rows and columns in order of: x, y, logr
  double a;
  gsl_matrix *Q;  // process noise covariance matrix
  gsl_matrix *R;  // sensor noise covariance matrix
  //dbug
} carmen_dot_person_filter_t, *carmen_dot_person_filter_p;

typedef struct dot_trash_filter {
  carmen_list_t *sensor_update_list;
  double x;
  double y;
  double vx;
  double vy;
  double vxy;
  double xhull[100];  //dbug
  double yhull[100];  //dbug
  double hull_size;
} carmen_dot_trash_filter_t, *carmen_dot_trash_filter_p;

/*
typedef struct dot_door_filter {
  double x;
  double y;
  double t;  // theta
  double w;  // width
} carmen_dot_door_filter_t, *carmen_dot_door_filter_p;
*/

typedef struct dot_filter {
  carmen_dot_person_filter_t person_filter;
  carmen_dot_trash_filter_t trash_filter;
  //carmen_dot_door_filter_t door_filter;
  int id;
  int type;
  int allow_change;
  int updated;
  int invisible_cnt;
} carmen_dot_filter_t, *carmen_dot_filter_p;


static double default_person_filter_a;
static double default_person_filter_px;
static double default_person_filter_py;
static double default_person_filter_pxy;
static double default_person_filter_qx;
static double default_person_filter_qy;
static double default_person_filter_qxy;
static double default_person_filter_qlogr;
static double default_person_filter_rx;
static double default_person_filter_ry;
static double default_person_filter_rxy;

static double new_cluster_threshold;
static double map_diff_threshold;
static double map_diff_threshold_scalar;
static double map_occupied_threshold;

static int kill_hidden_person_cnt;
static double laser_max_range;
static double trash_see_through_dist;
static double trash_sensor_stdev;
static double person_sensor_stdev;
static double new_cluster_sensor_stdev;
static int invisible_cnt;
static double person_filter_displacement_threshold;
static double bic_min_variance;
static int bic_num_params;

static carmen_localize_param_t localize_params;
static carmen_localize_map_t localize_map;
static carmen_robot_laser_message laser_msg;
static carmen_localize_globalpos_message odom;
static carmen_map_t static_map;
static carmen_dot_filter_p filters = NULL;
static int num_filters = 0;
static int filter_highlight = -1;
static carmen_list_t *highlight_list;


static inline int is_in_map(int x, int y) {

  return (x >= 0 && y >= 0 && x < static_map.config.x_size &&
	  y < static_map.config.y_size);
}


/*********** GRAPHICS ************/

static int laser_mask[500];

#ifdef HAVE_GRAPHICS

static GtkWidget *canvas;
static GdkPixmap *pixmap = NULL, *map_pixmap = NULL;
static int pixmap_xpos, pixmap_ypos;
static int canvas_width = 804, canvas_height = 804;
static GdkGC *drawing_gc = NULL;

static void get_map_window() {

  carmen_map_point_t mp;
  carmen_world_point_t wp;
  
  wp.map = &static_map;
  wp.pose.x = odom.globalpos.x;
  wp.pose.y = odom.globalpos.y;

  carmen_world_to_map(&wp, &mp);

  pixmap_xpos = 4*(mp.x-100);
  pixmap_ypos = 4*(static_map.config.y_size - mp.y - 101);

  //pixmap_xpos = carmen_clamp(0, pixmap_xpos, 4*(static_map.config.x_size - 200));
  //pixmap_ypos = carmen_clamp(0, pixmap_ypos, 4*(static_map.config.y_size - 200));  
}

static void draw_map() {

  int x, y;
  double p = 0.0;
  GdkColor color;
  static int first = 1;

  if (!first)
    return;
  
  first = 0;

  for (x = 0; x < static_map.config.x_size; x++) {
    for (y = 0; y < static_map.config.y_size; y++) {
      if (is_in_map(x,y))
	p = exp(localize_map.prob[x][y]);
      else
	p = -1.0;
      if (p >= 0.0)
	color = carmen_graphics_add_color_rgb((int)(256*(1.0-p)), (int)(256*(1.0-p)), (int)(256*(1.0-p)));
      else
	color = carmen_blue;
      gdk_gc_set_foreground(drawing_gc, &color);
      gdk_draw_rectangle(map_pixmap, drawing_gc, TRUE, 4*x, 4*(static_map.config.y_size-y-1), 4, 4);
    }
  }
}

static void draw_robot() {

  gdk_gc_set_foreground(drawing_gc, &carmen_red);
  gdk_draw_arc(pixmap, drawing_gc, TRUE, canvas_width/2 - 5, canvas_height/2 - 5,
	       10, 10, 0, 360 * 64);
  gdk_gc_set_foreground(drawing_gc, &carmen_black);
  gdk_draw_arc(pixmap, drawing_gc, FALSE, canvas_width/2 - 5, canvas_height/2 - 5,
	       10, 10, 0, 360 * 64);
}

static int laser_point_belongs_to_dot(carmen_dot_filter_p f, int laser_index) {

  int i, s;

  for (i = 0; i < f->person_filter.sensor_update_list->length; i++) {
    s = *(int*)carmen_list_get(f->person_filter.sensor_update_list, i);
    if (s == laser_index)
      return 1;
  }

  return 0;
}

static void draw_laser() {

  int i, xpos, ypos;
  double x, y, theta;
  carmen_map_point_t mp;
  carmen_world_point_t wp;
  
  wp.map = &static_map;

  for (i = 0; i < laser_msg.num_readings; i++) {
    if (laser_msg.range[i] < laser_max_range) {
      theta = carmen_normalize_theta(laser_msg.theta + (i-90)*M_PI/180.0);
      x = laser_msg.x + cos(theta) * laser_msg.range[i];
      y = laser_msg.y + sin(theta) * laser_msg.range[i];
      wp.pose.x = x;
      wp.pose.y = y;
      carmen_world_to_map(&wp, &mp);
      xpos = 4*mp.x - pixmap_xpos;
      ypos = 4*(static_map.config.y_size - mp.y - 1) - pixmap_ypos;
      if (num_filters > 0 && filter_highlight >= 0 && laser_point_belongs_to_dot(&filters[filter_highlight], i))
	gdk_gc_set_foreground(drawing_gc, &carmen_blue);
      else if (laser_mask[i])
	gdk_gc_set_foreground(drawing_gc, &carmen_red);
      else
	gdk_gc_set_foreground(drawing_gc, &carmen_yellow);
      gdk_draw_arc(pixmap, drawing_gc, TRUE, xpos-2, ypos-2,
		   4, 4, 0, 360 * 64);      
    }
  }

  //dbug
  for (i = 0; i < highlight_list->length; i+=2) {
    wp.pose.x = *(int*)carmen_list_get(highlight_list, i);
    wp.pose.y = *(int*)carmen_list_get(highlight_list, i+1);
    carmen_world_to_map(&wp, &mp);
    xpos = 4*mp.x - pixmap_xpos;
    ypos = 4*(static_map.config.y_size - mp.y - 1) - pixmap_ypos;
    gdk_gc_set_foreground(drawing_gc, &carmen_purple);
    gdk_draw_arc(pixmap, drawing_gc, TRUE, xpos-2, ypos-2,
		 4, 4, 0, 360 * 64);
  }
}

static void draw_ellipse(double ux, double uy, double vx, double vxy, double vy, double k) {

#define ELLIPSE_PLOTPOINTS 30

  static GdkPoint poly[ELLIPSE_PLOTPOINTS];
  double len;
  gint i;
  double discriminant, eigval1, eigval2,
    eigvec1x, eigvec1y, eigvec2x, eigvec2y;
  //carmen_world_point_t point, e1, e2;
  carmen_graphics_screen_point_t p1;
  carmen_map_point_t mp;

  /* check for special case of axis-aligned */
  if (fabs(vxy) < (fabs(vx) + fabs(vy) + 1e-4) * 1e-4) {
    eigval1 = vx;
    eigval2 = vy;
    eigvec1x = 1.;
    eigvec1y = 0.;
    eigvec2x = 0.;
    eigvec2y = 1.;
  } else {

    /* compute axes and scales of ellipse */
    discriminant = sqrt(4*carmen_square(vxy) + 
			carmen_square(vx - vy));
    eigval1 = .5 * (vx + vy - discriminant);
    eigval2 = .5 * (vx + vy + discriminant);
    eigvec1x = (vx - vy - discriminant) / (2.*vxy);
    eigvec1y = 1.;
    eigvec2x = (vx - vy + discriminant) / (2.*vxy);
    eigvec2y = 1.;

    /* normalize eigenvectors */
    len = sqrt(carmen_square(eigvec1x) + 1.);
    eigvec1x /= len;
    eigvec1y /= len;
    len = sqrt(carmen_square(eigvec2x) + 1.);
    eigvec2x /= len;
    eigvec2y /= len;
  }

  /* take square root of eigenvalues and scale -- once this is
     done, eigvecs are unit vectors along axes and eigvals are
     corresponding radii */
  if (eigval1 < 0 || eigval2 < 0) {
    return;
  }

  eigval1 = sqrt(eigval1) * k;
  eigval2 = sqrt(eigval2) * k;
  if (eigval1 < .01) eigval1 = .01;
  if (eigval2 < .01) eigval2 = .01;

  /* compute points around edge of ellipse */
  for (i = 0; i < ELLIPSE_PLOTPOINTS; i++) {
    double theta = M_PI * (-1 + 2.*i/ELLIPSE_PLOTPOINTS);
    double xi = cos(theta) * eigval1;
    double yi = sin(theta) * eigval2;

    mp.x = (int)((xi * eigvec1x + yi * eigvec2x + ux)/(static_map.config.resolution/4.0));
    mp.y = (int)((xi * eigvec1y + yi * eigvec2y + uy)/(static_map.config.resolution/4.0));

    p1.x = mp.x - pixmap_xpos;
    p1.y = (4*static_map.config.y_size - mp.y - 1) - pixmap_ypos;

    poly[i].x = p1.x;
    poly[i].y = p1.y;
  }

  /* finally we can draw it */
  gdk_draw_polygon(pixmap, drawing_gc, FALSE,
                   poly, ELLIPSE_PLOTPOINTS);

  /*
  e1 = *mean;
  e1.pose.x = mean->pose.x + eigval1 * eigvec1x;
  e1.pose.y = mean->pose.y + eigval1 * eigvec1y;

  e2 = *mean;
  e2.pose.x = mean->pose.x - eigval1 * eigvec1x;
  e2.pose.y = mean->pose.y - eigval1 * eigvec1y;

  carmen_map_graphics_draw_line(map_view, colour, &e1, &e2);

  e1.pose.x = mean->pose.x + eigval2 * eigvec2x;
  e1.pose.y = mean->pose.y + eigval2 * eigvec2y;
  e2.pose.x = mean->pose.x - eigval2 * eigvec2x;
  e2.pose.y = mean->pose.y - eigval2 * eigvec2y;

  carmen_map_graphics_draw_line(map_view, colour, &e1, &e2);
  */
}

static void draw_polygon(double *xpoints, double *ypoints, double num_points) {

  static GdkPoint *poly = NULL;
  static int poly_size = 0;
  double x, y;
  int i;

  if (num_points > poly_size) {
    poly = (GdkPoint *)realloc(poly, num_points*sizeof(GdkPoint));
    carmen_test_alloc(poly);
  }

  for (i = 0; i < num_points; i++) {
    x = xpoints[i] / (static_map.config.resolution/4.0);
    y = ypoints[i] / (static_map.config.resolution/4.0);
    poly[i].x = x - pixmap_xpos;
    poly[i].y = (4*static_map.config.y_size - y - 1) - pixmap_ypos;
  }

  gdk_draw_polygon(pixmap, drawing_gc, FALSE, poly, num_points);
}

static void draw_dots() {

  int i;
  double ux, uy, vx, vy, vxy, r;

  for (i = 0; i < num_filters; i++) {
    if (filters[i].type == CARMEN_DOT_PERSON) {
      gdk_gc_set_foreground(drawing_gc, &carmen_orange);
      ux = filters[i].person_filter.x;
      uy = filters[i].person_filter.y;
      r = exp(filters[i].person_filter.logr);
      vx = r*r;
      vy = r*r;
      vxy = 0.0;
      draw_ellipse(ux, uy, vx, vxy, vy, 1);
    }
    else if (filters[i].type == CARMEN_DOT_TRASH) {
      gdk_gc_set_foreground(drawing_gc, &carmen_green);
      if (i == filter_highlight) {
	ux = filters[i].trash_filter.x;
	uy = filters[i].trash_filter.y;
	vx = filters[i].trash_filter.vx;
	vy = filters[i].trash_filter.vy;
	vxy = filters[i].trash_filter.vxy;
	draw_ellipse(ux, uy, vx, vxy, vy, 1);
      }
      else {
	draw_polygon(filters[i].trash_filter.xhull, filters[i].trash_filter.yhull, filters[i].trash_filter.hull_size);
      }
    }
  }
}

static void redraw() {

  if (pixmap == NULL)
    return;

  draw_map();

  gdk_gc_set_foreground(drawing_gc, &carmen_blue);
  gdk_draw_rectangle(pixmap, drawing_gc, TRUE, 0, 0, canvas_width, canvas_height);
  gdk_draw_pixmap(pixmap, drawing_gc,
		  map_pixmap, pixmap_xpos, pixmap_ypos, 0, 0, canvas_width, canvas_height);
  draw_robot();
  draw_laser();
  draw_dots();

  gdk_draw_pixmap(canvas->window,
		  canvas->style->fg_gc[GTK_WIDGET_STATE(canvas)],
		  pixmap, 0, 0, 0, 0, canvas_width, canvas_height);
}

static void canvas_button_press(GtkWidget *w __attribute__ ((unused)),
				GdkEventButton *event) {

  if (event->button == 1)
    filter_highlight = (num_filters > 0 ? (filter_highlight+1) % num_filters : -1);
  else
    filter_highlight = -1;

  redraw();
}

static void canvas_expose(GtkWidget *w __attribute__ ((unused)),
			  GdkEventExpose *event) {

  gdk_draw_pixmap(canvas->window,
		  canvas->style->fg_gc[GTK_WIDGET_STATE(canvas)],
		  pixmap, event->area.x, event->area.y,
		  event->area.x, event->area.y,
		  event->area.width, event->area.height);
}

static void canvas_configure(GtkWidget *w __attribute__ ((unused))) {

  int display = (drawing_gc != NULL);

  /*
  canvas_width = canvas->allocation.width;
  canvas_height = canvas->allocation.height;
  if (pixmap)
    gdk_pixmap_unref(pixmap);
  */

  pixmap = gdk_pixmap_new(canvas->window, canvas_width, canvas_height, -1);
  map_pixmap = gdk_pixmap_new(canvas->window, 4*static_map.config.x_size,
			      4*static_map.config.y_size, -1);

  /*
  gdk_gc_set_foreground(drawing_gc, &carmen_white);
  gdk_draw_rectangle(pixmap, canvas->style->white_gc, TRUE, 0, 0,
		     canvas_width, canvas_height);
  gdk_draw_pixmap(canvas->window,
		  canvas->style->fg_gc[GTK_WIDGET_STATE(canvas)],
		  pixmap, 0, 0, 0, 0, canvas_width, canvas_height);
  */

  if (display)
    redraw();
}

static void window_destroy(GtkWidget *w __attribute__ ((unused))) {

  gtk_main_quit();
}

static void gui_init() {

  GtkWidget *window, *vbox;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, FALSE);
  gtk_signal_connect(GTK_OBJECT(window), "destroy",
		     GTK_SIGNAL_FUNC(window_destroy), NULL);

  vbox = gtk_vbox_new(TRUE, 0);

  canvas = gtk_drawing_area_new();

  gtk_drawing_area_size(GTK_DRAWING_AREA(canvas), canvas_width,
			canvas_height);

  gtk_box_pack_start(GTK_BOX(vbox), canvas, TRUE, TRUE, 0);

  gtk_signal_connect(GTK_OBJECT(canvas), "expose_event",
		     GTK_SIGNAL_FUNC(canvas_expose), NULL);

  gtk_signal_connect(GTK_OBJECT(canvas), "configure_event",
		     GTK_SIGNAL_FUNC(canvas_configure), NULL);

  gtk_signal_connect(GTK_OBJECT(canvas), "button_press_event",
		     GTK_SIGNAL_FUNC(canvas_button_press), NULL);

  gtk_widget_add_events(canvas, GDK_BUTTON_PRESS_MASK);

  gtk_container_add(GTK_CONTAINER(window), vbox);

  gtk_widget_show_all(window);

  drawing_gc = gdk_gc_new(canvas->window);
  carmen_graphics_setup_colors();
}

static gint updateIPC(gpointer *data __attribute__ ((unused))) {

  sleep_ipc(0.01);
  carmen_graphics_update_ipc_callbacks((GdkInputFunction) updateIPC);

  return 1;
}

#endif

/**************************************************/



//static void publish_dot_msg(carmen_dot_filter_p f, int delete);
static void publish_all_dot_msgs();

/**********************
static void print_filters() {

  int i;

  for (i = 0; i < num_filters; i++) {
    switch (filters[i].type) {
    case CARMEN_DOT_PERSON:
      printf("P");
      break;
    case CARMEN_DOT_TRASH:
	printf("T");
	break;
    case CARMEN_DOT_DOOR:
      printf("D");
      break;
    }
    printf("%d", filters[i].id);
    if (i < num_filters-1)
      printf(",");
  }
  printf("\n");
}
*************************/

static void reset() {

  free(filters);
  filters = NULL;
  num_filters = 0;
}

static inline double dist(double dx, double dy) {

  return sqrt(dx*dx+dy*dy);
}

#if 0
static void rotate2d(double *x, double *y, double theta) {

  double x2, y2;

  x2 = *x;
  y2 = *y; 

  *x = x2*cos(theta) - y2*sin(theta);
  *y = x2*sin(theta) + y2*cos(theta);
}

/*
 * inverts 2d matrix [[a, b],[c, d]]
 */
static int invert2d(double *a, double *b, double *c, double *d) {

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

static double matrix_det(gsl_matrix *M) {

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
#endif

static void matrix_invert(gsl_matrix *M) {

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

static gsl_matrix *matrix_mult(gsl_matrix *A, gsl_matrix *B) {

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

#if 0
/*
 * computes the orientation of the bivariate normal with variances
 * vx, vy, and covariance vxy.  returns an angle in [-pi/2, pi/2].
 */
static double bnorm_theta(double vx, double vy, double vxy) {

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
static inline double bnorm_w1(double vx, double vy, double vxy) {

  return (vx + vy)/2.0 + sqrt(vxy*vxy + (vx-vy)*(vx-vy)/4.0);
}

/*
 * computes the minor eigenvalue of the bivariate normal with variances
 * vx, vy, and covariance vxy.
 */
static inline double bnorm_w2(double vx, double vy, double vxy) {

  return (vx + vy)/2.0 - sqrt(vxy*vxy + (vx-vy)*(vx-vy)/4.0);
}
#endif

static double bnorm_f(double x, double y, double ux, double uy,
		      double vx, double vy, double vxy) {
  
  double z, p;
  
  p = carmen_clamp(-0.999, vxy/sqrt(vx*vy), 0.999);  //dbug
  z = (x - ux)*(x - ux)/vx - 2.0*p*(x - ux)*(y - uy)/sqrt(vx*vy) + (y - uy)*(y - uy)/vy;

  return exp(-z/(2.0*(1 - p*p)))/(2.0*M_PI*sqrt(vx*vy*(1 - p*p)));
}

#if 0
static double min_masked(double *data, int *mask, int n) {

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

static double argmin_imasked(double *data, int *mask, int i, int n) {

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

static double argmin_masked(double *data, int *mask, int n) {

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
#endif

static double argmin(double *data, int n) {

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

#if 0
static double max_masked(double *data, int *mask, int n) {

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

static double argmax_imasked(double *data, int *mask, int i, int n) {

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

static double argmax_masked(double *data, int *mask, int n) {

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

static double argmax(double *data, int n) {

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
#endif

static double mean_imasked(double *data, int *mask, int i, int n) {

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

static double var_imasked(double *data, double mean, int *mask, int i, int n) {

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

static double cov_imasked(double *xdata, double ux, double *ydata, double uy, int *mask, int i, int n) {

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

// gift wrapping algorithm for finding convex hull: returns size of hull
static int compute_convex_hull(double *xpoints, double *ypoints, int num_points, double *xhull, double *yhull) {
  
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

#if 0
static int person_contains(carmen_dot_person_filter_p f, double x, double y, double stdevs) {
  
  double ux, uy, theta, r, vx, vy, vxy, e1, e2, er;

  ux = f->x;
  uy = f->y;
  r = exp(f->logr);
  vx = f->P->data[MI(f->P,0,0)];
  vy = f->P->data[MI(f->P,1,1)];
  vxy = f->P->data[MI(f->P,0,1)];
  e1 = bnorm_w1(vx, vy, vxy);
  e2 = bnorm_w2(vx, vy, vxy);
  theta = atan2(y-uy, x-ux);
  theta += bnorm_theta(vx, vy, vxy);
  er = e2*e2*e1*e1/(e2*e2*cos(theta)*cos(theta) + e1*e1*sin(theta)*sin(theta));

  return ((x-ux)*(x-ux) + (y-uy)*(y-uy) <= stdevs*stdevs*(r*r+er));
}

static int trash_contains(carmen_dot_trash_filter_p f, double x, double y, double stdevs) {
  
  double ux, uy, theta, vx, vy, vxy, e1, e2, er;

  ux = f->x;
  uy = f->y;
  vx = f->vx;
  vy = f->vy;
  vxy = f->vxy;
  e1 = bnorm_w1(vx, vy, vxy);
  e2 = bnorm_w2(vx, vy, vxy);
  theta = atan2(y-uy, x-ux);
  theta += bnorm_theta(vx, vy, vxy);
  er = e2*e2*e1*e1/(e2*e2*cos(theta)*cos(theta) + e1*e1*sin(theta)*sin(theta));

  return ((x-ux)*(x-ux) + (y-uy)*(y-uy) <= stdevs*stdevs*(er));
}

static int door_contains(carmen_dot_door_filter_p f, double x, double y, double stdevs) {

  double ux, uy, vx, vy, vxy, theta, e1, e2, dx, dy;

  ux = f->x;
  uy = f->y;
  vx = f->px;
  vy = f->py;
  vxy = f->pxy;
  theta = bnorm_theta(vx, vy, vxy);
  e1 = bnorm_w1(vx, vy, vxy);
  e2 = bnorm_w2(vx, vy, vxy);

  dx = x - ux;
  dy = y - uy;

  rotate2d(&dx, &dy, -theta);

  return (e2*e2*dx*dx + e1*e1*dy*dy <= stdevs*stdevs*e1*e1*e2*e2);
}

static int dot_contains(carmen_dot_filter_p f, double x, double y, double stdevs) {

  if (f->type == CARMEN_DOT_PERSON)
    return person_contains(&f->person_filter, x, y, stdevs);
  else if (f->type == CARMEN_DOT_TRASH)
    return trash_contains(&f->trash_filter, x, y, stdevs);
  //else
  //  return door_contains(&f->door_filter, x, y, stdevs);

  return 0;
}
#endif

static void person_filter_motion_update(carmen_dot_person_filter_p f) {

  double ax, ay;
  
  // kalman filter time update with brownian motion model
  ax = carmen_uniform_random(-1.0, 1.0) * f->a;
  ay = carmen_uniform_random(-1.0, 1.0) * f->a;

  //printf("ax = %.4f, ay = %.4f\n", ax, ay);

  f->x += ax;
  f->y += ay;
  gsl_matrix_add(f->P, f->Q);

  //gsl_matrix_fprintf(stdout, f->P, "%f");
  //printf("\n\n");
  //gsl_matrix_fprintf(stdout, f->Q, "%f");
  //printf("\n");
}

static int ray_intersect_arg(double rx, double ry, double rtheta,
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

static int ray_intersect_circle(double rx, double ry, double rtheta,
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

static int ray_intersect_convex_polygon(double rx, double ry, double rtheta,
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

/*
 * assumes expected sensor reading is on the ray
 */
static gsl_matrix *person_filter_measurement_jacobian(carmen_dot_person_filter_p f,
						      double lx, double ly, double ltheta) {

  static gsl_matrix *H = NULL;
  static int first = 1;
  double x0, y0, y1, y2, r;
  double cos_ltheta, sin_ltheta;
  double epsilon;
  double sign;

  sign = 1.0;
  epsilon = 0.00001;  //dbug: param?

  if (first) {
    first = 0;
    H = gsl_matrix_calloc(2,3);
    carmen_test_alloc(H);
  }

  r = exp(f->logr);
  cos_ltheta = cos(ltheta);
  sin_ltheta = sin(ltheta);
  x0 = sin_ltheta*(f->x - lx) - cos_ltheta*(f->y - ly);
  y0 = cos_ltheta*(f->x - lx) + sin_ltheta*(f->y - ly);

  if (r*r - x0*x0 < 0) {  // [L^C| = 0
    H->data[MI(H,0,0)] = 1.0;
    H->data[MI(H,0,1)] = 0.0;
    H->data[MI(H,1,0)] = 0.0;
    H->data[MI(H,1,1)] = 1.0;
    if (x0 > 0.0) {
      H->data[MI(H,0,2)] = -sin_ltheta*r;
      H->data[MI(H,1,2)] = cos_ltheta*r;
    }
    else {
      H->data[MI(H,0,2)] = sin_ltheta*r;
      H->data[MI(H,1,2)] = -cos_ltheta*r;
    }
  }
  else if (r*r - x0*x0 < epsilon) {  // |L^C| = 1
    H->data[MI(H,0,0)] = cos_ltheta*cos_ltheta;
    H->data[MI(H,0,1)] = sin_ltheta*cos_ltheta;
    H->data[MI(H,0,2)] = 0.0;
    H->data[MI(H,1,0)] = sin_ltheta*cos_ltheta;
    H->data[MI(H,1,1)] = sin_ltheta*sin_ltheta;
    H->data[MI(H,1,2)] = 0.0;
  }
  else {  // |L^C| = 2
    y1 = y0 + sqrt(r*r - x0*x0);
    y2 = y0 - sqrt(r*r - x0*x0);
    switch (ray_intersect_arg(0.0, 0.0, M_PI/2.0, 0.0, y1, 0.0, y2)) {
    case 0:
      H->data[MI(H,0,0)] = 0.0;
      H->data[MI(H,0,1)] = 0.0;
      H->data[MI(H,1,0)] = 0.0;
      H->data[MI(H,1,1)] = 0.0;
      H->data[MI(H,0,2)] = 0.0;
      H->data[MI(H,1,2)] = 0.0;
      //carmen_die("expected sensor reading isn't on ray! (0, 0, pi/2, 0, %f, 0, %f)", y1, y2);
      return H;
    case 1:
      sign = 1.0;
      break;
    case 2:
      sign = -1.0;
    }
    H->data[MI(H,0,0)] = cos_ltheta*cos_ltheta - sign*sin_ltheta*cos_ltheta*x0/sqrt(r*r-x0*x0);
    H->data[MI(H,0,1)] = sin_ltheta*cos_ltheta + sign*cos_ltheta*cos_ltheta*x0/sqrt(r*r-x0*x0);
    H->data[MI(H,0,2)] = sign*cos_ltheta*r*r/sqrt(r*r-x0*x0);
    H->data[MI(H,1,0)] = sin_ltheta*cos_ltheta - sign*sin_ltheta*sin_ltheta*x0/sqrt(r*r-x0*x0);
    H->data[MI(H,1,1)] = sin_ltheta*sin_ltheta + sign*sin_ltheta*cos_ltheta*x0/sqrt(r*r-x0*x0);
    H->data[MI(H,1,2)] = sign*sin_ltheta*r*r/sqrt(r*r-x0*x0);
  }

  return H;
}

static int person_filter_max_likelihood_measurement(carmen_dot_person_filter_p f, double lx, double ly,
						    double ltheta, double *hx, double *hy) {

  double x0, y0, x1, y1, y2, r;
  double cos_ltheta, sin_ltheta;
  double epsilon;

  epsilon = 0.00001;  //dbug: param?

  r = exp(f->logr);
  cos_ltheta = cos(ltheta);
  sin_ltheta = sin(ltheta);
  x0 = sin_ltheta*(f->x - lx) - cos_ltheta*(f->y - ly);
  y0 = cos_ltheta*(f->x - lx) + sin_ltheta*(f->y - ly);

  x1 = y1 = 0.0;

  if (r*r - x0*x0 < 0) {  // [L^C| = 0
    if (x0 > 0.0)
      x1 = x0 - r;
    else
      x1 = x0 + r;
    y1 = y0;
  }
  else if (r*r - x0*x0 < epsilon) {  // |L^C| = 1
    x1 = 0.0;
    y1 = y0;
  }
  else {  // |L^C| = 2
    x1 = 0.0;
    y1 = y0 + sqrt(r*r - x0*x0);
    y2 = y0 - sqrt(r*r - x0*x0);
    switch (ray_intersect_arg(0.0, 0.0, M_PI/2.0, 0.0, y1, 0.0, y2)) {
    case 0:
      carmen_warn("expected sensor reading isn't on ray! (0, 0, pi/2, 0, %f, 0, %f)", y1, y2);
      return -1;
    case 2:
      y1 = y2;
    }
  }   

  *hx = cos_ltheta*y1 + sin_ltheta*x1 + lx;
  *hy = sin_ltheta*y1 - cos_ltheta*x1 + ly;

  return 0;
}

static void person_filter_sensor_update(carmen_dot_person_filter_p f, carmen_robot_laser_message *laser) {

  static gsl_matrix *K = NULL;
  static gsl_matrix *M1 = NULL;
  static gsl_matrix *M2 = NULL;
  static gsl_matrix *H = NULL;
  static gsl_matrix *HT = NULL;
  static gsl_matrix *R2 = NULL;
  static gsl_matrix *P2 = NULL;
  static int first = 1;
  double x, y, theta, hx, hy; //, d, dmin;
  double lx, ly, ltheta;
  float *range;
  int i, s, n; //, j, imin;

  //printf("update_person_filter()\n");

  lx = laser->x;
  ly = laser->y;
  ltheta = laser->theta;
  range = laser->range;

  P2 = gsl_matrix_calloc(3,3);
  carmen_test_alloc(P2);

  if (first) {
    first = 0;
    H = gsl_matrix_alloc(2*181, 3);
    carmen_test_alloc(H);
    HT = gsl_matrix_alloc(3, 2*181);
    carmen_test_alloc(HT);

  }

  // (x,y) update
  if (f->sensor_update_list->length > 0) {
    H->size1 = 2*f->sensor_update_list->length;
    n = 0;
    for (i = 0; i < f->sensor_update_list->length; i++) {
      s = *(int*)carmen_list_get(f->sensor_update_list, i);
      theta = carmen_normalize_theta(ltheta + (s-90)*M_PI/180.0);
      M1 = person_filter_measurement_jacobian(f, lx, ly, theta);
      H->data[MI(H,2*n,0)] = M1->data[MI(M1,0,0)];
      H->data[MI(H,2*n,1)] = M1->data[MI(M1,0,1)];
      H->data[MI(H,2*n,2)] = M1->data[MI(M1,0,2)];
      H->data[MI(H,2*n+1,0)] = M1->data[MI(M1,1,0)];
      H->data[MI(H,2*n+1,1)] = M1->data[MI(M1,1,1)];
      H->data[MI(H,2*n+1,2)] = M1->data[MI(M1,1,2)];
      n++;
    }
    HT->size2 = H->size1;
    gsl_matrix_transpose_memcpy(HT, H);
    
    P2->data[MI(P2,0,0)] = f->P->data[MI(f->P,0,0)];
    P2->data[MI(P2,0,1)] = f->P->data[MI(f->P,0,1)];
    P2->data[MI(P2,1,0)] = f->P->data[MI(f->P,1,0)];
    P2->data[MI(P2,1,1)] = f->P->data[MI(f->P,1,1)];
    //dbug
    P2->data[MI(P2,0,2)] = f->P->data[MI(f->P,0,2)];
    P2->data[MI(P2,1,2)] = f->P->data[MI(f->P,1,2)];
    P2->data[MI(P2,2,0)] = f->P->data[MI(f->P,2,0)];
    P2->data[MI(P2,2,1)] = f->P->data[MI(f->P,2,1)];
    P2->data[MI(P2,2,2)] = f->P->data[MI(f->P,2,2)];

    M1 = matrix_mult(H, P2);
    M2 = matrix_mult(M1, HT);

    gsl_matrix_free(M1);
    R2 = gsl_matrix_calloc(2*f->sensor_update_list->length,
			   2*f->sensor_update_list->length);
    carmen_test_alloc(R2);
    for (i = 0; i < f->sensor_update_list->length; i++) {
      R2->data[MI(R2,2*i,2*i)] = f->R->data[MI(f->R,0,0)];
      R2->data[MI(R2,2*i,2*i+1)] = f->R->data[MI(f->R,0,1)];
      R2->data[MI(R2,2*i+1,2*i)] = f->R->data[MI(f->R,1,0)];
      R2->data[MI(R2,2*i+1,2*i+1)] = f->R->data[MI(f->R,1,1)];
    }
    gsl_matrix_add(M2, R2);
    matrix_invert(M2);
    M1 = matrix_mult(HT, M2);
    gsl_matrix_free(M2);
    K = matrix_mult(P2, M1);
    gsl_matrix_free(M1);
    
    M1 = gsl_matrix_calloc(2*f->sensor_update_list->length, 1);
    carmen_test_alloc(M1);
    n = 0;
    for (i = 0; i < f->sensor_update_list->length; i++) {
      s = *(int*)carmen_list_get(f->sensor_update_list, i);
      theta = carmen_normalize_theta(ltheta + (s-90)*M_PI/180.0);
      hx = x = lx + cos(theta)*range[s];
      hy = y = ly + sin(theta)*range[s];
      person_filter_max_likelihood_measurement(f, lx, ly, theta, &hx, &hy);
      M1->data[MI(M1,2*n,0)] = x - hx;
      M1->data[MI(M1,2*n+1,0)] = y - hy;
      n++;
    }
    
    M2 = matrix_mult(K, M1);
    gsl_matrix_free(M1);
    
    f->x += M2->data[MI(M2,0,0)];
    f->y += M2->data[MI(M2,1,0)];
    f->logr += M2->data[MI(M2,2,0)];
    if (exp(f->logr) < 0.2)
      f->logr = log(0.2);
    else if (exp(f->logr) > 0.3)
      f->logr = log(0.3);
    
    gsl_matrix_free(M2);
    M1 = matrix_mult(K, H);
    M2 = gsl_matrix_alloc(3,3);  //dbug
    carmen_test_alloc(M2);
    gsl_matrix_set_identity(M2);
    gsl_matrix_sub(M2, M1);
    gsl_matrix_free(M1);
    M1 = matrix_mult(M2, f->P);
    gsl_matrix_free(M2);
    gsl_matrix_free(f->P);
    f->P = M1;

    gsl_matrix_free(K);
    gsl_matrix_free(R2);
  }

  gsl_matrix_free(P2);
}
 
//dbug: also need to shrink polygons!!
static void trash_filter_sensor_update(carmen_dot_trash_filter_p f, carmen_robot_laser_message *laser) {

  int i, s, num_points;
  static double xpoints[500];
  static double ypoints[500];
  double theta;
  
  num_points = 0;
  for (i = 0; i < f->hull_size; i++) {
    xpoints[num_points] = f->xhull[i];
    ypoints[num_points] = f->yhull[i];
    num_points++;
  }

  for (i = 0; i < f->sensor_update_list->length; i++) {
    s = *(int*)carmen_list_get(f->sensor_update_list, i);
    theta = carmen_normalize_theta(laser->theta + (s-90)*M_PI/180.0);
    xpoints[num_points] = laser->x + cos(theta)*laser->range[s];
    ypoints[num_points] = laser->y + sin(theta)*laser->range[s];
    num_points++;
  }

  f->hull_size = compute_convex_hull(xpoints, ypoints, num_points, f->xhull, f->yhull);

  //printf("\nhull = ");
  //for (i = 0; i < f->hull_size; i++)
  //  printf("(%.2f, %.2f) ", f->xhull[i], f->yhull[i]);
  //printf("\n");

  f->x = mean_imasked(f->xhull, NULL, 0, f->hull_size);
  f->y = mean_imasked(f->yhull, NULL, 0, f->hull_size);
  f->vx = var_imasked(f->xhull, f->x, NULL, 0, f->hull_size);
  f->vy = var_imasked(f->yhull, f->y, NULL, 0, f->hull_size);
  f->vxy = cov_imasked(f->xhull, f->x, f->yhull, f->y, NULL, 0, f->hull_size);

  //printf("x = %.f, y = %.2f, vx = %.2f, vy = %.2f, vxy = %.2f\n", f->x, f->y, f->vx, f->vy, f->vxy);
}

#if 0
static void door_filter_motion_update(carmen_dot_door_filter_p f) {

  double ax, ay;
  
  // kalman filter time update with brownian motion model
  ax = carmen_uniform_random(-1.0, 1.0) * f->a;
  ay = carmen_uniform_random(-1.0, 1.0) * f->a;

  //printf("ax = %.4f, ay = %.4f\n", ax, ay);

  f->x += ax;
  f->y += ay;
  f->px = ax*ax*f->px + f->qx;
  f->pxy = ax*ax*f->pxy + f->qxy;
  f->py = ay*ay*f->py + f->qy;
}

static void door_filter_sensor_update(carmen_dot_door_filter_p f, double x, double y) {

  double rx, ry, rxy, rt;
  double pxx, pxy, pyx, pyy, pt;
  double mxx, mxy, myx, myy, mt;
  double kxx, kxy, kyx, kyy, kt;

  pxx = f->px;
  pxy = f->pxy;
  pyx = f->pxy;
  pyy = f->py;
  pt = f->pt; //dbug: + f->qt ?

  rx = default_door_filter_rx*cos(f->t)*cos(f->t) +
    default_door_filter_ry*sin(f->t)*sin(f->t);
  ry = default_door_filter_rx*sin(f->t)*sin(f->t) +
    default_door_filter_ry*cos(f->t)*cos(f->t);
  rxy = default_door_filter_rx*cos(f->t)*sin(f->t) -
    default_door_filter_ry*cos(f->t)*sin(f->t);
  rt = default_door_filter_rt;

  // kalman filter sensor update
  mxx = pxx + rx;
  mxy = pxy + rxy;
  myx = pyx + rxy;
  myy = pyy + ry;
  mt = pt + rt;
  if (invert2d(&mxx, &mxy, &myx, &myy) < 0) {
    carmen_die("Error: can't invert matrix M");
    return;
  }
  mt = 1.0/mt;
  kxx = pxx*mxx+pxy*myx;
  kxy = pxx*mxy+pxy*myy;
  kyx = pyx*mxx+pyy*myx;
  kyy = pyx*mxy+pyy*myy;
  kt = pt*mt;
  f->x = f->x + kxx*(x - f->x) + kxy*(y - f->y);
  f->y = f->y + kyx*(x - f->x) + kyy*(y - f->y);
  f->px = (1.0 - kxx)*pxx + (-kxy)*pyx;
  f->pxy = (1.0 - kxx)*pxy + (-kxy)*pyy;
  f->py = (-kyx)*pxy + (1.0 - kyy)*pyy;
  f->pt = (1.0 - kt)*pt;
}
#endif

static void add_new_dot_filter(int *cluster_map, int c, int n,
			       double *x, double *y) {
  int i, id, cnt;
  double ux, uy, ulogr, vx, vy, vxy, vlogr, vxlogr, vylogr, lr;
  static double xbuf[500];
  static double ybuf[500];
  carmen_dot_person_filter_p pf;
  carmen_dot_trash_filter_p tf;
  //carmen_dot_door_filter_p df;

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

  ux = uy = 0.0;
  cnt = 0;
  for (i = 0; i < n; i++)
    if (cluster_map[i] == c) {
      ux += x[i];
      uy += y[i];
      cnt++;
    }
  ux /= (double)cnt;
  uy /= (double)cnt;

  ulogr = 0.0;
  for (i = 0; i < n; i++)
    if (cluster_map[i] == c)
      ulogr += log(sqrt((x[i]-ux)*(x[i]-ux) + (y[i]-uy)*(y[i]-uy)));
  ulogr /= (double)cnt;

  vx = vy = vxy = vlogr = vxlogr = vylogr = 0.0;
  for (i = 0; i < n; i++)
    if (cluster_map[i] == c) {
      vx += (x[i]-ux)*(x[i]-ux);
      vy += (y[i]-uy)*(y[i]-uy);
      vxy += (x[i]-ux)*(y[i]-uy);
      lr = log(sqrt((x[i]-ux)*(x[i]-ux) + (y[i]-uy)*(y[i]-uy)));
      vlogr += (lr-ulogr)*(lr-ulogr);
      vxlogr += (x[i]-ux)*(lr-ulogr);
      vylogr += (y[i]-uy)*(lr-ulogr);
    }
  vx /= (double)cnt;
  vy /= (double)cnt;
  vxy /= (double)cnt;
  vlogr /= (double)cnt;
  vxlogr /= (double)cnt;
  vylogr /= (double)cnt;

  printf("(ux = %.2f, uy = %.2f)", ux, uy);

  pf = &filters[num_filters-1].person_filter;
  pf->sensor_update_list = carmen_list_create(sizeof(int), 5);
  pf->x = ux;
  pf->y = uy;
  pf->logr = ulogr;
  pf->x0 = ux;
  pf->y0 = uy;
  pf->hidden_cnt = 0;
  pf->P = gsl_matrix_calloc(3,3);
  carmen_test_alloc(pf->P);
  gsl_matrix_set(pf->P, 0, 0, 0.01); //vx);
  gsl_matrix_set(pf->P, 0, 1, 0.001); //vxy);
  gsl_matrix_set(pf->P, 0, 2, 0.001); //vxlogr);
  gsl_matrix_set(pf->P, 1, 0, 0.001); //vxy);
  gsl_matrix_set(pf->P, 1, 1, 0.01); //vy);
  gsl_matrix_set(pf->P, 1, 2, 0.001); //vylogr);
  gsl_matrix_set(pf->P, 2, 0, 0.001); //vxlogr);
  gsl_matrix_set(pf->P, 2, 1, 0.001); //vylogr);
  gsl_matrix_set(pf->P, 2, 2, 0.00001); //vlogr);
  pf->a = default_person_filter_a;
  pf->Q = gsl_matrix_calloc(3,3);
  carmen_test_alloc(pf->Q);
  gsl_matrix_set(pf->Q, 0, 0, default_person_filter_qx);
  gsl_matrix_set(pf->Q, 0, 1, default_person_filter_qxy);
  gsl_matrix_set(pf->Q, 1, 0, default_person_filter_qxy);
  gsl_matrix_set(pf->Q, 1, 1, default_person_filter_qy);
  gsl_matrix_set(pf->Q, 2, 2, default_person_filter_qlogr);
  pf->R = gsl_matrix_calloc(2,2);
  carmen_test_alloc(pf->R);
  gsl_matrix_set(pf->R, 0, 0, default_person_filter_rx);
  gsl_matrix_set(pf->R, 0, 1, default_person_filter_rxy);
  gsl_matrix_set(pf->R, 1, 0, default_person_filter_rxy);
  gsl_matrix_set(pf->R, 1, 1, default_person_filter_ry);

  /*
  for (i = 0; i < n; i++)
    if (cluster_map[i] == c) {
      //printf(" (%.2f, %.2f)", x[i], y[i]);
      person_filter_sensor_update(pf, x[i], y[i]);
    }
  //printf("\n");
  */

  tf = &filters[num_filters-1].trash_filter;
  tf->sensor_update_list = carmen_list_create(sizeof(int), 5);
  tf->x = ux;
  tf->y = uy;
  tf->vx = vx;
  tf->vy = vy;
  tf->vxy = vxy;
  //compute convex hull
  cnt = 0;
  for (i = 0; i < n; i++) {
    if (cluster_map[i] == c) {
      xbuf[cnt] = x[i];
      ybuf[cnt] = y[i];
      cnt++;
    }
  }
  tf->hull_size = compute_convex_hull(xbuf, ybuf, cnt, tf->xhull, tf->yhull);
  //printf("\nhull = ");
  //for (i = 0; i < tf->hull_size; i++)
  //  printf("(%.2f, %.2f) ", tf->xhull[i], tf->yhull[i]);
  //printf("\n");

  /*
  for (i = 0; i < n; i++)
    if (cluster_map[i] == c)
      trash_filter_sensor_update(tf, x[i], y[i]);
  */

  /*
  df = &filters[num_filters-1].door_filter;
  df->sensor_update_list = carmen_list_create(sizeof(int), 5);
  df->x = ux;
  df->y = uy;
  df->t = bnorm_theta(vx, vy, vxy);
  //    bnorm_theta(filters[num_filters-1].person_filter.px,
  //	filters[num_filters-1].person_filter.py,
  //	filters[num_filters-1].person_filter.pxy);
  df->px = vx; //default_door_filter_px;
  df->py = vy; //default_door_filter_py;
  df->pt = default_door_filter_pt;
  df->pxy = vxy; //0;  //dbug?
  df->a = default_door_filter_a;
  df->qx = default_door_filter_qx;
  df->qy = default_door_filter_qy;
  df->qxy = default_door_filter_qxy;
  df->qt = default_door_filter_qt;  
  */

  /*
  for (i = 0; i < n; i++)
    if (cluster_map[i] == c)
      door_filter_sensor_update(df, x[i], y[i]);
  */

  filters[num_filters-1].type = CARMEN_DOT_TRASH;  //dbug?
  filters[num_filters-1].allow_change = 1;
  filters[num_filters-1].updated = 1;
  filters[num_filters-1].invisible_cnt = 0;

  //print_filters();
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

#if 0
static int list_contains(carmen_list_t *list, int entry) {

  int i;

  for (i = 0; i < list->length; i++)
    if (*(int*)carmen_list_get(list, i) == entry)
      return 1;

  return 0;
}
#endif

static void update_dots(carmen_robot_laser_message *laser) {

  int i;

  for (i = 0; i < num_filters; i++) {
    if (filters[i].person_filter.sensor_update_list->length > 0) {
      filters[i].person_filter.hidden_cnt = 0;
      filters[i].updated = 1;
      person_filter_sensor_update(&filters[i].person_filter, laser);
    }
    if (filters[i].trash_filter.sensor_update_list->length > 0) {
      filters[i].updated = 1;
      trash_filter_sensor_update(&filters[i].trash_filter, laser);
    }
  }
}

#if 0
// returns 1 if point is in map
static int map_filter(double x, double y, double r) {

  carmen_world_point_t wp;
  carmen_map_point_t mp;
  double d;
  int md, i, j;

  wp.map = &static_map;
  wp.pose.x = x;
  wp.pose.y = y;

  carmen_world_to_map(&wp, &mp);

  d = (map_diff_threshold + r*map_diff_threshold_scalar)/static_map.config.resolution;
  md = carmen_round(d);
  
  for (i = mp.x-md; i <= mp.x+md; i++)
    for (j = mp.y-md; j <= mp.y+md; j++)
      if (is_in_map(i, j) && dist(i-mp.x, j-mp.y) <= d &&
	  exp(localize_map.gprob[i][j]) >= map_occupied_threshold)
	return 1;

  return 0;
}
#endif

static int delete_centroid(int centroid, int *cluster_map, int num_points, double *xcentroids,
			   double *ycentroids, int *filter_map, int num_clusters) {

  int i;

  for (i = centroid; i < num_clusters-1; i++) {
    xcentroids[i] = xcentroids[i+1];
    ycentroids[i] = ycentroids[i+1];
    filter_map[i] = filter_map[i+1];
  }
  for (i = 0; i < num_points; i++) {
    if (laser_mask[i]) {
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

/*
static double dist_to_cluster(double x, double y, double xcentroid, double ycentroid, int filter_index) {

  int i;
  double d, dmin, dx, dy;

  d = 0.0;

  if (filter_index < 0)
    d = dist(x - xcentroid, y - ycentroid);
  else if (filters[filter_index].type == CARMEN_DOT_PERSON)
    d = fabs(dist(x - filters[filter_index].person_filter.x,
		  y - filters[filter_index].person_filter.y) -
	     exp(filters[filter_index].person_filter.logr));
  else if (filters[filter_index].type == CARMEN_DOT_TRASH) {
    // approx. for dist from a point to a convex polygon
    dx = x - filters[filter_index].trash_filter.xhull[0];
    dy = y - filters[filter_index].trash_filter.yhull[0];
    dmin = dist(dx, dy);
    for (i = 1; i < filters[filter_index].trash_filter.hull_size; i++) {
      dx = x - filters[filter_index].trash_filter.xhull[i];
      dy = y - filters[filter_index].trash_filter.yhull[i];
      d = dist(dx, dy);
      if (d < dmin)
	dmin = d;
    }
    d = dmin;
  }

  return d;
}
*/

static double dot_point_likelihood(carmen_dot_filter_p f, double lx, double ly, double theta, double r) {

  int i;
  double d, dmin, x, y, dx, dy, hx, hy, vhx, vhy, vhxy, fval;
  gsl_matrix *V, *M, *H, *HT;

  d = 0.0;

  x = lx + r*cos(theta);
  y = ly + r*sin(theta);

  if (f->type == CARMEN_DOT_PERSON) {
    person_filter_max_likelihood_measurement(&f->person_filter, lx, ly, theta, &hx, &hy);
    H = person_filter_measurement_jacobian(&f->person_filter, lx, ly, theta);
    if (H == NULL)
      return 0.0;
    HT = gsl_matrix_alloc(H->size2, H->size1);
    carmen_test_alloc(HT);
    gsl_matrix_transpose_memcpy(HT, H);
    M = matrix_mult(H, f->person_filter.P);
    V = matrix_mult(M, HT);
    gsl_matrix_free(M);
    gsl_matrix_free(HT);
    vhx = V->data[MI(V,0,0)];
    vhy = V->data[MI(V,1,1)];
    vhxy = V->data[MI(V,0,1)];
    gsl_matrix_free(V);
    //if (bnorm_f(x, y, hx, hy, vhx, vhy, vhxy) > .5)
    //  printf("bnorm_f(x = %.4f, y = %.4f, hx = %.4f, hy = %.4f, vxh = %f, vhy = %f, vhxy = %f) = %.2f\n",
    //	     x, y, hx, hy, vhx, vhy, vhxy, bnorm_f(x, y, hx, hy, vhx, vhy, vhxy));
    fval = bnorm_f(x, y, hx, hy, vhx, vhy, vhxy);
    return fval*person_sensor_stdev*person_sensor_stdev*M_PI;  // approx. integral of f
  }
  else if (f->type == CARMEN_DOT_TRASH) {
    // approx. for dist from a point to a convex polygon
    dx = x - f->trash_filter.xhull[0];
    dy = y - f->trash_filter.yhull[0];
    dmin = dist(dx, dy);
    for (i = 1; i < f->trash_filter.hull_size; i++) {
      dx = x - f->trash_filter.xhull[i];
      dy = y - f->trash_filter.yhull[i];
      d = dist(dx, dy);
      if (d < dmin)
	dmin = d;
    }
    d = dmin;
    fval = exp(-d*d/(2.0*trash_sensor_stdev*trash_sensor_stdev)) / (trash_sensor_stdev*sqrt(2.0*M_PI));
    return fval*trash_sensor_stdev;
  }

  return 0.0;
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
      }
    }
  }

  return dpl;
}

// return num. clusters
static int cluster_kmeans(double *xpoints, double *ypoints, double **dpl, int *cluster_map, int num_points,
			  double *xcentroids, double *ycentroids, double *vx, double *vy, double *vxy,
			  int *filter_map, int num_clusters) {

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
	if (filter_map[j] < 0)
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
  for (i = 0; i < num_filters; i++) {
    //dbug: should check for visibility
    if (filters[i].type == CARMEN_DOT_PERSON) {
      xcentroids[i] = filters[i].person_filter.x;
      ycentroids[i] = filters[i].person_filter.y;
    }
    else { //if (filters[i].type == CARMEN_DOT_TRASH) {
      xcentroids[i] = filters[i].trash_filter.x;
      ycentroids[i] = filters[i].trash_filter.y;
    }
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
    if (f != FILTER_MAP_ADD_CLUSTER) {
      carmen_list_add(filters[f].person_filter.sensor_update_list, &i);
      carmen_list_add(filters[f].trash_filter.sensor_update_list, &i);
    }
  }

  // add new dots
  for (i = 0; i < num_clusters; i++)
    if (filter_map[i] == FILTER_MAP_ADD_CLUSTER)
      add_new_dot_filter(cluster_map, i, num_points, xpoints, ypoints);

  for (i = 0; i < num_points; i++)
    if (laser_mask[i] && cluster_map[i] < 0)
      laser_mask[i] = 0;
}

static void delete_filter(int i) {

  //printf("D%d\n", filters[i].id);

  gsl_matrix_free(filters[i].person_filter.P);
  gsl_matrix_free(filters[i].person_filter.Q);
  gsl_matrix_free(filters[i].person_filter.R);

  carmen_list_destroy(&filters[i].person_filter.sensor_update_list);
  carmen_list_destroy(&filters[i].trash_filter.sensor_update_list);

  if (i < num_filters-1)
    memmove(&filters[i], &filters[i+1], (num_filters-i-1)*sizeof(carmen_dot_filter_t));
  num_filters--;

  //print_filters();
}

/*
 * delete people filters when we haven't seen them for
 * kill_hidden_person_cnt updates.
 */
static void kill_people() {

  int i;

  for (i = 0; i < num_filters; i++)
    if (!filters[i].updated)
      if (++filters[i].person_filter.hidden_cnt >= kill_hidden_person_cnt &&
	  filters[i].type == CARMEN_DOT_PERSON)
	delete_filter(i);
}

/*
 * (1) classify dot as a person (if displacement > threshold)
 * (2) do motion update (if !hidden or classified as a person)
 */
static void filter_motion() {

  int i;
  double dx, dy;

  for (i = 0; i < num_filters; i++) {
    
    dx = filters[i].person_filter.x - filters[i].person_filter.x0;
    dy = filters[i].person_filter.y - filters[i].person_filter.y0;

    if (dist(dx, dy) >= person_filter_displacement_threshold) {
      filters[i].type = CARMEN_DOT_PERSON;
      filters[i].allow_change = 0;
    }
    
    if (filters[i].person_filter.hidden_cnt == 0 || filters[i].type == CARMEN_DOT_PERSON) {
      person_filter_motion_update(&filters[i].person_filter);
      filters[i].updated = 1;
    }
  }
}

static int filter_invisible(carmen_dot_filter_p f, carmen_robot_laser_message *laser) {

  int i;
  double x1, y1, x2, y2, theta, r;

  for (i = 0; i < laser->num_readings; i++) {
    //if (laser->range[i] > laser_max_range)
    //  continue;
    theta = carmen_normalize_theta(laser->theta + (i-90)*M_PI/180.0);
    if (f->type == CARMEN_DOT_PERSON) {
      if (ray_intersect_circle(laser->x, laser->y, theta, f->person_filter.x,
			       f->person_filter.y, exp(f->person_filter.logr),
			       &x1, &y1, &x2, &y2)) {
	r = dist(x2 - laser->x, y2 - laser->y);
	if (laser->range[i] > r)
	  return 1;
      }
    }
    else if (f->type == CARMEN_DOT_TRASH) {
      if (ray_intersect_convex_polygon(laser->x, laser->y, theta, f->trash_filter.xhull,
				       f->trash_filter.yhull, f->trash_filter.hull_size,
				       &x1, &y1, &x2, &y2)) {
	r = dist(x2 - laser->x, y2 - laser->y);
	if (r < dist(x1 - laser->x, y1 - laser->y) + trash_see_through_dist)
	  r = dist(x1 - laser->x, y1 - laser->y) + trash_see_through_dist;
	if (laser->range[i] > r)
	  return 1;
      }
    }
  }

  return 0;
}

static void laser_handler(carmen_robot_laser_message *laser) {

  int i;

  if (static_map.map == NULL || odom.timestamp == 0.0)
    return;

  carmen_localize_correct_laser(laser, &odom);

  kill_people();  // when we haven't seen them for a while
  filter_motion();

  highlight_list->length = 0;  //dbug

  for (i = 0; i < num_filters; i++) {
    filters[i].updated = 0;
    filters[i].person_filter.sensor_update_list->length = 0;
    filters[i].trash_filter.sensor_update_list->length = 0;
  }

  cluster(laser);
  update_dots(laser);

  // delete invisible filters (filters we can see through)
  for (i = 0; i < num_filters; i++) {
    if (!filters[i].updated && filter_invisible(&filters[i], laser)) {
      if (++filters[i].invisible_cnt >= invisible_cnt)
	delete_filter(i);
    }
    else
      filters[i].invisible_cnt = 0;
  }      

  publish_all_dot_msgs();

#ifdef HAVE_GRAPHICS
  get_map_window();
  redraw();
#endif
}

/*******************************************
static void publish_person_msg(carmen_dot_filter_p f, int delete) {

  static carmen_dot_person_msg msg;
  static int first = 1;
  IPC_RETURN_TYPE err;
  
  if (first) {
    strcpy(msg.host, carmen_get_tenchar_host_name());
    first = 0;
  }

  msg.person.id = f->id;
  msg.person.x = f->person_filter.x;
  msg.person.y = f->person_filter.y;
  msg.person.vx = f->person_filter.px;
  msg.person.vy = f->person_filter.py;
  msg.person.vxy = f->person_filter.pxy;
  msg.delete = delete;
  msg.timestamp = carmen_get_time_ms();

  err = IPC_publishData(CARMEN_DOT_PERSON_MSG_NAME, &msg);
  carmen_test_ipc_exit(err, "Could not publish", CARMEN_DOT_PERSON_MSG_NAME);
}

static void publish_trash_msg(carmen_dot_filter_p f, int delete) {

  static carmen_dot_trash_msg msg;
  static int first = 1;
  IPC_RETURN_TYPE err;
  
  if (first) {
    strcpy(msg.host, carmen_get_tenchar_host_name());
    first = 0;
  }

  msg.trash.id = f->id;
  msg.trash.x = f->trash_filter.x;
  msg.trash.y = f->trash_filter.y;
  msg.trash.vx = f->trash_filter.px;
  msg.trash.vy = f->trash_filter.py;
  msg.trash.vxy = f->trash_filter.pxy;
  msg.delete = delete;
  msg.timestamp = carmen_get_time_ms();

  err = IPC_publishData(CARMEN_DOT_TRASH_MSG_NAME, &msg);
  carmen_test_ipc_exit(err, "Could not publish", CARMEN_DOT_TRASH_MSG_NAME);
}

static void publish_door_msg(carmen_dot_filter_p f, int delete) {

  static carmen_dot_door_msg msg;
  static int first = 1;
  IPC_RETURN_TYPE err;
  
  if (first) {
    strcpy(msg.host, carmen_get_tenchar_host_name());
    first = 0;
  }

  msg.door.id = f->id;
  msg.door.x = f->door_filter.x;
  msg.door.y = f->door_filter.y;
  msg.door.theta = f->door_filter.t;
  msg.door.vx = f->door_filter.px;
  msg.door.vy = f->door_filter.py;
  msg.door.vxy = f->door_filter.pxy;
  msg.door.vtheta = f->door_filter.pt;
  msg.delete = delete;
  msg.timestamp = carmen_get_time_ms();

  err = IPC_publishData(CARMEN_DOT_DOOR_MSG_NAME, &msg);
  carmen_test_ipc_exit(err, "Could not publish", CARMEN_DOT_DOOR_MSG_NAME);
}

static void publish_dot_msg(carmen_dot_filter_p f, int delete) {

  switch (f->type) {
  case CARMEN_DOT_PERSON:
    //if (delete)
    //printf("publishing delete person %d msg\n", f->id);
    publish_person_msg(f, delete);
    break;
  case CARMEN_DOT_TRASH:
    //if (delete)
    //printf("publishing delete trash %d msg\n", f->id);
    publish_trash_msg(f, delete);
    break;
  case CARMEN_DOT_DOOR:
    //if (delete)
    //printf("publishing delete door %d msg\n", f->id);
    publish_door_msg(f, delete);
    break;
  }
}
***************************************************/

static void publish_all_dot_msgs() {

  static carmen_dot_all_people_msg all_people_msg;
  static carmen_dot_all_trash_msg all_trash_msg;
  //static carmen_dot_all_doors_msg all_doors_msg;
  static int first = 1;
  IPC_RETURN_TYPE err;
  int i, n;
  
  if (first) {
    all_people_msg.people = NULL;
    strcpy(all_people_msg.host, carmen_get_tenchar_host_name());
    all_trash_msg.trash = NULL;
    strcpy(all_trash_msg.host, carmen_get_tenchar_host_name());
    //all_doors_msg.doors = NULL;
    //strcpy(all_doors_msg.host, carmen_get_tenchar_host_name());
    first = 0;
  }

  for (n = i = 0; i < num_filters; i++)
    if (filters[i].type == CARMEN_DOT_PERSON)
      n++;

  all_people_msg.num_people = n;
  if (n == 0)
    all_people_msg.people = NULL;
  else {
    all_people_msg.people = (carmen_dot_person_p)
      realloc(all_people_msg.people, n*sizeof(carmen_dot_person_t));
  
    for (n = i = 0; i < num_filters; i++)
      if (filters[i].type == CARMEN_DOT_PERSON) {
	all_people_msg.people[n].id = filters[i].id;
	all_people_msg.people[n].x = filters[i].person_filter.x;
	all_people_msg.people[n].y = filters[i].person_filter.y;
	all_people_msg.people[n].r = exp(filters[i].person_filter.logr);
	all_people_msg.people[n].vx = gsl_matrix_get(filters[i].person_filter.P, 0, 0);
	all_people_msg.people[n].vy = gsl_matrix_get(filters[i].person_filter.P, 1, 1);
	all_people_msg.people[n].vxy = gsl_matrix_get(filters[i].person_filter.P, 0, 1);
	n++;
      }
  }

  all_people_msg.timestamp = carmen_get_time_ms();

  err = IPC_publishData(CARMEN_DOT_ALL_PEOPLE_MSG_NAME, &all_people_msg);
  carmen_test_ipc_exit(err, "Could not publish", CARMEN_DOT_ALL_PEOPLE_MSG_NAME);

  for (n = i = 0; i < num_filters; i++)
    if (filters[i].type == CARMEN_DOT_TRASH)
      n++;

  all_trash_msg.num_trash = n;
  if (n == 0)
    all_trash_msg.trash = NULL;
  else {
    all_trash_msg.trash = (carmen_dot_trash_p)
      realloc(all_trash_msg.trash, n*sizeof(carmen_dot_trash_t));
  
    for (n = i = 0; i < num_filters; i++)
      if (filters[i].type == CARMEN_DOT_TRASH) {
	all_trash_msg.trash[n].id = filters[i].id;
	all_trash_msg.trash[n].x = filters[i].trash_filter.x;
	all_trash_msg.trash[n].y = filters[i].trash_filter.y;
	all_trash_msg.trash[n].vx = filters[i].trash_filter.vx;
	all_trash_msg.trash[n].vy = filters[i].trash_filter.vy;
	all_trash_msg.trash[n].vxy = filters[i].trash_filter.vxy;
	all_trash_msg.trash[n].xhull = (double *)
	  calloc(filters[i].trash_filter.hull_size, sizeof(double));
	carmen_test_alloc(all_trash_msg.trash[n].xhull);
	memcpy(all_trash_msg.trash[n].xhull, filters[i].trash_filter.xhull,
	       filters[i].trash_filter.hull_size*sizeof(double));
	all_trash_msg.trash[n].yhull = (double *)
	  calloc(filters[i].trash_filter.hull_size, sizeof(double));
	carmen_test_alloc(all_trash_msg.trash[n].yhull);
	memcpy(all_trash_msg.trash[n].yhull, filters[i].trash_filter.yhull,
	       filters[i].trash_filter.hull_size*sizeof(double));
	all_trash_msg.trash[n].hull_size = filters[i].trash_filter.hull_size;
	n++;
      }
  }

  all_trash_msg.timestamp = carmen_get_time_ms();

  err = IPC_publishData(CARMEN_DOT_ALL_TRASH_MSG_NAME, &all_trash_msg);
  carmen_test_ipc_exit(err, "Could not publish", CARMEN_DOT_ALL_TRASH_MSG_NAME);

  for (i = 0; i < n; i++) {
    free(all_trash_msg.trash[i].xhull);
    free(all_trash_msg.trash[i].yhull);
  }

  /*
  for (n = i = 0; i < num_filters; i++)
    if (filters[i].type == CARMEN_DOT_DOOR)
      n++;

  all_doors_msg.num_doors = n;
  if (n == 0)
    all_doors_msg.doors = NULL;
  else {
    all_doors_msg.doors = (carmen_dot_door_p)
      realloc(all_doors_msg.doors, n*sizeof(carmen_dot_door_t));
  
    for (n = i = 0; i < num_filters; i++)
      if (filters[i].type == CARMEN_DOT_DOOR) {
	all_doors_msg.doors[n].id = filters[i].id;
	all_doors_msg.doors[n].x = filters[i].door_filter.x;
	all_doors_msg.doors[n].y = filters[i].door_filter.y;
	all_doors_msg.doors[n].vx = filters[i].door_filter.px;
	all_doors_msg.doors[n].vy = filters[i].door_filter.py;
	all_doors_msg.doors[n].vxy = filters[i].door_filter.pxy;
	n++;
      }
  }

  all_doors_msg.timestamp = carmen_get_time_ms();

  err = IPC_publishData(CARMEN_DOT_ALL_DOORS_MSG_NAME, &all_doors_msg);
  carmen_test_ipc_exit(err, "Could not publish", CARMEN_DOT_ALL_DOORS_MSG_NAME);
  */
}

static void respond_all_people_msg(MSG_INSTANCE msgRef) {

  static carmen_dot_all_people_msg msg;
  static int first = 1;
  IPC_RETURN_TYPE err;
  int i, n;

  if (first) {
    first = 0;
    msg.people = NULL;
    strcpy(msg.host, carmen_get_tenchar_host_name());
  }
  
  for (n = i = 0; i < num_filters; i++)
    if (filters[i].type == CARMEN_DOT_PERSON)
      n++;

  msg.num_people = n;
  if (n == 0)
    msg.people = NULL;
  else {
    msg.people = (carmen_dot_person_p)
		  realloc(msg.people, n*sizeof(carmen_dot_person_t));
    carmen_test_alloc(msg.people);
    for (n = i = 0; i < num_filters; i++) {
      if (filters[i].type == CARMEN_DOT_PERSON) {
	msg.people[n].id = filters[i].id;
	msg.people[n].x = filters[i].person_filter.x;
	msg.people[n].y = filters[i].person_filter.y;
	msg.people[n].vx = gsl_matrix_get(filters[i].person_filter.P, 0, 0);
	msg.people[n].vy = gsl_matrix_get(filters[i].person_filter.P, 1, 1);
	msg.people[n].vxy = gsl_matrix_get(filters[i].person_filter.P, 0, 1);
	n++;
      }
    }
  }

  msg.timestamp = carmen_get_time_ms();

  err = IPC_respondData(msgRef, CARMEN_DOT_ALL_PEOPLE_MSG_NAME, &msg);
  carmen_test_ipc(err, "Could not respond",
		  CARMEN_DOT_ALL_PEOPLE_MSG_NAME);
}

static void respond_all_trash_msg(MSG_INSTANCE msgRef) {

  static carmen_dot_all_trash_msg msg;
  static int first = 1;
  IPC_RETURN_TYPE err;
  int i, n;
  
  if (first) {
    first = 0;
    msg.trash = NULL;
    strcpy(msg.host, carmen_get_tenchar_host_name());
  }
  
  for (n = i = 0; i < num_filters; i++)
    if (filters[i].type == CARMEN_DOT_TRASH)
      n++;

  msg.num_trash = n;
  if (n == 0)
    msg.trash = NULL;

  else {
    msg.trash = (carmen_dot_trash_p)
      realloc(msg.trash, n*sizeof(carmen_dot_trash_t));
    carmen_test_alloc(msg.trash);
    for (n = i = 0; i < num_filters; i++) {
      if (filters[i].type == CARMEN_DOT_TRASH) {
	msg.trash[n].id = filters[i].id;
	msg.trash[n].x = filters[i].trash_filter.x;
	msg.trash[n].y = filters[i].trash_filter.y;
	msg.trash[n].vx = filters[i].trash_filter.vx;
	msg.trash[n].vy = filters[i].trash_filter.vy;
	msg.trash[n].vxy = filters[i].trash_filter.vxy;
	n++;
      }
    }
  }

  msg.timestamp = carmen_get_time_ms();

  err = IPC_respondData(msgRef, CARMEN_DOT_ALL_TRASH_MSG_NAME, &msg);
  carmen_test_ipc(err, "Could not respond",
		  CARMEN_DOT_ALL_TRASH_MSG_NAME);
}

#if 0
static void respond_all_doors_msg(MSG_INSTANCE msgRef) {

  static carmen_dot_all_doors_msg msg;
  static int first = 1;
  IPC_RETURN_TYPE err;
  int i, n;
  
  if (first) {
    first = 0;
    msg.doors = NULL;
    strcpy(msg.host, carmen_get_tenchar_host_name());
  }
  
  for (n = i = 0; i < num_filters; i++)
    if (filters[i].type == CARMEN_DOT_DOOR)
      n++;

  msg.num_doors = n;
  if (n == 0)
    msg.doors = NULL;

  else {
    msg.doors = (carmen_dot_door_p)
      realloc(msg.doors, n*sizeof(carmen_dot_door_t));
    carmen_test_alloc(msg.doors);
    for (n = i = 0; i < num_filters; i++) {
      if (filters[i].type == CARMEN_DOT_DOOR) {
	msg.doors[n].id = filters[i].id;
	msg.doors[n].x = filters[i].door_filter.x;
	msg.doors[n].y = filters[i].door_filter.y;
	msg.doors[n].theta = filters[i].door_filter.t;
	msg.doors[n].vx = filters[i].door_filter.px;
	msg.doors[n].vy = filters[i].door_filter.py;
	msg.doors[n].vxy = filters[i].door_filter.pxy;
	msg.doors[n].vtheta = filters[i].door_filter.pt;
	n++;
      }
    }
  }

  msg.timestamp = carmen_get_time_ms();

  err = IPC_respondData(msgRef, CARMEN_DOT_ALL_DOORS_MSG_NAME, &msg);
  carmen_test_ipc(err, "Could not respond",
		  CARMEN_DOT_ALL_DOORS_MSG_NAME);
}
#endif

static void dot_query_handler
(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
 void *clientData __attribute__ ((unused))) {

  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err;
  carmen_dot_query query;

  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &query, sizeof(carmen_dot_query));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall",
			 IPC_msgInstanceName(msgRef));

  printf("--> received query %d\n", query.type);

  switch(query.type) {
  case CARMEN_DOT_PERSON:
    respond_all_people_msg(msgRef);
    break;
  case CARMEN_DOT_TRASH:
    respond_all_trash_msg(msgRef);
    break;
    //case CARMEN_DOT_DOOR:
    //respond_all_doors_msg(msgRef);
    //break;
  }
}

static void reset_handler
(MSG_INSTANCE msgRef __attribute__ ((unused)), BYTE_ARRAY callData,
 void *clientData __attribute__ ((unused))) {

  IPC_freeByteArray(callData);

  reset();
}

static void ipc_init() {

  IPC_RETURN_TYPE err;

  err = IPC_defineMsg(CARMEN_DOT_PERSON_MSG_NAME, IPC_VARIABLE_LENGTH, 
	 	      CARMEN_DOT_PERSON_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_DOT_PERSON_MSG_NAME);

  err = IPC_defineMsg(CARMEN_DOT_TRASH_MSG_NAME, IPC_VARIABLE_LENGTH, 
	 	      CARMEN_DOT_TRASH_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_DOT_TRASH_MSG_NAME);

  /*
  err = IPC_defineMsg(CARMEN_DOT_DOOR_MSG_NAME, IPC_VARIABLE_LENGTH, 
	 	      CARMEN_DOT_DOOR_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_DOT_DOOR_MSG_NAME);
  */

  err = IPC_defineMsg(CARMEN_DOT_ALL_PEOPLE_MSG_NAME, IPC_VARIABLE_LENGTH, 
	 	      CARMEN_DOT_ALL_PEOPLE_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_DOT_ALL_PEOPLE_MSG_NAME);

  err = IPC_defineMsg(CARMEN_DOT_ALL_TRASH_MSG_NAME, IPC_VARIABLE_LENGTH, 
	 	      CARMEN_DOT_ALL_TRASH_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_DOT_ALL_TRASH_MSG_NAME);

  /*
  err = IPC_defineMsg(CARMEN_DOT_ALL_DOORS_MSG_NAME, IPC_VARIABLE_LENGTH, 
	 	      CARMEN_DOT_ALL_DOORS_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_DOT_ALL_DOORS_MSG_NAME);
  */

  err = IPC_subscribe(CARMEN_DOT_QUERY_NAME, dot_query_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subcribe", CARMEN_DOT_QUERY_NAME);
  IPC_setMsgQueueLength(CARMEN_DOT_QUERY_NAME, 100);

  err = IPC_subscribe(CARMEN_DOT_RESET_MSG_NAME, reset_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subcribe", CARMEN_DOT_RESET_MSG_NAME);
  IPC_setMsgQueueLength(CARMEN_DOT_RESET_MSG_NAME, 100);

  carmen_robot_subscribe_frontlaser_message(&laser_msg,
					    (carmen_handler_t)laser_handler,
					    CARMEN_SUBSCRIBE_LATEST);
  carmen_localize_subscribe_globalpos_message(&odom, NULL,
					      CARMEN_SUBSCRIBE_LATEST);
}

static void params_init(int argc, char *argv[]) {

  carmen_param_t param_list[] = {
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

    {"dot", "person_filter_a", CARMEN_PARAM_DOUBLE, &default_person_filter_a, 1, NULL},
    {"dot", "person_filter_px", CARMEN_PARAM_DOUBLE, &default_person_filter_px, 1, NULL},
    {"dot", "person_filter_py", CARMEN_PARAM_DOUBLE, &default_person_filter_py, 1, NULL},
    {"dot", "person_filter_pxy", CARMEN_PARAM_DOUBLE, &default_person_filter_pxy, 1, NULL},
    {"dot", "person_filter_qx", CARMEN_PARAM_DOUBLE, &default_person_filter_qx, 1, NULL},
    {"dot", "person_filter_qy", CARMEN_PARAM_DOUBLE, &default_person_filter_qy, 1, NULL},
    {"dot", "person_filter_qxy", CARMEN_PARAM_DOUBLE, &default_person_filter_qxy, 1, NULL},
    {"dot", "person_filter_qlogr", CARMEN_PARAM_DOUBLE, &default_person_filter_qlogr, 1, NULL},
    {"dot", "person_filter_rx", CARMEN_PARAM_DOUBLE, &default_person_filter_rx, 1, NULL},
    {"dot", "person_filter_ry", CARMEN_PARAM_DOUBLE, &default_person_filter_ry, 1, NULL},
    {"dot", "person_filter_rxy", CARMEN_PARAM_DOUBLE, &default_person_filter_rxy, 1, NULL},
    {"dot", "new_cluster_threshold", CARMEN_PARAM_DOUBLE, &new_cluster_threshold, 1, NULL},
    {"dot", "map_diff_threshold", CARMEN_PARAM_DOUBLE, &map_diff_threshold, 1, NULL},
    {"dot", "map_diff_threshold_scalar", CARMEN_PARAM_DOUBLE,
     &map_diff_threshold_scalar, 1, NULL},
    {"dot", "map_occupied_threshold", CARMEN_PARAM_DOUBLE, &map_occupied_threshold, 1, NULL},
    {"dot", "person_filter_displacement_threshold", CARMEN_PARAM_DOUBLE,
     &person_filter_displacement_threshold, 1, NULL},
    {"dot", "kill_hidden_person_cnt", CARMEN_PARAM_INT, &kill_hidden_person_cnt, 1, NULL},
    {"dot", "laser_max_range", CARMEN_PARAM_DOUBLE, &laser_max_range, 1, NULL},
    {"dot", "trash_sensor_stdev", CARMEN_PARAM_DOUBLE, &trash_sensor_stdev, 1, NULL},
    {"dot", "person_sensor_stdev", CARMEN_PARAM_DOUBLE, &person_sensor_stdev, 1, NULL},
    {"dot", "new_cluster_sensor_stdev", CARMEN_PARAM_DOUBLE, &new_cluster_sensor_stdev, 1, NULL},
    {"dot", "invisible_cnt", CARMEN_PARAM_INT, &invisible_cnt, 1, NULL},
    {"dot", "trash_see_through_dist", CARMEN_PARAM_DOUBLE, &trash_see_through_dist, 1, NULL},
    {"dot", "bic_min_variance", CARMEN_PARAM_DOUBLE, &bic_min_variance, 1, NULL},
    {"dot", "bic_num_params", CARMEN_PARAM_INT, &bic_num_params, 1, NULL}
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

  highlight_list = carmen_list_create(sizeof(int), 5);  //dbug

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
  gtk_init(&argc, &argv);
  gui_init();
  carmen_graphics_update_ipc_callbacks((GdkInputFunction) updateIPC);
  gtk_main();
#else
  IPC_dispatch();
#endif
  
  return 0;
}
