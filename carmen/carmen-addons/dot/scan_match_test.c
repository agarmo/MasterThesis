#include <carmen/carmen_graphics.h>
//#include <values.h>

#define NUM_POINTS 180
#define MAX_RANGE 4e2

static GtkWidget *drawing_area;
static GdkGC *drawing_gc = NULL;
static GdkPixmap *pixmap = NULL;
static int x, y;
static float *ranges = NULL;
static float *prev_ranges = NULL;
static float *angles = NULL;
static GdkPoint *laser_poly = NULL;
// static GdkPoint *corner_list = NULL;
// static int corner_list_size = 0;
static int laser_poly_size = 0;

static int points[4][2] = {{150, 300}, {150, 350}, {250, 350}, {250, 300}};
static GdkPoint polygon[4];

static int paused = 0;
static guint running_timeout = -1;

static gint update_contour();
static void simulate_laser(void);
static void redraw(void);

static carmen_point_t corrected_position, odom_position, prev_position;
static int init_x, init_y;

static double x_sigma = 3, y_sigma = 3, laser_sigma = 5;
// static double t_sigma = 0;

static void delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  widget = widget; event = event; data = data;

  gtk_main_quit();
  exit(0);
}

static int key_event(GtkWidget *widget __attribute__ ((unused)),
                     GdkEventKey *event, guint data __attribute__ ((unused)))
{
  if (toupper(event->keyval) == 'Q') {
    gtk_main_quit();
    exit(0);
  } else if (toupper(event->keyval) == 'P') {
    if (paused) {
      running_timeout = 
	gtk_timeout_add(100, (GtkFunction)update_contour, NULL);
      paused = 0;
      return 1;
    } else {
      gtk_timeout_remove(running_timeout);
      paused = 1;
    }
  } else if (toupper(event->keyval) == 'R') {
    x -= 10;
    simulate_laser();
    redraw();
  } else {
    x += 10;
    simulate_laser();
    redraw();
  }
  return 1;
}

static double compute_prob(float *ranges, float *prev_ranges, carmen_point_t odom_delta)
{
  double log_prob = 0;
  double angle1, angle2, prev_x, prev_y, new_x, new_y, dist;
  int i, j;
  int min, max;
  double min_dist;

  for (i = 0; i < NUM_POINTS; i++) {
    if (ranges[i] >= MAX_RANGE/2)
      continue;
    angle1 = -M_PI/2+M_PI/NUM_POINTS*i;
    
    new_x = odom_delta.x + cos(angle1+odom_delta.theta)*ranges[i];
    new_y = odom_delta.y + sin(angle1+odom_delta.theta)*ranges[i];
    
    min = (i < 20 ? 0 : i-20);
    max = (i >= NUM_POINTS-20 ? NUM_POINTS : i+20);
    min_dist = MAXFLOAT;
    for (j = min; j < max; j++) {
      angle2 = -M_PI/2+M_PI/NUM_POINTS*j;
      prev_x = cos(angle2)*prev_ranges[j];
      prev_y = sin(angle2)*prev_ranges[j];
      dist = hypot(new_x-prev_x, new_y-prev_y);
      if (dist < min_dist) 
	min_dist = dist;
    }
    
    if (min_dist < 10) 
      log_prob += -min_dist*min_dist/(2*laser_sigma*laser_sigma);
      // Norm term would be log(1.0/sqrt(2*M_PI)*sigma). Ignoring this.
  }
  return log_prob;
}

static carmen_point_t scan_match(float *ranges, float *prev_ranges, carmen_point_t odom_delta)
{
  carmen_point_t best_delta, new_delta;
  double best_prob, current_prob;
  int dx, dy, dt;
  double start, end;

  best_prob = compute_prob(ranges, prev_ranges, odom_delta);
  best_delta = odom_delta;

  start = carmen_get_time_ms();

  for (dx = -5; dx <= 5; dx++) {
    for (dy = -5; dy <= 5; dy++) {
      for (dt = -5; dt <= 5; dt++) {
	new_delta.x = odom_delta.x+dx/5.0*(2.0*x_sigma);
	new_delta.y = odom_delta.y+dy/5.0*(2.0*y_sigma);
	new_delta.theta = odom_delta.theta + (dt/160.0)*M_PI;
	current_prob = compute_prob(ranges, prev_ranges, new_delta);
	if (dx == -5 && dy == -5) {
	  printf("new_delta.theta = %.1f\n", carmen_radians_to_degrees(new_delta.theta));
	  printf("current_prob = %.2f\n", current_prob);
	}
	if (current_prob > best_prob) {
	  best_prob = current_prob;
	  best_delta = new_delta;
	}
      }
    }
  }

  end = carmen_get_time_ms();
  carmen_warn("Took %f\n", end-start);
  return best_delta;
}


static void check_bounds(int x1, int y1, int x2, int y2, 
			 double *x3, double *y3)
{
  double angle1, angle2;

  double x, y, w, z;
  x = x1 - *x3;
  y = y1 - *y3;
  w = x2 - *x3;
  z = y2 - *y3;

  angle1 = atan2(y, x);
  angle2 = atan2(z, w);
  
  if (fabs(angle2-angle1) < M_PI/2) {
    *x3 = 1e6;
    *y3 = 1e6;
  }
  
}

