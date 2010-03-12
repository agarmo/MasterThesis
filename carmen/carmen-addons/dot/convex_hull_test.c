#include <carmen/carmen_graphics.h>


static int num_points = 0;
static double *xpoints = NULL;
static double *ypoints = NULL;
static int *cluster_mask = NULL;

static GtkWidget *canvas;
static GdkPixmap *pixmap = NULL;
static int canvas_width = 300, canvas_height = 300;
static GdkGC *drawing_gc = NULL;


static inline double dist(double x, double y) {

  return sqrt(x*x+y*y);
}

static inline double abs(double x) {

  if (x < 0.0)
    return -x;
  else
    return x;
}

/*
static double mean(double *data, int n) {

  int i;
  double m;
  
  m = 0.0;
  for (i = 0; i < n; i++)
    m += data[i];
  m /= (double)n;

  return m;
}

static double stdev(double *data, int n, double mean) {

  int i;
  double v;
  
  v = 0.0;
  for (i = 0; i < n; i++)
    v += (data[i]-mean)*(data[i]-mean);
  v /= (double)n;

  return sqrt(v);
}

static inline void swap(double *a, double *b) {

  double tmp;

  tmp = *a;
  *a = *b;
  *b = tmp;
}

static inline void iswap(int *a, int *b) {

  int tmp;

  tmp = *a;
  *a = *b;
  *b = tmp;
}

static void bsort(double *d, int *di, int n) {

  int i, j;

  for (i = 0; i < n-1; i++) {
    for (j = 0; j < n-1; j++) {
      if (d[j] > d[j+1]) {
	swap(&d[j], &d[j+1]);
	if (di != NULL)
	  iswap(&di[j], &di[j+1]);
      }
    }
  }
}

static void transpose(double **d, int n) {

  int i, j;

  for (i = 0; i < n; i++)
    for (j = i+1; j < n; j++)
      swap(&d[i][j], &d[j][i]);
}

static double min(double *data, int n) {

  int i;
  double x;

  x = data[0];
  for (i = 1; i < n; i++)
    if (data[i] < x)
      x = data[i];

  return x;
}
*/

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

/*
static double max(double *data, int n) {

  int i;
  double x;

  x = data[0];
  for (i = 1; i < n; i++)
    if (data[i] > x)
      x = data[i];

  return x;
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

static void cluster_kmeans() {

#define CLUSTER_DIST 50
#define CENTROID_MOVED_DIST 0

  double xmin, ymin, xmax, ymax, x, y, d, dx, dy, dmin;
  static double xcentroids[400];
  static double ycentroids[400];
  int num_clusters, n, i, j, assigned, imin;
  int centroids_changed;

  cluster_mask = (int *)realloc(cluster_mask, num_points*sizeof(double));
  carmen_test_alloc(cluster_mask);
  for (i = 0; i < num_points; i++)
    cluster_mask[i] = -1;

  // step 1: initialize centroids

  xmin = min(xpoints, num_points);
  xmax = max(xpoints, num_points);
  ymin = min(ypoints, num_points);
  ymax = max(ypoints, num_points);

  num_clusters = 0;
  n = 0;
  while (n < num_points) {
    xcentroids[num_clusters] = carmen_uniform_random(xmin, xmax);
    ycentroids[num_clusters] = carmen_uniform_random(ymin, ymax);
    num_clusters++;
    assigned = 0;
    for (i = 0; i < num_points; i++) {
      dx = xpoints[i] - xcentroids[num_clusters-1];
      dy = ypoints[i] - ycentroids[num_clusters-1];
      if (cluster_mask[i] < 0 && dist(dx, dy) < CLUSTER_DIST) {
	cluster_mask[i] = num_clusters-1;
	assigned = 1;
	n++;
      }
    }
    if (!assigned)
      num_clusters--;
  }

  centroids_changed = 1;
  while (centroids_changed) {
    centroids_changed = 0;
    // step 2: assign points to closest centroids
    for (i = 0; i < num_points; i++) {
      dx = xpoints[i] - xcentroids[0];
      dy = ypoints[i] - ycentroids[0];
      dmin = dist(dx, dy);
      imin = 0;
      for (j = 0; j < num_clusters; j++) {
	dx = xpoints[i] - xcentroids[j];
	dy = ypoints[i] - ycentroids[j];
	d = dist(dx, dy);
	if (d < dmin) {
	  dmin = d;
	  imin = j;
	}
      }
      cluster_mask[i] = imin;
    }
    // step 3: re-calculate positions of centroids
    for (i = 0; i < num_clusters; i++) {
      x = y = 0.0;
      n = 0;
      for (j = 0; j < num_points; j++) {
	if (cluster_mask[j] == i) {
	  x += xpoints[j];
	  y += ypoints[j];
	  n++;
	}
      }
      if (n == 0) {  // delete centroid
	for (j = i; j < num_clusters-1; j++) {
	  xcentroids[j] = xcentroids[j+1];
	  ycentroids[j] = ycentroids[j+1];
	}
	for (j = 0; j < num_points; j++)
	  if (cluster_mask[j] > i)
	    cluster_mask[j]--;
      }
      else {
	x /= (double)n;
	y /= (double)n;
	if (!centroids_changed) {
	  dx = x - xcentroids[i];
	  dy = y - ycentroids[i];
	  if (dist(dx, dy) > CENTROID_MOVED_DIST)
	    centroids_changed = 1;
	}
	xcentroids[i] = x;
	ycentroids[i] = y;
	printf("centroid %d = (%.2f, %.2f)\n", i, xcentroids[i], ycentroids[i]);
      }
    }
  }
}
*/

