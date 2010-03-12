#include <carmen/carmen_graphics.h>
#include "dots_graphics.h"
#include "dots.h"
#include "dots_util.h"
#include "scanmatch.h"
#include "contour.h"


static GtkWidget *canvas;
//static GdkPixmap *pixmap = NULL, *map_pixmap = NULL;
//static int pixmap_xpos, pixmap_ypos;
//static int canvas_width = 400, canvas_height = 400;
static GdkGC *drawing_gc = NULL;
static GtkMapViewer *map_view;
static carmen_map_t localize_grid_map;


static void get_map_window() {

  carmen_world_point_t wp;
  
  wp.map = &static_map;
  if (current_filter < 0) {
    wp.pose.x = odom.globalpos.x;
    wp.pose.y = odom.globalpos.y;
  }
  else {
    wp.pose.x = filters[current_filter].x;
    wp.pose.y = filters[current_filter].y;
  }

  carmen_map_graphics_adjust_scrollbars(map_view, &wp);
}

static void draw_robot() {

  carmen_world_point_t wp;

  wp.map = &static_map;
  wp.pose.x = odom.globalpos.x;
  wp.pose.y = odom.globalpos.y;  

  carmen_map_graphics_draw_circle(map_view, &carmen_red, 1, &wp, robot_width/2.0);
  carmen_map_graphics_draw_circle(map_view, &carmen_black, 0, &wp, robot_width/2.0);
}

static void draw_laser() {

  int i;
  double x, y, theta;
  carmen_world_point_t wp;
  double pixel_size;
  
  pixel_size = carmen_fmax
    (map_view->internal_map->config.x_size/(double)map_view->port_size_x,
     map_view->internal_map->config.y_size/(double)map_view->port_size_y);
  
  pixel_size *= map_view->internal_map->config.resolution * (map_view->zoom/100.0);

  wp.map = &static_map;

  for (i = 0; i < laser_msg.num_readings; i++) {
    if (laser_msg.range[i] < laser_max_range) {
      theta = carmen_normalize_theta(laser_msg.theta + (i-90)*M_PI/180.0);
      x = laser_msg.x + cos(theta) * laser_msg.range[i];
      y = laser_msg.y + sin(theta) * laser_msg.range[i];
      wp.pose.x = x;
      wp.pose.y = y;
      if (laser_mask[i])
	carmen_map_graphics_draw_circle(map_view, &carmen_red, 1, &wp, 3*pixel_size);
      else	
	carmen_map_graphics_draw_circle(map_view, &carmen_yellow, 1, &wp, 3*pixel_size);
    }
  }
}

static void draw_polygon(double *xpoints, double *ypoints, int num_points) {

  carmen_world_point_t wp1, wp2;
  int i;

  wp1.map = wp2.map = &static_map;

  for (i = 0; i < num_points; i++) {
    wp1.pose.x = xpoints[i];
    wp1.pose.y = ypoints[i];
    wp2.pose.x = xpoints[(i+1)%num_points];
    wp2.pose.y = ypoints[(i+1)%num_points];
    carmen_map_graphics_draw_line(map_view, &carmen_blue, &wp1, &wp2);
  }
}

static void draw_egrid() {

  int i, j;
  double p;
  carmen_world_point_t wp1, wp2;
  GdkColor color;

  if (!egrid)
    return;
  
  wp1.map = wp2.map = &static_map;

  printf("draw_egrid()\n");

  printf("egrid_x_offset = %.2f\n", egrid_x_offset);
  printf("egrid_y_offset = %.2f\n", egrid_y_offset);

  for (i = 0; i < egrid_x_size; i++) {
    wp1.pose.x = -egrid_x_offset + i*egrid_resolution;
    wp2.pose.x = -egrid_x_offset + (i+1)*egrid_resolution;
    for (j = 0; j < egrid_y_size; j++) {
      p = exp(egrid[i][j]);
      printf(" %.2f", p);
      color = carmen_graphics_add_color_rgb((int)(256*(1.0-p)), (int)(256*(1.0-p)), (int)(256*(1.0-p)));
      wp1.pose.y = -egrid_y_offset + j*egrid_resolution;
      wp2.pose.y = -egrid_y_offset + (j+1)*egrid_resolution;
      carmen_map_graphics_draw_rectangle(map_view, &color, 1, &wp1, &wp2);
    }
  }
  
  printf("\n");
}

static void draw_scan_match() {

  int i, n;
  double *x, *y;
  carmen_world_point_t wp;
  double pixel_size;
  
  pixel_size = carmen_fmax
    (map_view->internal_map->config.x_size/(double)map_view->port_size_x,
     map_view->internal_map->config.y_size/(double)map_view->port_size_y);
  
  pixel_size *= map_view->internal_map->config.resolution * (map_view->zoom/100.0);

  wp.map = &static_map;

  n = last_partial_contour.num_points;
  x = last_partial_contour.x;
  y = last_partial_contour.y;

  for (i = 0; i < n; i++) {
    wp.pose.x = x[i];
    wp.pose.y = y[i];
    carmen_map_graphics_draw_circle(map_view, &carmen_orange, 1, &wp, 3*pixel_size);
  }
}

static void draw_dots() {

  int i;
  double ux, uy, vx, vy, vxy;
  carmen_world_point_t wp;

  wp.map = &static_map;

  printf("draw_dots()\n");

  for (i = 0; i < num_filters; i++) {
    ux = filters[i].x;
    uy = filters[i].y;
    vx = filters[i].vx;
    vy = filters[i].vy;
    vxy = filters[i].vxy;
    wp.pose.x = ux;
    wp.pose.y = uy;
    carmen_map_graphics_draw_ellipse(map_view, &carmen_green, &wp, vx, vxy, vy, 2);
  }

  if (current_filter >= 0) {
    if (filters[current_filter].shape.num_points > 0) {
      draw_polygon(filters[current_filter].shape.x, filters[current_filter].shape.y, filters[current_filter].shape.num_points);
    }
  }
}


