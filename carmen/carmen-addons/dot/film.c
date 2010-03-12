#include <carmen/carmen_graphics.h>

static GtkWidget *window;
static GtkMapViewer *map_view;
static int black_and_white = 0;
static int show_particles = 0;
static carmen_navigator_status_message status;
static GtkItemFactory *item_factory;
static int is_filming = 0;
static guint filming_timeout = 0;
static double cur_max_x, cur_max_y;
static carmen_map_t map;

static carmen_localize_particle_message particles;

typedef struct {
  int length;
  int capacity;
  carmen_point_p poses;
} list_t;

static list_t *estimated_list;
static list_t *truepos_list;

static void 
handle_exit(int signo __attribute__ ((unused)))
{
  gtk_main_quit ();
  exit(0);
}

static void 
delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) 
{
  widget = widget; event = event; data = data;

  handle_exit(0);
}

static int 
key_event(GtkWidget *widget __attribute__ ((unused)), 
	  GdkEventKey *event, 
	  guint data __attribute__ ((unused))) 
{
  if (toupper(event->keyval) == 'Q' && event->state & GDK_CONTROL_MASK)
    handle_exit(0);

  return 1;
}

static void 
draw_robot_objects(GtkMapViewer *map_view)
{
  GdkColor particle_colour;
  carmen_world_point_t start, end;
  int i;

  start.map = map_view->internal_map;
  end.map = map_view->internal_map;

  gdk_gc_set_line_attributes
    (map_view->drawing_gc, 2, GDK_LINE_SOLID, 10, 10);

  for (i = 0; i < estimated_list->length-1; i++) {
    start.pose = estimated_list->poses[i];
    end.pose = estimated_list->poses[i+1];
    //    end.pose.x = estimated_list->poses[i].x + 
    // cos(estimated_list->poses[i].theta);
    //end.pose.y = estimated_list->poses[i].y + 
    //  sin(estimated_list->poses[i].theta);

    if (black_and_white)
      carmen_map_graphics_draw_line(map_view, &carmen_black, &start, &end);
    else
      carmen_map_graphics_draw_line(map_view, &carmen_red, &start, &end);
  }

  if (black_and_white) 
    carmen_map_graphics_draw_circle(map_view, &carmen_black, 1, &end, .3);
  else
    carmen_map_graphics_draw_circle(map_view, &carmen_red, 1, &end, .3);  
  carmen_map_graphics_draw_circle(map_view, &carmen_black, 0, &end, .3);

  for (i = 0; i < truepos_list->length-1; i++) {
    start.pose = truepos_list->poses[i];
    end.pose = truepos_list->poses[i+1];
    if (black_and_white) {
      gdk_gc_set_line_attributes
	(map_view->drawing_gc, 2, GDK_LINE_ON_OFF_DASH, 10, 10);
      carmen_map_graphics_draw_line(map_view, &carmen_grey, &start, &end);
      gdk_gc_set_line_attributes(map_view->drawing_gc, 2, 0, 10, 10);
    } else 
      carmen_map_graphics_draw_line(map_view, &carmen_blue, &start, &end);
  }

  gdk_gc_set_line_attributes
    (map_view->drawing_gc, 1, GDK_LINE_SOLID, 10, 10);

  if (black_and_white) 
    carmen_map_graphics_draw_circle(map_view, &carmen_grey, 1, &end, .3);
  else
    carmen_map_graphics_draw_circle(map_view, &carmen_blue, 1, &end, .3);

  carmen_map_graphics_draw_circle(map_view, &carmen_black, 0, &end, .3);

  end.pose = status.goal;
  if (black_and_white) 
    carmen_map_graphics_draw_circle(map_view, &carmen_black, 1, &end, .3);
  else
    carmen_map_graphics_draw_circle(map_view, &carmen_yellow, 1, &end, .3);

  carmen_map_graphics_draw_circle(map_view, &carmen_black, 0, &end, .3);

  if (show_particles) {
    if (black_and_white)
      particle_colour = carmen_black;
    else
      particle_colour = carmen_red;

    for (i = 0; i < particles.num_particles; i++) {
      end.pose.x = particles.particles[i].x;
      end.pose.y = particles.particles[i].y;
      carmen_map_graphics_draw_point(map_view, &particle_colour, &end);
    }
  }

}

