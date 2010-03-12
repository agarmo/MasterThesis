#ifdef NO_GRAPHICS
#include <carmen/carmen.h>
#else
#include <carmen/carmen_graphics.h>
#endif


static carmen_robot_laser_message laser_msg;
static carmen_localize_globalpos_message odom;
static carmen_map_t static_map;

typedef struct dog_cell {
  int hits;       // hits in cell
  double length;  // total length of all laser readings through cell
} dog_cell_t, *dog_cell_p;

static dog_cell_t **dog;

static inline int is_in_map(int x, int y) {

  return (x >= 0 && y >= 0 && x < static_map.config.x_size &&
	  y < static_map.config.y_size);
}

static void dog_init() {

  int i, j, n, m;

  n = static_map.config.x_size;
  m = static_map.config.y_size;

  dog = (dog_cell_t **) malloc(n*(m+1)*sizeof(dog_cell_t));
  carmen_test_alloc(dog);
  for (i = 0; i < n; i++) {
    dog[i] = (dog_cell_p)(dog+i*m);
    for (j = 0; j < m; j++) {
      dog[i][j].hits = 
    }
  }
}

/*********** GRAPHICS ************/

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
	p = static_map.map[x][y];
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

static void redraw() {

  if (pixmap == NULL)
    return;

  draw_map();

  gdk_gc_set_foreground(drawing_gc, &carmen_blue);
  gdk_draw_rectangle(pixmap, drawing_gc, TRUE, 0, 0, canvas_width, canvas_height);
  gdk_draw_pixmap(pixmap, drawing_gc,
		  map_pixmap, pixmap_xpos, pixmap_ypos, 0, 0, canvas_width, canvas_height);
  draw_robot();

  gdk_draw_pixmap(canvas->window,
		  canvas->style->fg_gc[GTK_WIDGET_STATE(canvas)],
		  pixmap, 0, 0, 0, 0, canvas_width, canvas_height);
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

static void laser_handler(carmen_robot_laser_message *laser) {

  if (static_map.map == NULL || odom.timestamp == 0.0)
    return;

  carmen_localize_correct_laser(laser, &odom);

  // handle laser data here
  

#ifdef HAVE_GRAPHICS
  get_map_window();
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

/*
static void params_init(int argc, char *argv[]) {

  carmen_param_t param_list[] = {
    {"dot", "bic_num_params", CARMEN_PARAM_INT, &bic_num_params, 1, NULL}
  };

  carmen_param_install_params(argc, argv, param_list,
	 		      sizeof(param_list) / sizeof(param_list[0]));
}
*/

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
  dog_init();

  //params_init(argc, argv);
  ipc_init();

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