static void intersect(int x, int y, int x2, int y2, int p1x, int p1y, 
		      int p2x, int p2y, double *x3, double *y3)
{
  double a, b, c, d, e, f;

  if (x2 == x) {
    if (p2x == p1x) {
      *x3 = 1e6;
      *y3 = 1e6;
    } else {
      d = (p2y-p1y)/(double)(p2x-p1x);
      *x3 = x;
      *y3 = d * x -d*p1x+p1y;
      check_bounds(x, y, x2, y2, x3, y3);
      check_bounds(p1x, p1y, p2x, p2y, x3, y3);
    }
    return;
  }

  if (p2x == p1x) {
    *x3 = p1x;
    a = (y2-y)/(double)(x2-x);
    *y3 = a * p1x -a*x+y;
    check_bounds(x, y, x2, y2, x3, y3);
    check_bounds(p1x, p1y, p2x, p2y, x3, y3);
    return;
  }

  a = (y2-y)/(double)(x2-x);
  b = -1;
  c = -a*x + y;

  d = (p2y-p1y)/(double)(p2x-p1x);
  e = -1;
  f = -d*p1x+p1y;

  if (b*d == a*e) {
    *x3 = 1e6;
    *y3 = 1e6;
    return;
  }

  *y3 = (a*f-c*d)/(double)(b*d-a*e);
  if (a == 0)
    *x3 = (-e*(*y3)-f)/d;
  else
    *x3 = (-b*(*y3)-c)/a;

  check_bounds(x, y, x2, y2, x3, y3);
  check_bounds(p1x, p1y, p2x, p2y, x3, y3);
}

static void simulate_laser(void)
{
  int i = 0, j;
  carmen_map_config_t map_defn;
  int x2, y2;
  double x3, y3;
  float this_range;
  double theta;
  static int first = 1;
  carmen_point_t delta_odom, corrected_delta;

  map_defn.x_size = 400;
  map_defn.y_size = 400;
  map_defn.resolution = 1.0;

  if (ranges == NULL) {
    ranges = (float *)calloc(NUM_POINTS, sizeof(float));
    carmen_test_alloc(ranges);
    prev_ranges = (float *)calloc(NUM_POINTS, sizeof(float));
    carmen_test_alloc(prev_ranges);
    laser_poly = (GdkPoint *)calloc(NUM_POINTS+1, sizeof(GdkPoint));
    carmen_test_alloc(laser_poly);
    angles = (float *)calloc(NUM_POINTS, sizeof(float));
    carmen_test_alloc(ranges);
    for (i = 0; i < NUM_POINTS; i++) 
      angles[i] = -M_PI/2+M_PI/(double)NUM_POINTS*i;
  }

  laser_poly[0].x = x;
  laser_poly[0].y = 400-y;
  laser_poly_size = 1;

  for (i = 0; i < NUM_POINTS; i++) {
    theta = -M_PI/2+M_PI/(double)NUM_POINTS*i;
    carmen_geometry_project_point(x, y, theta, &x2, &y2, map_defn);
    ranges[i] = MAX_RANGE;
    for (j = 0; j < 4; j++) {
      intersect(x, y, x2, y2, points[j][0], points[j][1], points[(j+1)%4][0], 
		points[(j+1)% 4][1], &x3, &y3);
      if (x3 < 0 || x3 >= 400 || y3 < 0 || y3 >= 400) 
	this_range = MAX_RANGE;
      else {
	this_range = sqrt((x3-x)*(x3-x)+(y3-y)*(y3-y));
      }
      if (ranges[i] > this_range) {
	ranges[i] = this_range;
      }      
    }
    //    if (ranges[i] < MAX_RANGE)
    //      ranges[i] += carmen_gaussian_random(0, laser_sigma);
    laser_poly[laser_poly_size].x = x+cos(theta)*ranges[i];
    laser_poly[laser_poly_size].y = 400-(y+sin(theta)*ranges[i]);
    laser_poly_size++;
  }

  if (first) {
    first = 0;
    odom_position.x = x;
    odom_position.y = y;
    odom_position.theta = 0;
  } else {
    odom_position.x = x+carmen_gaussian_random(0, x_sigma);
    odom_position.y = y+carmen_gaussian_random(0, y_sigma);
    odom_position.theta = 0;
    delta_odom.x = odom_position.x - prev_position.x;
    delta_odom.y = odom_position.y - prev_position.y;
    delta_odom.theta = carmen_normalize_theta(odom_position.theta - prev_position.theta);
    corrected_delta = scan_match(ranges, prev_ranges, delta_odom);
    corrected_position.x = prev_position.x+corrected_delta.x;
    corrected_position.y = prev_position.y+corrected_delta.y;
    corrected_position.theta = carmen_normalize_theta(prev_position.theta+corrected_delta.theta);

    carmen_warn("%f %f %f : %f %f %f\n", odom_position.x, odom_position.y, 
		carmen_radians_to_degrees(odom_position.theta),
		corrected_position.x, corrected_position.y, 
		carmen_radians_to_degrees(corrected_position.theta));
  }
  memcpy(prev_ranges, ranges, sizeof(float)*NUM_POINTS);
  prev_position.x = x;
  prev_position.y = y;
  prev_position.theta = 0;
}