static gint 
updateIPC(gpointer *data __attribute__ ((unused))) 
{
  sleep_ipc(0.01);
  carmen_graphics_update_ipc_callbacks((GdkInputFunction)updateIPC);
  return 1;
}

static gint
handle_data(gpointer *data __attribute__ ((unused)))
{
  sleep_ipc(0.01);
  carmen_map_graphics_redraw(map_view);

  return 1;
}

static gint
save_image(gpointer data __attribute__ ((unused)),
	   guint action __attribute__ ((unused)),
	   GtkWidget *widget  __attribute__ ((unused)))
{
  int x_size, y_size;
  int x_start, y_start;
  static int counter = 0;
  char filename[255];

  x_start = map_view->x_scroll_adj->value;
  y_start = map_view->y_scroll_adj->value;
  x_size = carmen_fmin(map_view->screen_defn.width, map_view->port_size_x);
  y_size = carmen_fmin(map_view->screen_defn.height, map_view->port_size_y);

  sprintf(filename, "%s%d.png", 
	  carmen_extract_filename(map_view->internal_map->config.map_name), 
	  counter++);

  carmen_graphics_write_pixmap_as_png(map_view->drawing_pixmap, filename, 
				      0, 0, x_size, y_size);

  return 1;
}

static gint
film_image(gpointer data)
{
  return save_image(data, 0, NULL);
}

static void 
start_filming(GtkWidget *w __attribute__ ((unused)), 
	      int arg __attribute__ ((unused)))
{
  GtkWidget *menu_item;

  if (is_filming) 
    {
      menu_item = 
	gtk_item_factory_get_widget(item_factory, 
				    "/File/Stop Filming");
      gtk_widget_hide(menu_item);
      menu_item = 
	gtk_item_factory_get_widget(item_factory, 
				    "/File/Start Filming");
      gtk_widget_show(menu_item);
      gtk_timeout_remove(filming_timeout);
      is_filming = 0;
    } 
  else
    {
      menu_item = 
	gtk_item_factory_get_widget(item_factory, 
				    "/File/Start Filming");
      gtk_widget_hide(menu_item);
      menu_item = 
	gtk_item_factory_get_widget(item_factory, 
				    "/File/Stop Filming");
      gtk_widget_show(menu_item);
      is_filming = 1;
      filming_timeout = gtk_timeout_add
	(1000, (GtkFunction)film_image, NULL);
    }
}

static void 
switch_display(GtkWidget *w __attribute__ ((unused)), 
			int arg)
		 
{
  int flags = 0;

  if (arg == 0) 
    {
      estimated_list->length = 0;
      truepos_list->length = 0;
    } 
  else if (arg == 1) 
    {
      GtkWidget *menu_item = gtk_item_factory_get_widget
	(item_factory,"/Show/Black&White");
      black_and_white = ((struct _GtkCheckMenuItem *)menu_item)->active;
      if (black_and_white) {
	flags |= CARMEN_GRAPHICS_BLACK_AND_WHITE;	
	carmen_map_graphics_modify_map
	  (map_view, map_view->internal_map->complete_map, flags);
      } else {
	flags = 0;
	carmen_map_graphics_modify_map
	  (map_view, map_view->internal_map->complete_map, flags);
      }
    } 

  else if (arg == 2) 
    {
      GtkWidget *menu_item = gtk_item_factory_get_widget
	(item_factory,"/Show/Particles");
      show_particles = ((struct _GtkCheckMenuItem *)menu_item)->active;
    } 
}