#if 0
static void canvas_button_press(GtkWidget *w __attribute__ ((unused)),
				GdkEventButton *event) {

  printf("button press\n");
  
  if (event->button == 1)
    current_filter = get_closest_filter(event->x, event->y);

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
#endif


static void window_destroy(GtkWidget *w __attribute__ ((unused))) {

  gtk_main_quit();
}

void redraw() {

  get_map_window();
  carmen_map_graphics_redraw(map_view);

  printf("break 4\n");

  /*
  gdk_draw_pixmap(canvas->window,
		  canvas->style->fg_gc[GTK_WIDGET_STATE(canvas)],
		  pixmap, 0, 0, 0, 0, canvas_width, canvas_height);
  */
}

/*
static void press_handler(GtkMapViewer *the_map_view  __attribute__ ((unused)), 
			   carmen_world_point_p wp,
			   GdkEventButton *event) {

  if (event->button == 1) {
    button_pressed = 1;
  }
}
*/

int get_closest_filter(double x, double y) {

  double d, dmin;
  int i, imin;

  printf("get_closest_filter()\n");

  if (num_filters == 0)
    return -1;

  imin = 0;
  dmin = dist(filters[0].x - x, filters[0].y - y);
  for (i = 1; i < num_filters; i++) {
    d = dist(filters[i].x - x, filters[i].y - y);
    if (d < dmin) {
      dmin = d;
      imin = i;
    }
  }

  printf("  --> %d\n", imin);

  return imin;
}

static void map_button_release_handler(GtkMapViewer *the_map_view  __attribute__ ((unused)), 
				       carmen_world_point_p wp,
				       GdkEventButton *event) {

  if (event->button == 1)
    current_filter = get_closest_filter(wp->pose.x, wp->pose.y);
}

static void draw_map_graphics(GtkMapViewer *the_map_view  __attribute__ ((unused))) {

  draw_egrid();
  draw_robot();
  draw_laser();
  draw_dots();
  draw_scan_match();
}

static gint updateIPC(gpointer *data __attribute__ ((unused))) {

  sleep_ipc(0.01);
  carmen_graphics_update_ipc_callbacks((GdkInputFunction) updateIPC);

  return 1;
}

static void get_localize_grid_map() {

  int i, j, x_size, y_size;
  
  localize_grid_map = static_map;
  x_size = localize_grid_map.config.x_size;
  y_size = localize_grid_map.config.y_size;
  localize_grid_map.complete_map = (float *) calloc(x_size*y_size, sizeof(float));
  carmen_test_alloc(localize_grid_map.complete_map);
  localize_grid_map.map = (float **) calloc(x_size, sizeof(float *));
  carmen_test_alloc(localize_grid_map.map);

  for (i = 0; i < x_size; i++) {
    localize_grid_map.map[i] = localize_grid_map.complete_map + i*y_size;
    for (j = 0; j < y_size; j++)
      localize_grid_map.map[i][j] = exp(localize_map.prob[i][j]);
  }
}

void gui_init(int *argc, char **argv[]) {

  GtkWidget *window, *hbox;

  gtk_init(argc, argv);
  gdk_imlib_init();
  carmen_graphics_update_ipc_callbacks((GdkInputFunction) updateIPC);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);
  //gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, FALSE);
  gtk_signal_connect(GTK_OBJECT(window), "destroy",
		     GTK_SIGNAL_FUNC(window_destroy), NULL);

  hbox = gtk_hbox_new(TRUE, 0);

  // init map_view
  map_view = carmen_map_graphics_new_viewer(400, 400, 100.0);
  carmen_map_graphics_add_drawing_func(map_view, (drawing_func) draw_map_graphics);
  //carmen_map_graphics_add_button_press_event(map_view, (GtkSignalFunc) map_button_press_handler);
  //carmen_map_graphics_add_motion_event(map_view, (GtkSignalFunc) map_motion_handler);
  carmen_map_graphics_add_button_release_event(map_view, (GtkSignalFunc) map_button_release_handler);  
  // init canvas
  canvas = gtk_drawing_area_new();
  //gtk_drawing_area_size(GTK_DRAWING_AREA(canvas), canvas_width,
  //			canvas_height);
  //gtk_signal_connect(GTK_OBJECT(canvas), "expose_event",
  //		     GTK_SIGNAL_FUNC(canvas_expose), NULL);
  //gtk_signal_connect(GTK_OBJECT(canvas), "configure_event",
  //		     GTK_SIGNAL_FUNC(canvas_configure), NULL);
  //gtk_signal_connect(GTK_OBJECT(canvas), "button_press_event",
  //		     GTK_SIGNAL_FUNC(canvas_button_press), NULL);
  //gtk_widget_add_events(canvas, GDK_BUTTON_PRESS_MASK);

  gtk_box_pack_start(GTK_BOX(hbox), map_view->map_box, TRUE, TRUE, 0);
  //gtk_box_pack_start(GTK_BOX(hbox), canvas, TRUE, TRUE, 0);

  gtk_container_add(GTK_CONTAINER(window), hbox);

  gtk_widget_show_all(window);

  drawing_gc = NULL; //gdk_gc_new(canvas->window);
  carmen_graphics_setup_colors();

  get_localize_grid_map();
  carmen_map_graphics_add_map(map_view, &static_map, 0);
}

void gui_start() {

  gtk_main();
}
