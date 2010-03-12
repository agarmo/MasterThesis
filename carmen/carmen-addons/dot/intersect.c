#include <carmen/carmen_graphics.h>


static int num_points = 0;
static double *xpoints = NULL;
static double *ypoints = NULL;
static int intersect = 0;

static GtkWidget *canvas;
static GdkPixmap *pixmap = NULL;
static int canvas_width = 300, canvas_height = 300;
static GdkGC *drawing_gc = NULL;


static inline double dist(double x, double y) {

  return sqrt(x*x+y*y);
}

static inline double abs(double x) {

  return (x < 0 ? -x : x);
}

static int compute_intersection() {

  double dxa, dya, dxb, dyb, dx1, dy1;
  double det, alpha, gamma;
  const double epsilon = 0.0000001;

  printf("computing intersection\n");

  dxa = xpoints[1] - xpoints[0];
  dya = ypoints[1] - ypoints[0];
  dxb = xpoints[3] - xpoints[2];
  dyb = ypoints[3] - ypoints[2];
  dx1 = xpoints[3] - xpoints[1];
  dy1 = ypoints[3] - ypoints[1];

  det = dxb*dya - dxa*dyb;

  if (abs(det) < epsilon) {
    printf("abs(det) < epsilon\n");
    return 0;
  }

  alpha = (dyb*dx1 - dxb*dy1) / det;

  if (alpha < 0 || alpha > 1) {
    printf("alpha < 0 or alpha > 1\n");
    return 0;
  }

  gamma = (dya*dx1 - dxa*dy1) / det;

  if (gamma < 0 || gamma > 1) {
    printf("gamma < 0 or gamma > 1\n");
    return 0;
  }

  return 1;
}

static void add_point(int x, int y) {

  xpoints = (double *)realloc(xpoints, (num_points+1)*sizeof(double));
  carmen_test_alloc(xpoints);
  ypoints = (double *)realloc(ypoints, (num_points+1)*sizeof(double));
  carmen_test_alloc(ypoints);
  xpoints[num_points] = (double)x;
  ypoints[num_points] = (double)y;
  num_points++;

  if (num_points == 4)
    intersect = compute_intersection();
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

  if (num_points == 4)
    intersect = compute_intersection();
}

static void draw_points() {

  int i;

  if (intersect)
    gdk_gc_set_foreground(drawing_gc, &carmen_red);
  else
    gdk_gc_set_foreground(drawing_gc, &carmen_blue);
  for (i = 0; i < num_points; i++) {
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