static GtkItemFactoryEntry menu_items[] = {
  { "/_File",         NULL,          NULL, 0, "<Branch>" },
  { "/File/_Screen Shot", "<control>S",  (GtkItemFactoryCallback)save_image, 
    0, NULL },
  { "/File/Start Filming", NULL, start_filming, 0, NULL },
  { "/File/Stop Filming", NULL, start_filming, 0, NULL },
  { "/File/sep1",     NULL,          NULL, 0, "<Separator>" },
  { "/File/_Quit",     "<control>Q",  gtk_main_quit, 0, NULL },
  { "/_Show",         NULL,          NULL, 0, "<Branch>" },
  { "/Show/Clear", NULL, switch_display, 0,  NULL },
  { "/Show/Black&White", NULL, switch_display, 1,  "<ToggleItem>" },
  { "/Show/Particles", NULL, switch_display, 2,  "<ToggleItem>" }
};

static void 
get_main_menu(GtkWidget *new_window, GtkWidget **menubar) 
{
  GtkAccelGroup *accel_group;
  gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

  accel_group = gtk_accel_group_new ();
  
  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", 
				       accel_group);
  gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);
  gtk_accel_group_attach (accel_group, GTK_OBJECT (new_window));
  if (menubar)
    *menubar = gtk_item_factory_get_widget (item_factory, "<main>");
}

static void initialize_graphics(int argc, char *argv[], carmen_map_p map)
{
  /* GtkWidget is the storage type for widgets */
  GtkWidget *main_box;
  GtkWidget *menubar;

  gtk_init (&argc, &argv);
  gdk_imlib_init();

  carmen_graphics_setup_colors();
  
  /* Create a new window */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_signal_connect (GTK_OBJECT (window), "destroy", 
                      GTK_SIGNAL_FUNC (gtk_main_quit), 
                     "WM destroy");

  gtk_window_set_title (GTK_WINDOW (window), "POMDP Viewer");
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
                      GTK_SIGNAL_FUNC (delete_event), NULL);
  gtk_container_set_border_width (GTK_CONTAINER (window), 0);

  main_box = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (main_box), 0); 
  gtk_container_add (GTK_CONTAINER (window), main_box);

  get_main_menu (window, &menubar);
  gtk_box_pack_start (GTK_BOX (main_box), menubar, FALSE, FALSE, 0);

  if (map->config.x_size > 500) 
    map_view = 
      carmen_map_graphics_new_viewer(map->config.x_size*2/3.0, 
                                     map->config.y_size*2/3.0, 100.0);
  else
    map_view = 
      carmen_map_graphics_new_viewer(map->config.x_size, map->config.y_size, 
                                     100.0);
  gtk_box_pack_start(GTK_BOX (main_box), map_view->map_box, TRUE, TRUE, 0);
  if (black_and_white)
    carmen_map_graphics_add_map(map_view, map, 
                                CARMEN_GRAPHICS_BLACK_AND_WHITE);
  else
    carmen_map_graphics_add_map(map_view, map, 0);
  carmen_map_graphics_add_drawing_func(map_view, 
                                       (drawing_func)draw_robot_objects);

  gtk_signal_connect (GTK_OBJECT (window), "key_press_event", 
                      (GtkSignalFunc) key_event, NULL);

  gtk_widget_show_all (window);
  gtk_widget_grab_focus(window);

  return;
}

static void check_size(list_t *list)
{
  int new_capacity;
  carmen_point_p new_mem;

  if (list->capacity == 0) {
    list->capacity = 100;
    list->poses = (carmen_point_p)
      calloc(list->capacity, sizeof(carmen_point_t));
    carmen_test_alloc(list->poses);
  }

  if (list->length == list->capacity) {
    new_capacity = list->capacity*2;
    new_mem = (carmen_point_p)
      realloc(list->poses, new_capacity*sizeof(carmen_point_t));
    carmen_test_alloc(new_mem);
    list->capacity = new_capacity;
    list->poses = new_mem;
  }
}