static void redraw(void)
{
  int i;

  if (pixmap == NULL)
    pixmap = gdk_pixmap_new(drawing_area->window, 400, 400, -1);
  if(drawing_gc == NULL)
    drawing_gc = gdk_gc_new(drawing_area->window);
  gdk_gc_set_foreground(drawing_gc, &carmen_light_blue);
  gdk_draw_rectangle(pixmap, drawing_gc, TRUE, 0, 0, 400, 400);

  gdk_gc_set_foreground(drawing_gc, &carmen_red);
  gdk_draw_polygon(pixmap, drawing_gc, TRUE, polygon, 4);

  gdk_gc_set_foreground(drawing_gc, &carmen_white);
  gdk_draw_polygon(pixmap, drawing_gc, TRUE, laser_poly, laser_poly_size);

  gdk_gc_set_foreground(drawing_gc, &carmen_green);
  if (1) {
    gdk_gc_set_foreground(drawing_gc, &carmen_green);
    for (i = 0; i < NUM_POINTS; i++) {    
      gdk_draw_arc(pixmap, drawing_gc, FALSE, 
		   odom_position.x+cos(i*M_PI/(double)NUM_POINTS-M_PI/2)*ranges[i],
		 400 - (odom_position.y+sin(i*M_PI/(double)NUM_POINTS-M_PI/2)*ranges[i]), 
		   2, 2, 0, 360 * 64);
    }    

    gdk_gc_set_foreground(drawing_gc, &carmen_red);
    for (i = 0; i < NUM_POINTS; i++) {    
      gdk_draw_arc(pixmap, drawing_gc, FALSE, 
		   prev_position.x+cos(i*M_PI/(double)NUM_POINTS-M_PI/2)*ranges[i],
		 400 - (prev_position.y+sin(i*M_PI/(double)NUM_POINTS-M_PI/2)*ranges[i]), 
		   2, 2, 0, 360 * 64);
    }
  }

  gdk_gc_set_foreground(drawing_gc, &carmen_grey);
  gdk_draw_arc(pixmap, drawing_gc, TRUE, x-10, 400 - y+10, 20, 20, 0, 360 * 64);
  gdk_gc_set_foreground(drawing_gc, &carmen_black);
  gdk_draw_arc(pixmap, drawing_gc, FALSE, x-10, 400 - y+10, 20, 20, 0, 360 * 64);

  gdk_gc_set_foreground(drawing_gc, &carmen_red);
  gdk_draw_arc(pixmap, drawing_gc, FALSE, odom_position.x-10, 
	       400 - odom_position.y+10, 20, 20, 0, 360 * 64);

  gdk_gc_set_foreground(drawing_gc, &carmen_blue);
  gdk_draw_arc(pixmap, drawing_gc, FALSE, corrected_position.x-10, 
	       400 - corrected_position.y+10, 20, 20, 0, 360 * 64);

  gdk_draw_pixmap(drawing_area->window,
                  drawing_area->style->fg_gc[GTK_WIDGET_STATE (drawing_area)],
                  pixmap, 0, 0, 0, 0, drawing_area->allocation.width,
                  drawing_area->allocation.height);
}

static gint update_contour()
{
  x += 10;

  simulate_laser();

  redraw();

  return 1;
}

static void initialize_graphics(int *argc, char **argv[])
{
  GtkWidget *window;
  GtkWidget *main_box;

  gtk_init (argc, argv);
  gdk_imlib_init();

  carmen_graphics_setup_colors();

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

  gtk_signal_connect (GTK_OBJECT (window), "key_press_event",
                      (GtkSignalFunc) key_event, NULL);

  drawing_area = gtk_drawing_area_new();

  gtk_drawing_area_size (GTK_DRAWING_AREA (drawing_area), 400, 400);
  gtk_box_pack_start(GTK_BOX(main_box), drawing_area, TRUE, TRUE, 0);
  gtk_widget_add_events(drawing_area,  GDK_KEY_PRESS_MASK
                        | GDK_KEY_RELEASE_MASK);

  gtk_widget_show_all (window);
  gtk_widget_grab_focus(window); 
  //  running_timeout = gtk_timeout_add(100, (GtkFunction)update_contour, NULL);
  paused = 1;
}


int main(int argc, char *argv[])
{
  unsigned int seed;
  int i;

  seed = carmen_randomize(&argc, &argv);
  carmen_warn("Seed: %u\n", seed);
  
  initialize_graphics(&argc, &argv);

  init_x = 0;
  init_y = 250;

  x = init_x;
  y = init_y;

  for (i = 0; i < 4; i++) {
    polygon[i].x = points[i][0];
    polygon[i].y = 400-points[i][1];
  }

  simulate_laser();

  gtk_main();
 
  return 0;
}