// gift wrapping algorithm for finding convex hull
static void cluster_convex_hull() {
  
  int a, b, i;
  double last_angle, max_angle, theta, dx, dy;

  cluster_mask = (int *)realloc(cluster_mask, num_points*sizeof(double));
  carmen_test_alloc(cluster_mask);
  for (i = 0; i < num_points; i++)
    cluster_mask[i] = 0;

  a = argmin(ypoints, num_points);
  cluster_mask[a] = 1;
  
  last_angle = 0.0;  // 0 for clockwise, pi for counter-clockwise wrapping
  while (1) {
    max_angle = -0.0;
    b = -1;
    for (i = 0; i < num_points; i++) {
      if (i == a)
	continue;
      dx = xpoints[i] - xpoints[a];
      dy = ypoints[i] - ypoints[a];
      if (dist(dx, dy) == 0.0)
	continue;
      theta = abs(carmen_normalize_theta(atan2(dy, dx) - last_angle));
      if (theta > max_angle) {
	max_angle = theta;
	b = i;
      }
    }
    if (cluster_mask[b])
      break;
    cluster_mask[b] = 1;
    dx = xpoints[a] - xpoints[b];
    dy = ypoints[a] - ypoints[b];
    last_angle = atan2(dy, dx);
    a = b;
  }
}

static void add_point(int x, int y) {

  xpoints = (double *)realloc(xpoints, (num_points+1)*sizeof(double));
  carmen_test_alloc(xpoints);
  ypoints = (double *)realloc(ypoints, (num_points+1)*sizeof(double));
  carmen_test_alloc(ypoints);
  xpoints[num_points] = (double)x;
  ypoints[num_points] = (double)y;
  num_points++;

  //cluster();
  //cluster_kmeans();
  cluster_convex_hull();
}

static void delete_closest_point(int x, int y) {

  int i, imin;
  double d, dmin;
  
  if (num_points == 0)
    return;

  imin = 0;
  dmin = dist(xpoints[0]-(double)x, ypoints[0]-(double)y);
  for (i = 1; i < num_points; i++) {
    d = dist(xpoints[i]-(double)x, ypoints[i]-(double)y);
    if (d < dmin) {
      dmin = d;
      imin = i;
    }
  }

  for (i = imin; i < num_points-1; i++) {
    xpoints[i] = xpoints[i+1];
    ypoints[i] = ypoints[i+1];
   }

  num_points--;

  //cluster();
  //cluster_kmeans();
  cluster_convex_hull();
}

static void draw_points() {

  int i;

  for (i = 0; i < num_points; i++) {
    if (cluster_mask[i] == 1)
      gdk_gc_set_foreground(drawing_gc, &carmen_red);
    else if (cluster_mask[i] == 2)
      gdk_gc_set_foreground(drawing_gc, &carmen_blue);
    else if (cluster_mask[i] == 3)
      gdk_gc_set_foreground(drawing_gc, &carmen_green);
    else if (cluster_mask[i] == 4)
      gdk_gc_set_foreground(drawing_gc, &carmen_yellow);
    else if (cluster_mask[i] == 5)
      gdk_gc_set_foreground(drawing_gc, &carmen_grey);
    else if (cluster_mask[i] == 0)
      gdk_gc_set_foreground(drawing_gc, &carmen_purple);
    else //if (cluster_mask[i] == 0)
      gdk_gc_set_foreground(drawing_gc, &carmen_black);
    gdk_draw_arc(pixmap, drawing_gc, TRUE, (int)xpoints[i], (int)ypoints[i],
		 10, 10, 0, 360 * 64);
  }
}

static void redraw() {

  gdk_gc_set_foreground(drawing_gc, &carmen_white);
  gdk_draw_rectangle(pixmap, canvas->style->white_gc, TRUE, 0, 0,
		     canvas_width, canvas_height);
  draw_points();
  gdk_draw_pixmap(canvas->window,
		  canvas->style->fg_gc[GTK_WIDGET_STATE(canvas)],
		  pixmap, 0, 0, 0, 0, canvas_width, canvas_height);
}

static void canvas_button_press(GtkWidget *w __attribute__ ((unused)),
				GdkEventButton *event) {

  if (event->button == 1)
    add_point(event->x, event->y);
  else
    delete_closest_point(event->x, event->y);

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

  canvas_width = canvas->allocation.width;
  canvas_height = canvas->allocation.height;
  if (pixmap)
    gdk_pixmap_unref(pixmap);
  pixmap = gdk_pixmap_new(canvas->window, canvas_width,
			  canvas_height, -1);
  gdk_gc_set_foreground(drawing_gc, &carmen_white);
  gdk_draw_rectangle(pixmap, canvas->style->white_gc, TRUE, 0, 0,
		     canvas_width, canvas_height);
  /*
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
  gtk_window_set_policy(GTK_WINDOW(window), FALSE, TRUE, FALSE);
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

/***
static gint updateIPC(gpointer *data __attribute__ ((unused))) {

  sleep_ipc(0.01);
  carmen_graphics_update_ipc_callbacks((GdkInputFunction) updateIPC);

  return 1;
}
***/

int main(int argc, char *argv[]) {

  //carmen_initialize_ipc(argv[0]);
  //carmen_param_check_version(argv[0]);

  carmen_randomize(&argc, &argv);

  gtk_init(&argc, &argv);

  gui_init();

  //carmen_graphics_update_ipc_callbacks((GdkInputFunction) updateIPC);

  gtk_main();

  return 0;
}