static void status_handler(carmen_navigator_status_message *status) 
{
  int latest;

  if (!status->autonomous)
    return;

  //  if (0) {
    check_size(estimated_list);
    latest = estimated_list->length;
    estimated_list->poses[latest].x = status->robot.x;
    estimated_list->poses[latest].y = status->robot.y;
    estimated_list->poses[latest].theta = status->robot.theta;
    estimated_list->length++;
    //  }
}

static void
simulator_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
                  void *clientData)
{
  IPC_RETURN_TYPE err = IPC_OK;
  FORMATTER_PTR formatter;
  int latest;
  carmen_simulator_truepos_message *msg;

  msg = clientData;
  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, msg, 
                           sizeof(carmen_simulator_truepos_message));
  IPC_freeByteArray(callData);

  carmen_test_ipc_return(err, "Could not unmarshall", 
                         IPC_msgInstanceName(msgRef));

  if (!status.autonomous)
    return;

  check_size(truepos_list);
  latest = truepos_list->length;
  truepos_list->poses[latest] = msg->truepose;
  truepos_list->length++;
}

void particles_handler(carmen_localize_particle_message *message)
{
  static double **distribution = NULL;
  static int size = 0;
  int x, y;
  int i;
  double max;
  int max_x, max_y;
  int latest;

  if (distribution == NULL) {
    size = 1000;
    distribution = (double **)calloc(size, sizeof(double *));
    carmen_test_alloc(distribution);
    distribution[0] = (double *)calloc(size*size, sizeof(double));
    carmen_test_alloc(distribution[0]);
    for (i = 1; i < size; i++)
      distribution[i] = distribution[0]+i*size;
  }  

  memset(distribution[0], 0, size*size*sizeof(double));
  max_x = 0;
  max_y = 0;
  max = 0;
  for (i = 0; i < message->num_particles; i++) {
    x = (message->particles[i].x - message->globalpos.x) / 
      map.config.resolution;
    y = (message->particles[i].y - message->globalpos.y) / 
      map.config.resolution;
    
    x += size/2;
    y += size/2;

    if (!(x < 0 || x >= size || y < 0 || y >= size)) {
      distribution[x][y] += 1.0/message->num_particles;
      if (distribution[x][y] > max) {
	max = distribution[x][y];
	max_x = x;
	max_y = y;
      }
    }
  }

  cur_max_x = (max_x - size/2) * map.config.resolution + message->globalpos.x;
  cur_max_y = (max_y - size/2) * map.config.resolution +  message->globalpos.y;

  check_size(estimated_list);
  latest = estimated_list->length;
  estimated_list->poses[latest].x = cur_max_x;
  estimated_list->poses[latest].y = cur_max_y;
  estimated_list->poses[latest].theta = message->globalpos.theta;
  estimated_list->length++;

}


int main(int argc, char *argv[])
{
  carmen_simulator_truepos_message simulator;

  carmen_initialize_ipc(argv[0]);
  carmen_randomize(&argc, &argv);

  carmen_map_get_gridmap(&map);

  carmen_navigator_subscribe_status_message
    (&status, (carmen_handler_t)status_handler, CARMEN_SUBSCRIBE_LATEST);
  
  carmen_localize_subscribe_particle_message
    (&particles, (carmen_handler_t)particles_handler, CARMEN_SUBSCRIBE_LATEST);

  if (IPC_isMsgDefined(CARMEN_SIMULATOR_TRUEPOS_NAME)) 
    {
      IPC_subscribe(CARMEN_SIMULATOR_TRUEPOS_NAME, 
		    simulator_handler, &simulator);
      IPC_setMsgQueueLength(CARMEN_SIMULATOR_TRUEPOS_NAME, 1);
    }

  truepos_list = (list_t *)calloc(1, sizeof(list_t));
  estimated_list = (list_t *)calloc(1, sizeof(list_t));

  initialize_graphics(argc, argv, &map);
  carmen_graphics_update_ipc_callbacks((GdkInputFunction)updateIPC);
  
  gtk_timeout_add(500, (GtkFunction)handle_data, NULL);
  gtk_main();

  return 0;
}
