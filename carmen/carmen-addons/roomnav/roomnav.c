//#define NO_GRAPHICS 1

#ifndef NO_GRAPHICS
#include <carmen/carmen_graphics.h>
#else
#include <carmen/carmen.h>
#endif

#include "roomnav_messages.h"


#define GRID_NONE       -1
#define GRID_UNKNOWN    -2
#define GRID_WALL       -3
#define GRID_DOOR       -4
#define GRID_PROBE      -5
#define GRID_VORONOI    -6
#define GRID_CRITICAL   -7
#define GRID_OFFLIMITS  -8

#define DEFAULT_GRID_RESOLUTION 0.1

#define LEVEL_CHANGE_COST 1.0

typedef struct point_node {
  int x;
  int y;
  struct point_node *next;
} point_node_t, *point_node_p;

typedef struct path_node {
  int *path;
  int pathlen;
  double cost;
  struct path_node *next;
} path_node_t, *path_node_p;

typedef struct fill_node {
  int x;
  int y;
  int fill;
  struct fill_node *next;
} fill_node_t, *fill_node_p;

typedef struct {
  point_node_p probe_stack;
  point_node_p room_stack;
  int num;
} blob_t, *blob_p;

#define MAX_NUM_MAPS 100

static int num_maps = 0;
static carmen_map_t maps[MAX_NUM_MAPS];
static carmen_map_t map;
static int **grids[MAX_NUM_MAPS];
static int **grid;
static int grid_width, grid_height;
static double grid_resolution;
static carmen_room_p rooms = NULL;
static int num_rooms = 0;
static carmen_door_p doors = NULL;
static int num_doors = 0;

#if 0
static double voronoi_dist_tolerance = 1.5;
static double voronoi_min_theta = 9.0*M_PI/18.0;
static double voronoi_min_dist = 4.0;
static double voronoi_local_min_filter_dist = 1.0;
static int voronoi_local_min_filter_inner_radius = 2;
static int voronoi_local_min_filter_outer_radius = 5;
static double voronoi_critical_point_min_theta = 16.0*M_PI/18.0;
//static int voronoi_critical_point_min_dist = 5.0;
#endif

static double hallway_eccentricity_threshold = 9.0;

static int loading_map = 0;

carmen_localize_globalpos_message global_pos;
static int room = -1;
static int goal = -1;
static int *path = NULL;
static int pathlen = -1;  // 0 = goal reached, -1 = goal unreachable

static FILE *roomnav_logfile_fp = NULL;

#ifndef NO_GRAPHICS

static GdkGC *drawing_gc = NULL;
static GdkPixmap *pixmap = NULL;
static GtkWidget *window, *canvas;
static int canvas_width = 300, canvas_height = 300;

static void draw_canvas(int x0, int y0, int width, int height);
static void draw_grid(int x0, int y0, int width, int height);
static void grid_to_image(int x, int y, int width, int height);
static void draw_arrow(int x, int y, double theta, int width, int height,
		       GdkColor color);

#endif

static int closest_room(int x, int y, int max_shell);

static int fast = 0;

static int get_farthest(int door, int place);
static void set_goal(int new_goal);

//#define MIN(x,y) ((x) < (y) ? (x) : (y))
//#define MAX(x,y) ((x) > (y) ? (x) : (y))
//#define ABS(x) ((x) < 0 ? -(x) : (x))

static inline double dist(double x, double y) {

  return sqrt(x*x + y*y);
}

static inline double idist(int x, int y) {

  return sqrt((double)(x*x + y*y));
}

static inline int round_to_int(double x) {

  int xi, negative = 0;

  if (x < 0) {
    x = -x;
    negative = 1;
  }

  xi = ((int)x) + (x - (int)x > 0.5);

  return (negative ? -xi : xi);
}

static void print_doors() {

  int i;

  for (i = 0; i < num_doors; i++) {
    printf("door %d:  ", i);
    printf("rooms(%d, %d), ", doors[i].room1 != -1 ? rooms[doors[i].room1].num : -1,
	   doors[i].room2 != -1 ? rooms[doors[i].room2].num : -1);
    printf("pose(%.2f, %.2f, %.0f), ", doors[i].pose.x, doors[i].pose.y,
	   carmen_radians_to_degrees(doors[i].pose.theta));
    printf("width(%.2f)", doors[i].width);
    printf("\n");
  }
}

// fmt of place names: "[b|d]<door>.<place>" 
static int get_doors(carmen_map_placelist_p placelist) {

  int num_doornames, num_places, *num_doorplaces;
  int i, j, k, n, e1, e2;
  double e1x, e1y, e2x, e2y;
  char **doornames, *placename;

  printf("get_doors()\n");

  num_doornames = 0;
  num_places = placelist->num_places;

  doornames = (char **) calloc(num_places, sizeof(char *));
  carmen_test_alloc(doornames);
  num_doorplaces = (int *) calloc(num_places, sizeof(int));
  carmen_test_alloc(num_doorplaces);

  printf("break 1\n");

  // get doornames & num_doors
  for (i = 0; i < num_places; i++) {
    placename = placelist->places[i].name;
    if (placename[0] != 'b' && placename[0] != 'd')
      continue;
    if (placename[strspn(placename+1, "0123456789") + 1] != '.')
      continue;
    n = strcspn(placename, ".");
    for (j = 0; j < num_doornames; j++)
      if ((n == (int)strlen(doornames[j])) &&
	  !strncmp(doornames[j], placename, n))
	break;
    if (j < num_doornames)
      num_doorplaces[j]++;
    else {
      doornames[j] = (char *) calloc(n+1, sizeof(char));
      carmen_test_alloc(doornames[j]);
      strncpy(doornames[j], placename, n);
      doornames[j][n] = '\0';
      num_doorplaces[j] = 1;
      num_doornames++;
    }
  }
  
  printf("break 2\n");

  doors = (carmen_door_p)
    realloc(doors, (num_doors + num_doornames) * sizeof(carmen_door_t));
  carmen_test_alloc(doors);

  printf("break 3\n");

  // get door places
  for (j = num_doors; j < num_doors + num_doornames; j++) {
    printf("break 3.%d.1\n", j);
    doors[j].num = j;
    doors[j].is_real_door = (doornames[j-num_doors][0] == 'd');
    doors[j].points.num_places = num_doorplaces[j-num_doors];
    doors[j].points.places =
      (carmen_place_p) calloc(num_doorplaces[j-num_doors], sizeof(carmen_place_t));
    carmen_test_alloc(doors[j].points.places);
    k = 0;
    printf("break 3.%d.2\n", j);
    for (i = 0; i < num_places; i++) {
      placename = placelist->places[i].name;
      n = strcspn(placename, ".");
      if ((n == (int)strlen(doornames[j-num_doors])) &&
	  !strncmp(doornames[j-num_doors], placename, n))
	memcpy(&doors[j].points.places[k++],
	       &placelist->places[i], sizeof(carmen_place_t));
    }
    printf("break 3.%d.3\n", j);
    free(doornames[j-num_doors]);

    printf("break 3.%d.4\n", j);
    e1 = get_farthest(j, 0);
    e2 = get_farthest(j, e1);
    printf("break 3.%d.5\n", j);
    e1x = doors[j].points.places[e1].x;
    e1y = doors[j].points.places[e1].y;
    e2x = doors[j].points.places[e2].x;
    e2y = doors[j].points.places[e2].y;
    doors[j].pose.x = (e1x + e2x) / 2.0;
    doors[j].pose.y = (e1y + e2y) / 2.0;
    doors[j].width = dist(e2x - e1x, e2y - e1y);
  }

  printf("break 4\n");

  num_doors += num_doornames;

  free(doornames);
  free(num_doorplaces);

  printf("break 5\n");

  return num_doornames;
}

/* returns room number */
static int probe(int px, int py) {

  blob_p blob;
  point_node_p tmp;
  int x, y, i, j, step, cell, num;
  int bbx0, bby0, bbx1, bby1;

  blob = (blob_p) calloc(1, sizeof(blob_t));
  carmen_test_alloc(blob);

  blob->num = -1;
  blob->probe_stack = (point_node_p) calloc(1, sizeof(point_node_t));
  carmen_test_alloc(blob->probe_stack);
  blob->probe_stack->x = px;
  blob->probe_stack->y = py;
  blob->probe_stack->next = NULL;
  blob->room_stack = NULL;

  step = (int) (0.4 / grid_resolution);
  if (step == 0)
    step = 1;

  bbx0 = px;
  bby0 = py;
  bbx1 = px + step;
  bby1 = py + step;
  
  while (blob->probe_stack != NULL) {

    x = blob->probe_stack->x;
    y = blob->probe_stack->y;      

    if (x < bbx0)
      bbx0 = x;
    else if (x + step > bbx1)
      bbx1 = x + step;
    if (y < bby0)
      bby0 = y;
    else if (y + step > bby1)
      bby1 = y + step;

    tmp = blob->probe_stack;
    blob->probe_stack = blob->probe_stack->next;
    free(tmp);

    if (x < 0 || x >= grid_width || y < 0 || y >= grid_height)
      continue;

    for (i = x; (i < x + step) && (i < grid_width); i++) {
      for (j = y; (j < y + step) && (j < grid_height); j++) {
	grid[i][j] = GRID_PROBE;
	tmp = (point_node_p) calloc(1, sizeof(point_node_t));
	carmen_test_alloc(tmp);
	tmp->x = i;
	tmp->y = j;
	tmp->next = blob->room_stack;
	blob->room_stack = tmp;
      }
    }
    
    // up
    cell = GRID_NONE;
    for (i = x; (i < x + step) && (i < grid_width) && (cell == GRID_NONE); i++)
      for (j = y + step; j < y + 2*step && j < grid_height && cell == GRID_NONE; j++)
	cell = grid[i][j];
    if (cell >= 0)
      blob->num = cell;
    else if (cell == GRID_NONE) {
      tmp = (point_node_p) calloc(1, sizeof(point_node_t));
      carmen_test_alloc(tmp);
      tmp->x = x;
      tmp->y = y + step;
      tmp->next = blob->probe_stack;
      blob->probe_stack = tmp;
    }
    
    // down
    cell = GRID_NONE;
    for (i = x; (i < x + step) && (i < grid_width) && (cell == GRID_NONE); i++)
      for (j = y - 1; (j >= y - step) && (j >= 0) && (cell == GRID_NONE); j--)
	cell = grid[i][j];
    if (cell >= 0)
      blob->num = cell;
    else if (cell == GRID_NONE) {
      tmp = (point_node_p) calloc(1, sizeof(point_node_t));
      carmen_test_alloc(tmp);
      tmp->x = x;
      tmp->y = y - step;
      tmp->next = blob->probe_stack;
      blob->probe_stack = tmp;
    }
  
    // right
    cell = GRID_NONE;
    for (i = x + step; i < x + 2*step && i < grid_width && cell == GRID_NONE; i++)
      for (j = y; j < y + step && j < grid_height && cell == GRID_NONE; j++)
	cell = grid[i][j];
    if (cell >= 0)
      blob->num = cell;
    else if (cell == GRID_NONE) {
      tmp = (point_node_p) calloc(1, sizeof(point_node_t));
      carmen_test_alloc(tmp);
      tmp->x = x + step;
      tmp->y = y;
      tmp->next = blob->probe_stack;
      blob->probe_stack = tmp;
    }
    
    // left
    cell = GRID_NONE;
    for (i = x-1; i >= x - step && i >= 0 && cell == GRID_NONE; i--)
      for (j = y; j < y + step && j < grid_height && cell == GRID_NONE; j++)
	cell = grid[i][j];
    if (cell >= 0)
      blob->num = cell;
    else if (cell == GRID_NONE) {
      tmp = (point_node_p) calloc(1, sizeof(point_node_t));
      carmen_test_alloc(tmp);
      tmp->x = x - step;
      tmp->y = y;
      tmp->next = blob->probe_stack;
      blob->probe_stack = tmp;
    }

#ifndef NO_GRAPHICS
    if (!fast) {
      grid_to_image(x-2, y-2, step+4, step+4);
      draw_grid(x-2, y-2, step+4, step+4);
    }
#endif

  }

  if (blob->num == -1)
    blob->num = num_rooms;

  num = blob->num;

  while(blob->room_stack != NULL) {
    tmp = blob->room_stack;
    grid[tmp->x][tmp->y] = blob->num;
    blob->room_stack = blob->room_stack->next;
    free(tmp);
  }

  free(blob);

  bbx0 = carmen_clamp(0, bbx0-2, grid_width);
  bby0 = carmen_clamp(0, bby0-2, grid_height);
  bbx1 = carmen_clamp(0, bbx1+2, grid_width);
  bby1 = carmen_clamp(0, bby1+2, grid_height);

#ifndef NO_GRAPHICS
  grid_to_image(bbx0, bby0, bbx1 - bbx0, bby1 - bby0);
  draw_grid(bbx0, bby0, bbx1 - bbx0, bby1 - bby0);
#endif

  return num;
}

static int get_farthest(int door, int place) {

  int i, n, f;
  double x, y, d, d2;
  carmen_place_p p;

  n = doors[door].points.num_places;
  p = doors[door].points.places;

  x = p[place].x;
  y = p[place].y;

  d = f = 0;

  for (i = 0; i < n; i++) {
    if (i == place)
      continue;
    d2 = dist(x - p[i].x, y - p[i].y);
    if (d < d2) {
      d = d2;
      f = i;
    }
  }

  return f;
}

static void get_room_names(carmen_map_placelist_p placelist) {

  carmen_world_point_t world_point;
  carmen_map_point_t map_point;
  char *placename;
  int i, r, type;

  type = 0;

  world_point.map = &map;
  map_point.map = &map;

  for (r = 0; r < num_rooms; r++)
    rooms[r].type = CARMEN_ROOM_TYPE_UNKNOWN;

  for (i = 0; i < placelist->num_places; i++) {
    placename = placelist->places[i].name;
    if (placename[0] == 'R')
      type = CARMEN_ROOM_TYPE_ROOM;
    else if (placename[0] == 'H')
      type = CARMEN_ROOM_TYPE_HALLWAY;
    else if (placename[0] == 'r')
      type = CARMEN_ROOM_TYPE_UNKNOWN;
    else
      continue;
    placename++;
    if (*placename == '\0')
      continue;
    placename += strspn(placename, "0123456789");
    if (*placename != '\0' && *placename != ' ' && *placename != '\t')
      continue;
    world_point.pose.x = placelist->places[i].x;
    world_point.pose.y = placelist->places[i].y;
    carmen_world_to_map(&world_point, &map_point);
    r = closest_room(map_point.x, map_point.y, 10);
    if (r == -1)
      continue;
    rooms[r].type = type;
    placename += strspn(placename, " \t");
    if (*placename == '\0')
      continue;
    rooms[r].name = (char *) realloc(rooms[r].name, (strlen(placename) + 1) * sizeof(char));
    carmen_test_alloc(rooms[r].name);
    strcpy(rooms[r].name, placename);
  }
}

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

#ifndef NO_GRAPHICS
static void draw_hallway_endpoints() {

  carmen_world_point_t world_point;
  carmen_map_point_t map_point;
  int r;

  world_point.map = map_point.map = &map;

  for (r = 0; r < num_rooms; r++) {
    world_point.pose.x = rooms[r].e1.x;
    world_point.pose.y = rooms[r].e1.y;
    carmen_world_to_map(&world_point, &map_point);
    draw_arrow(map_point.x, map_point.y, rooms[r].theta - M_PI, 20, 10, carmen_white);
    world_point.pose.x = rooms[r].e2.x;
    world_point.pose.y = rooms[r].e2.y;
    carmen_world_to_map(&world_point, &map_point);
    draw_arrow(map_point.x, map_point.y, rooms[r].theta, 20, 10, carmen_white);
  }
}
#endif

static void get_room_types(int num_new_rooms) {

  int i, j, r;
  double x, y, x2, y2, vx, vy, vxy;

  for (r = num_rooms - num_new_rooms; r < num_rooms; r++) {
    rooms[r].num_cells = 0;
    rooms[r].ux = rooms[r].uy = rooms[r].vx = rooms[r].vy = rooms[r].vxy = 0;
    rooms[r].e1.y = rooms[r].e2.y = 0.0;
    rooms[r].e1.theta = rooms[r].e2.theta = -1.0;
  }

  // get room centroids and extremities
  for (i = 0; i < grid_width; i++) {
    for (j = 0; j < grid_height; j++) {
      r = grid[i][j];
      if (r >= num_rooms - num_new_rooms) {
	x = i * map.config.resolution;
	y = j * map.config.resolution;
	rooms[r].ux += x;
	rooms[r].uy += y;
	rooms[r].num_cells++;
      }
    }
  }
  for (r = num_rooms - num_new_rooms; r < num_rooms; r++) {
    rooms[r].ux /= (double) rooms[r].num_cells;
    rooms[r].uy /= (double) rooms[r].num_cells;
  }

  // get covariances
  for (i = 0; i < grid_width; i++) {
    for (j = 0; j < grid_height; j++) {
      r = grid[i][j];
      if (r >= num_rooms - num_new_rooms) {
	x = i * map.config.resolution;
	y = j * map.config.resolution;
	rooms[r].vx += (x - rooms[r].ux)*(x - rooms[r].ux);
	rooms[r].vy += (y - rooms[r].uy)*(y - rooms[r].uy);
	rooms[r].vxy += (x - rooms[r].ux)*(y - rooms[r].uy);
      }
    }
  }
  for (r = num_rooms - num_new_rooms; r < num_rooms; r++) {
    rooms[r].vx /= (double) rooms[r].num_cells;
    rooms[r].vy /= (double) rooms[r].num_cells;
    rooms[r].vxy /= (double) rooms[r].num_cells;
    vx = rooms[r].vx;
    vy = rooms[r].vy;
    vxy = rooms[r].vxy;
    rooms[r].theta = bnorm_theta(vx, vy, vxy);
    rooms[r].w1 = (vx + vy)/2.0 + sqrt(vxy*vxy + (vx-vy)*(vx-vy)/4.0);
    rooms[r].w2 = (vx + vy)/2.0 - sqrt(vxy*vxy + (vx-vy)*(vx-vy)/4.0);
    if (rooms[r].type == CARMEN_ROOM_TYPE_UNKNOWN) {
      if (rooms[r].w1 / rooms[r].w2 > hallway_eccentricity_threshold)
	rooms[r].type = CARMEN_ROOM_TYPE_HALLWAY;
      else
	rooms[r].type = CARMEN_ROOM_TYPE_ROOM;
    }
  }

  // get hallway endpoints in local coordinates
  for (i = 0; i < grid_width; i++) {
    for (j = 0; j < grid_height; j++) {
      r = grid[i][j];
      if (rooms[r].type == CARMEN_ROOM_TYPE_HALLWAY) {
	if (r >= num_rooms - num_new_rooms) {
	  x = i * map.config.resolution;
	  y = j * map.config.resolution;
	  x2 = x - rooms[r].ux;
	  y2 = y - rooms[r].uy;
	  carmen_rotate_2d(&x2, &y2, -rooms[r].theta);
	  if (rooms[r].e1.theta < 0.0 || x2 < rooms[r].e1.x) {
	    rooms[r].e1.x = x2;
	    rooms[r].e1.theta = 0.0;
	  }
	  if (rooms[r].e2.theta < 0.0 || x2 > rooms[r].e2.x) {
	    rooms[r].e2.x = x2;
	    rooms[r].e2.theta = 0.0;
	  }
	}
      }
    }  
  }

  // transform hallway endpoints to global coordinates
  for (r = num_rooms - num_new_rooms; r < num_rooms; r++) {
    if (rooms[r].type == CARMEN_ROOM_TYPE_HALLWAY) {
      carmen_rotate_2d(&rooms[r].e1.x, &rooms[r].e1.y, rooms[r].theta);
      rooms[r].e1.x += rooms[r].ux;
      rooms[r].e1.y += rooms[r].uy;
      carmen_rotate_2d(&rooms[r].e2.x, &rooms[r].e2.y, rooms[r].theta);
      rooms[r].e2.x += rooms[r].ux;
      rooms[r].e2.y += rooms[r].uy;
    }
  }
}

static void grid_draw_world_line(double x1, double y1, double x2, double y2, int cell_type) {

  double x, y, dx, dy, d;
  carmen_world_point_t world_point;
  carmen_map_point_t map_point;

  world_point.map = &map;
  map_point.map = &map;

  dx = x2 - x1;
  dy = y2 - y1;
  d = dist(dx, dy);

  for (x = x1, y = y1;
       (dx < 0 ? x >= x2 : x <= x2) && (dy < 0 ? y >= y2 : y <= y2);
       x += dx*grid_resolution/d, y += dy*grid_resolution/d) {
    world_point.pose.x = x;
    world_point.pose.y = y;
    carmen_world_to_map(&world_point, &map_point);
    grid[map_point.x][map_point.y] = cell_type;
  }
}

static void grid_draw_map_line(int x1, int y1, int x2, int y2, int cell_type) {

  double x, y, dx, dy, d;

  dx = x2 - x1;
  dy = y2 - y1;
  d = idist(dx, dy);

  for (x = x1, y = y1;
       (dx < 0 ? x >= x2 : x <= x2) && (dy < 0 ? y >= y2 : y <= y2);
       x += dx*grid_resolution/d, y += dy*grid_resolution/d) {
    grid[round_to_int(x)][round_to_int(y)] = cell_type;
  }
}

static void mark_offlimits() {

  int i, n;
  carmen_offlimits_p offlimits;
  
  printf("marking offlimits...");

  if (carmen_map_get_offlimits(&offlimits, &n) < 0) {
    printf("none\n");
    return;
  }

  for (i = 0; i < n; i++) {
    if (offlimits[i].type == CARMEN_OFFLIMITS_LINE_ID) {
      grid_draw_map_line(offlimits[i].x1, offlimits[i].y1,
			 offlimits[i].x2, offlimits[i].y2, GRID_OFFLIMITS);
      printf("%d", i);
    }
  }

  printf("\n");
}

static void get_rooms(carmen_map_placelist_p placelist, int num_new_doors) {

  int i, j, n, e1, e2;
  double dx, dy, d;
  double e1x, e1y;  // endpoint 1
  double e2x, e2y;  // endpoint 2
  double mx, my;    // midpoint
  double p1x, p1y;  // probe 1
  double p2x, p2y;  // probe 2
  carmen_world_point_t world_point;
  carmen_map_point_t map_point;
  int num_new_rooms = 0;

  mark_offlimits();

  rooms = (carmen_room_p) realloc(rooms, (num_rooms + num_new_doors + 1) * sizeof(carmen_room_t));
  carmen_test_alloc(rooms);

  world_point.map = &map;
  map_point.map = &map;

  // mark off door
  for (i = num_doors - num_new_doors; i < num_doors; i++) {

    e1 = get_farthest(i, 0);
    e2 = get_farthest(i, e1);

    e1x = doors[i].points.places[e1].x;
    e1y = doors[i].points.places[e1].y;
    e2x = doors[i].points.places[e2].x;
    e2y = doors[i].points.places[e2].y;

    grid_draw_world_line(e1x, e1y, e2x, e2y, GRID_DOOR);
  }

#ifndef NO_GRAPHICS

  grid_to_image(0, 0, grid_width, grid_height);
  draw_grid(0, 0, grid_width, grid_height);

#endif

  // get probes
  for (i = num_doors - num_new_doors; i < num_doors; i++) {

    e1 = get_farthest(i, 0);
    e2 = get_farthest(i, e1);

    e1x = doors[i].points.places[e1].x;
    e1y = doors[i].points.places[e1].y;
    e2x = doors[i].points.places[e2].x;
    e2y = doors[i].points.places[e2].y;

    mx = (e1x + e2x) / 2.0;
    my = (e1y + e2y) / 2.0;

    dx = e2x - e1x;
    dy = e2y - e1y;
    d = dist(dx, dy);

    p1x = mx + dy / d;
    p1y = my - dx / d;

    p2x = mx - dy / d;
    p2y = my + dx / d;

    doors[i].pose.theta = atan2(p2y - p1y, p2x - p1x);

    world_point.pose.x = p1x;
    world_point.pose.y = p1y;
    carmen_world_to_map(&world_point, &map_point);
    n = probe(map_point.x, map_point.y);

    doors[i].room1 = n;

    // new room
    if (n == num_rooms) {
      rooms[n].num = n;
      rooms[n].name = (char *) calloc(16, sizeof(char));
      carmen_test_alloc(rooms[n].name);
      sprintf(rooms[n].name, "room %d", n);
      rooms[n].doors = (int *) calloc(num_doors, sizeof(int));
      carmen_test_alloc(rooms[n].doors);
      rooms[n].num_doors = 0;
      num_rooms++;
      num_new_rooms++;
    }

    for (j = 0; j < rooms[n].num_doors; j++)
      if (doors[rooms[n].doors[j]].num == i)
	break;

    // add current door to room
    if (j == rooms[n].num_doors)
      rooms[n].doors[rooms[n].num_doors++] = i;

    world_point.pose.x = p2x;
    world_point.pose.y = p2y;
    carmen_world_to_map(&world_point, &map_point);
    n = probe(map_point.x, map_point.y);

    doors[i].room2 = n;

    if (n == num_rooms) {
      rooms[n].num = n;
      rooms[n].name = (char *) calloc(16, sizeof(char));
      carmen_test_alloc(rooms[n].name);
      sprintf(rooms[n].name, "room %d", n);
      rooms[n].doors = (int *) calloc(num_doors, sizeof(int));
      carmen_test_alloc(rooms[n].doors);
      rooms[n].num_doors = 0;
      num_rooms++;
      num_new_rooms++;
    }

    for (j = 0; j < rooms[n].num_doors; j++)
      if (doors[rooms[n].doors[j]].num == i)
	break;

    // add current door to room
    if (j == rooms[n].num_doors)
      rooms[n].doors[rooms[n].num_doors++] = i;
  }

  get_room_names(placelist);
  get_room_types(num_new_rooms);
}

#ifndef NO_GRAPHICS
static void canvas_resize() {

  double ratio;

  ratio = sqrt(canvas_width * canvas_height / ((double) grid_width * grid_height));
  canvas_width = grid_width;
  canvas_height = grid_height;
  canvas_width *= ratio;
  canvas_height *= ratio;

  gtk_drawing_area_size(GTK_DRAWING_AREA(canvas), canvas_width, canvas_height);

  while(gtk_events_pending())
    gtk_main_iteration_do(TRUE);

  grid_to_image(0, 0, grid_width, grid_height);
  draw_grid(0, 0, grid_width, grid_height);
}
#endif

static void map_switch(int map_num) {

  map = maps[map_num];
  grid = grids[map_num];

  grid_resolution = map.config.resolution; //dbug

  grid_width = (int) map.config.x_size;
  grid_height = (int) map.config.y_size;
  
#ifndef NO_GRAPHICS
  canvas_resize();
#endif
}

static void grid_init() {

  int i, j;

  grid_resolution = map.config.resolution; //dbug

  grid_width = (int) map.config.x_size;
  grid_height = (int) map.config.y_size;

  grid = (int **) calloc(grid_width, sizeof(int *));
  carmen_test_alloc(grid);

  for (i = 0; i < grid_width; i++) {
    grid[i] = (int *) calloc(grid_height, sizeof(int));
    carmen_test_alloc(grid[i]);
  }

  //dbug
  /*  
  grid = (int **) calloc(grid_width * (grid_height + 1), sizeof(int *));
  carmen_test_alloc(grid);
  for (i = 0; i < grid_width; i++)
    grid[i] = (int *)(grid + grid_width + i*grid_height);
  
  printf("------------------------------------------------------\n"
	 "grid start = 0x%x, grid end = 0x%x\n"
	 "------------------------------------------------------\n",
	 (unsigned) grid, (unsigned) (grid + grid_width * (grid_height + 1)));
  */

  for (i = 0; i < grid_width; i++) {
    for (j = 0; j < grid_height; j++) {
      if (map.map[i][j] < 0.0)
	grid[i][j] = GRID_UNKNOWN;
      else if (map.map[i][j] < 0.5)
	grid[i][j] = GRID_NONE;
      else
	grid[i][j] = GRID_WALL;
    }
  }

#ifndef NO_GRAPHICS
  canvas_resize();
#endif
}

static inline int grid_oob(int x, int y) {

  return (x < 0 || x >= grid_width || y < 0 || y >= grid_height);
}

static int closest_room(int x, int y, int max_shell) {

  int i, j, x2, y2, shell;

  if (x < 0 || x >= grid_width || y < 0 || y >= grid_height)
    return -1;

  if (grid[x][y] >= 0)
    return grid[x][y];

  for (i = 0; i < 4*max_shell; i++) {
    shell = i/4+1;
    for (j = 0; j < 2*shell; j++) {
      switch (i % 4) {
      case 0:
	x2 = carmen_clamp(0, x-shell+j, grid_width-1);
	y2 = carmen_clamp(0, y+shell, grid_height-1);
	if (grid[x2][y2] >= 0)
	  return grid[x2][y2];
	break;
      case 1:
	x2 = carmen_clamp(0, x+shell, grid_width-1);
	y2 = carmen_clamp(0, y+shell-j, grid_height-1);
	if (grid[x2][y2] >= 0)
	  return grid[x2][y2];
	break;
      case 2:
	x2 = carmen_clamp(0, x+shell-j, grid_width-1);
	y2 = carmen_clamp(0, y-shell, grid_height-1);
	if (grid[x2][y2] >= 0)
	  return grid[x2][y2];
	break;
      case 3:
	x2 = carmen_clamp(0, x-shell, grid_width-1);
	y2 = carmen_clamp(0, y-shell+j, grid_height-1);
	if (grid[x2][y2] >= 0)
	  return grid[x2][y2];
      }
    }
  }

  return -1;
}

/*
static void cleanup_map() {

  int i, j;
  fill_node_p tmp, fill_stack;
  double t0, t1, t2, t3;

  return; //dbug!

  t0 = carmen_get_time_ms();

  for (i = 0; i < grid_width; i++)
    for (j = 0; j < grid_height; j++)
      if (closest_room(i,j,7) == -1)
	grid[i][j] = GRID_UNKNOWN;

  t1 = carmen_get_time_ms();

  fill_stack = NULL;
  
  for (i = 0; i < grid_width; i++) {
    for (j = 0; j < grid_height; j++) {
      if (grid[i][j] == GRID_NONE) {
	tmp = (fill_node_p) calloc(1, sizeof(fill_node_t));
	carmen_test_alloc(tmp);
	tmp->x = i;
	tmp->y = j;
	tmp->fill = closest_room(i,j,7);
	tmp->next = fill_stack;
	fill_stack = tmp;
      }
    }
  }

  t2 = carmen_get_time_ms();

  while (fill_stack != NULL) {
    grid[fill_stack->x][fill_stack->y] = fill_stack->fill;
    tmp = fill_stack;
    fill_stack = fill_stack->next;
    free(tmp);
  }

  t3 = carmen_get_time_ms();

  printf("cleanup_map: %.2fms, %.2fms, %.2fms\n", t1-t0, t2-t1, t3-t2);
}
*/

//static void dbug_func() { return; }

#if 0
static double dist_to_wall(int x, int y) {

  int i, xi, yi, w, wx, wy, shell;

  w = 0;
  wx = wy = -1;
  for (shell = 1; ; shell++) {  // find min dist
    // Quadrant I
    for (i = 0; i < shell; i++) {
      xi = x+shell-i;
      yi = y+i;
      if (xi >= grid_width || yi >= grid_height)
	continue;
      if (grid[xi][yi] == GRID_WALL) {
	if (!w) {
	  w = shell;
	  wx = xi;
	  wy = yi;
	}
	else if (idist(xi-x, yi-y) < idist(wx-x, wy-y)) {
	  wx = xi;
	  wy = yi;
	}
      }
    }
    // Quadrant II
    for (i = 0; i < shell; i++) {
      xi = x-i;
      yi = y+shell-i;
      if (xi < 0 || yi >= grid_height)
	continue;
      if (grid[xi][yi] == GRID_WALL) {
	if (!w) {
	  w = shell;
	  wx = xi;
	  wy = yi;
	}
	else if (idist(xi-x, yi-y) < idist(wx-x, wy-y)) {
	  wx = xi;
	  wy = yi;
	}
      }
    }
    // Quadrant III
    for (i = 0; i < shell; i++) {
      xi= x-shell+i;
      yi = y-i;
      if (xi < 0 || yi < 0)
	continue;
      if (grid[xi][yi] == GRID_WALL) {
	if (!w) {
	  w = shell;
	  wx = xi;
	  wy = yi;
	}
	else if (idist(xi-x, yi-y) < idist(wx-x, wy-y)) {
	  wx = xi;
	  wy = yi;
	}
      }
    }
    // Quadrant IV
    for (i = 0; i < shell; i++) {
      xi = x+i;
      yi = y-shell+i;
      if (xi >= grid_width || yi < 0)
	continue;
      if (grid[xi][yi] == GRID_WALL) {
	if (!w) {
	  w = shell;
	  wx = xi;
	  wy = yi;
	}
	else if (idist(xi-x, yi-y) < idist(wx-x, wy-y)) {
	  wx = xi;
	  wy = yi;
	}
      }
    }
    if (w && 2*shell > 3*w)
      break;
  }
  
  return idist(wx-x, wy-y);
}

static void get_voronoi() {

  int x, y, xi, yi, w, w2, wx, wy, shell, oob, i;
  double wdist = 0.0;

  printf("\n");
  for (x = 0; x < grid_width; x++) {
    printf("\rGetting Voronoi Diagram: %2d%% complete", (int) (100 * (x / (double) grid_width)));
    fflush(0);
    grid_to_image(x, 0, x+1, grid_height);
    draw_grid(x, 0, x+1, grid_height);
    for (y = 0; y < grid_height; y++) {
      if (grid[x][y] != GRID_WALL && grid[x][y] != GRID_UNKNOWN) {
	wdist = dist_to_wall(x, y);
	if (wdist < voronoi_min_dist)
	  continue;
	w = w2 = 0;
	wx = wy = -1;
	for (shell = 1; !w2; shell++) {  // find # walls at min dist
	  oob = 0;
	  // Quadrant I
	  for (i = 0; !w2 && i < shell; i++) {
	    xi = x+shell-i;
	    yi = y+i;
	    if (xi >= grid_width || yi >= grid_height)
	      oob = 1;
	    else if (grid[xi][yi] == GRID_WALL) {
	      if (idist(xi-x, yi-y) - wdist < voronoi_dist_tolerance) {
		if (!w) {
		  w = shell;
		  wx = xi;
		  wy = yi;
		}
		else if (fabs(carmen_normalize_theta(atan2(wy-y, wx-x) - atan2(yi-y, xi-x))) > voronoi_min_theta)
		  w2 = 1;
	      }
	    }
	  }
	  // Quadrant II
	  for (i = 0; !w2 && i < shell; i++) {
	    xi = x-i;
	    yi = y+shell-i;
	    if (xi < 0 || yi >= grid_height)
	      oob = 1;
	    else if (grid[xi][yi] == GRID_WALL) {
	      if (idist(xi-x, yi-y) - wdist < voronoi_dist_tolerance) {
		if (!w) {
		  w = shell;
		  wx = xi;
		  wy = yi;
		}
		else if (fabs(carmen_normalize_theta(atan2(wy-y, wx-x) - atan2(yi-y, xi-x))) > voronoi_min_theta)
		  w2 = 1;
	      }
	    }
	  }
	  // Quadrant III
	  for (i = 0; !w2 &&i < shell; i++) {
	    xi= x-shell+i;
	    yi = y-i;
	    if (xi < 0 || yi < 0)
	      oob = 1;
	    else if (grid[xi][yi] == GRID_WALL) {
	      if (idist(xi-x, yi-y) - wdist < voronoi_dist_tolerance) {
		if (!w) {
		  w = shell;
		  wx = xi;
		  wy = yi;
		}
		else if (fabs(carmen_normalize_theta(atan2(wy-y, wx-x) - atan2(yi-y, xi-x))) > voronoi_min_theta)
		  w2 = 1;
	      }
	    }
	  }
	  // Quadrant IV
	  for (i = 0; !w2 && i < shell; i++) {
	    xi = x+i;
	    yi = y-shell+i;
	    if (xi >= grid_width || yi < 0)
	      oob = 1;
	    else if (grid[xi][yi] == GRID_WALL) {
	      if (idist(xi-x, yi-y) - wdist < voronoi_dist_tolerance) {
		if (!w) {
		  w = shell;
		  wx = xi;
		  wy = yi;
		}
		else if (fabs(carmen_normalize_theta(atan2(wy-y, wx-x) - atan2(yi-y, xi-x))) > voronoi_min_theta)
		  w2 = 1;
	      }
	    }
	  }
	  if (w2)
	    grid[x][y] = GRID_VORONOI;
	  else if (oob || (w && 2*shell > 3*w))
	    break;
	}
      }
    }
  }
  printf("\rGetting Voronoi Diagram: 100%% complete\n");
}

static void draw_critical_line(int x, int y, double wdist) {
  
  int xi, yi, w, w2, wx, wy, w2x, w2y, shell, oob, i;

  w = w2 = 0;
  wx = wy = w2x = w2y = -1;
  oob = 0;
  for (shell = 1; !w2 && !oob && !(w && 2*shell > 3*w); shell++) {
    // Quadrant I
    for (i = 0; !w2 && i < shell; i++) {
      xi = x+shell-i;
      yi = y+i;
      if (xi >= grid_width || yi >= grid_height)
	oob = 1;
      else if (grid[xi][yi] == GRID_WALL) {
	if (idist(xi-x, yi-y) - wdist < voronoi_dist_tolerance) {
	  if (!w) {
	    w = shell;
	    wx = xi;
	    wy = yi;
	  }
	  else if (fabs(carmen_normalize_theta(atan2(wy-y, wx-x) - atan2(yi-y, xi-x))) > voronoi_min_theta) {
	    w2 = 1;
	    w2x = xi;
	    w2y = yi;
	  }
	}
      }
    }
    // Quadrant II
    for (i = 0; !w2 && i < shell; i++) {
      xi = x-i;
      yi = y+shell-i;
      if (xi < 0 || yi >= grid_height)
	oob = 1;
      else if (grid[xi][yi] == GRID_WALL) {
	if (idist(xi-x, yi-y) - wdist < voronoi_dist_tolerance) {
	  if (!w) {
	    w = shell;
	    wx = xi;
	    wy = yi;
	  }
	  else if (fabs(carmen_normalize_theta(atan2(wy-y, wx-x) - atan2(yi-y, xi-x))) > voronoi_min_theta) {
	    w2 = 1;
	    w2x = xi;
	    w2y = yi;
	  }
	}
      }
    }
    // Quadrant III
    for (i = 0; !w2 &&i < shell; i++) {
      xi= x-shell+i;
      yi = y-i;
      if (xi < 0 || yi < 0)
	oob = 1;
      else if (grid[xi][yi] == GRID_WALL) {
	if (idist(xi-x, yi-y) - wdist < voronoi_dist_tolerance) {
	  if (!w) {
	    w = shell;
	    wx = xi;
	    wy = yi;
	  }
	  else if (fabs(carmen_normalize_theta(atan2(wy-y, wx-x) - atan2(yi-y, xi-x))) > voronoi_min_theta) {
	    w2 = 1;
	    w2x = xi;
	    w2y = yi;
	  }
	}
      }
    }
    // Quadrant IV
    for (i = 0; !w2 && i < shell; i++) {
      xi = x+i;
      yi = y-shell+i;
      if (xi >= grid_width || yi < 0)
	oob = 1;
      else if (grid[xi][yi] == GRID_WALL) {
	if (idist(xi-x, yi-y) - wdist < voronoi_dist_tolerance) {
	  if (!w) {
	    w = shell;
	    wx = xi;
	    wy = yi;
	  }
	  else if (fabs(carmen_normalize_theta(atan2(wy-y, wx-x) - atan2(yi-y, xi-x))) > voronoi_min_theta) {
	    w2 = 1;
	    w2x = xi;
	    w2y = yi;
	  }
	}
      }
    }
  }

  if (w2) {
    if (fabs(carmen_normalize_theta(atan2(wy-y, wx-x) - atan2(w2y-y, w2x-x))) > voronoi_critical_point_min_theta) {
      grid_draw_map_line(wx, wy, w2x, w2y, GRID_CRITICAL);
      grid_to_image(MIN(wx,w2x), MIN(wy,w2y), ABS(w2x-wx) + 1, ABS(w2y-wy) + 1);
      draw_grid(MIN(wx,w2x), MIN(wy,w2y), ABS(w2x-wx) + 1, ABS(w2y-wy) + 1);
    }
  }
}

static void get_critical_points() {

  int x, y, xi, yi, stop;
  double d, d2;

  printf("\n");
  for (x = 0; x < grid_width; x++) {
    printf("\rGetting critical points: %2d%% complete", (int) (100 * (x / (double) grid_width)));
    grid_to_image(x, 0, x+1, grid_height);
    draw_grid(x, 0, x+1, grid_height);
    for (y = 0; y < grid_height; y++) {
      if (grid[x][y] == GRID_VORONOI) {
	d = dist_to_wall(x, y);
	stop = 0;
	xi = yi = 0;
	for (xi = x - voronoi_local_min_filter_inner_radius;
	     !stop && xi <= x + voronoi_local_min_filter_inner_radius; xi++) {
	  for (yi = y - voronoi_local_min_filter_inner_radius;
	       !stop && yi <= y + voronoi_local_min_filter_inner_radius; yi++) {
	    if (!grid_oob(xi, yi)){
	      if (grid[xi][yi] == GRID_VORONOI) {
		d2 = dist_to_wall(xi, yi);
		if (d2 < d)
		  stop = 1;
	      }
	    }
	  }
	}
	if (stop)
	  continue;
	for (xi = x - voronoi_local_min_filter_outer_radius;
	     !stop && xi <= x + voronoi_local_min_filter_outer_radius; xi++) {
	  for (yi = y - voronoi_local_min_filter_outer_radius;
	       !stop && yi <= y + voronoi_local_min_filter_inner_radius; yi++) {
	    if (!grid_oob(xi, yi)){
	      if (grid[xi][yi] == GRID_VORONOI) {
		d2 = dist_to_wall(xi, yi);
		if (d2 - d > voronoi_local_min_filter_dist)
		  stop = 1;
	      }
	    }	    
	  }
	}
	if (stop)
	  draw_critical_line(x, y, d);
      }
    }
  }
  printf("\rGetting critical points: 100%% complete\n");
}
#endif

static void get_map_by_name(char *name) {

  carmen_map_placelist_t placelist;

  printf("get_map_by_name(%s)\n", name);  //dbug

  if (name) {
    if (carmen_map_get_gridmap_by_name(name, &map) < 0 || carmen_map_get_placelist_by_name(name, &placelist) < 0) {
      fprintf(stderr, "error getting map from mapserver\n");
      exit(0);
    }     
  }
  else {
    if (carmen_map_get_gridmap(&map) < 0 || carmen_map_get_placelist(&placelist) < 0) {
      fprintf(stderr, "error getting map from mapserver\n");
      exit(0);
    } 
  }
  
  if (placelist.num_places == 0) {
    fprintf(stderr, "no places found\n");
    exit(1);
  }

  grid_init();
  get_rooms(&placelist, get_doors(&placelist));
  print_doors();

  //cleanup_map();

  //get_voronoi();
  //get_critical_points();

  //dbug_func();

#ifndef NO_GRAPHICS
  grid_to_image(0, 0, grid_width, grid_height);
  draw_grid(0, 0, grid_width, grid_height);
#endif

}

static void get_map() {
  get_map_by_name(NULL);
}

static void get_maps() {

  carmen_hmap_t hmap;
  //carmen_door_p door;
  //carmen_map_point_t map_point;
  //carmen_world_point_t world_point;
  int i, j;

  if (carmen_map_get_hmap(&hmap) < 0 || hmap.num_zones == 0) {
    printf("not an hmap\n");  //dbug
    get_map();
    num_maps = 1;
    maps[0] = map;
    grids[0] = grid;
    return;
  }

  printf("this is an hmap\n");  //dbug

  // get maps
  for (i = 0; i < hmap.num_zones; i++)
    get_map_by_name(hmap.zone_names[i]);

  // get ceiling "doors"
  for (i = 0; i < hmap.num_inodes; i++) {
    if (hmap.inodes[i].type == CARMEN_HMAP_INODE_DOOR)
      printf("hmap inode %d is a DOOR\n", i+1);
    else {
      printf("hmap inode %d is an ELEVATOR\n", i+1);
      for (j = 0; j < hmap.inodes[i].degree; j++) {
	printf(" - map point %d: %s  %f %f %f\n", j+1, hmap.zone_names[hmap.inodes[i].keys[j]],
	       hmap.inodes[i].points[j].x, hmap.inodes[i].points[j].y, hmap.inodes[i].points[j].theta);
      }
    }


    /*************** old *****************

    printf("map point %d: (%.2f, %.2f, %.2f)\n", j, hmap.maps[i].points[j].pose.x,
	   hmap.maps[i].points[j].pose.y, hmap.maps[i].points[j].pose.theta);
    if (hmap.maps[i].points[j].up_map != NULL) {
      doors = (carmen_door_p) realloc(doors, (num_doors+1) * sizeof(carmen_door_t));
      carmen_test_alloc(doors);
      door = &doors[num_doors];
      door->num = num_doors;
      num_doors++;
      door->points.num_places = 0;
      door->width = 1.0;
      door->pose = hmap.maps[i].points[j].pose;
      map_switch(i);
      map_point.map = world_point.map = &maps[i];
      world_point.pose = hmap.maps[i].points[j].pose;
      carmen_world_to_map(&world_point, &map_point);
      door->room1 = closest_room(map_point.x, map_point.y, 10);
      if (door->room1 < 0)
	carmen_die("Error: map-switching point must be in a room: (%.2f, %.2f, %.2f)\n",
		   world_point.pose.x, world_point.pose.y, world_point.pose.theta);
      map_switch(hmap.maps[i].points[j].up_map->map_num);
      map_point.map = world_point.map = &maps[hmap.maps[i].points[j].up_map->map_num];
      world_point.pose = hmap.maps[i].points[j].up_pose;
      carmen_world_to_map(&world_point, &map_point);
      door->room2 = closest_room(map_point.x, map_point.y, 10);
      if (door->room2 < 0)
	carmen_die("Error: map-switching point must be in a room: (%.2f, %.2f, %.2f)\n",
		   world_point.pose.x, world_point.pose.y, world_point.pose.theta);
      rooms[door->room1].doors = (int *)
	realloc(rooms[door->room1].doors,
		(rooms[door->room1].num_doors + 1) * sizeof(int));
      carmen_test_alloc(rooms[door->room1].doors);
      rooms[door->room1].doors[rooms[door->room1].num_doors] = door->num;
      rooms[door->room1].num_doors++;
    }
    if (hmap.maps[i].points[j].down_map != NULL) {
      doors = (carmen_door_p) realloc(doors, (num_doors+1) * sizeof(carmen_door_t));
      carmen_test_alloc(doors);
      door = &doors[num_doors];
      door->num = num_doors;
      num_doors++;
      door->points.num_places = 0;
      door->width = -1.0;
      door->pose = hmap.maps[i].points[j].pose;
      map_switch(i);
      map_point.map = world_point.map = &maps[i];
      world_point.pose = hmap.maps[i].points[j].pose;
      carmen_world_to_map(&world_point, &map_point);
      door->room1 = closest_room(map_point.x, map_point.y, 10);
      if (door->room1 < 0)
	carmen_die("Error: map-switching point must be in a room: (%.2f, %.2f, %.2f)\n",
		   world_point.pose.x, world_point.pose.y, world_point.pose.theta);
      map_switch(hmap.maps[i].points[j].down_map->map_num);
      map_point.map = world_point.map =
	&maps[hmap.maps[i].points[j].down_map->map_num];
      world_point.pose = hmap.maps[i].points[j].down_pose;
      carmen_world_to_map(&world_point, &map_point);
      door->room2 = closest_room(map_point.x, map_point.y, 10);
      if (door->room2 < 0)
	carmen_die("Error: map-switching point must be in a room: (%.2f, %.2f, %.2f)\n",
		   world_point.pose.x, world_point.pose.y, world_point.pose.theta);
      rooms[door->room1].doors = (int *)
	realloc(rooms[door->room1].doors,
		(rooms[door->room1].num_doors + 1) * sizeof(int));
      carmen_test_alloc(rooms[door->room1].doors);
      rooms[door->room1].doors[rooms[door->room1].num_doors] = door->num;
      rooms[door->room1].num_doors++;
    }

    **********************/
  }

  //carmen_map_set_filename(hmap.maps[0].map_name);

  print_doors();
}

#ifndef NO_GRAPHICS

static GdkColor grid_color(int x, int y) {

  switch (grid[x][y]) {
  case GRID_NONE:      return carmen_white;
  case GRID_UNKNOWN:   return carmen_blue;
  case GRID_WALL:      return carmen_black;
  case GRID_OFFLIMITS:
  case GRID_DOOR:      return carmen_red;
  case GRID_PROBE:     return carmen_yellow;
  case GRID_VORONOI:   return carmen_orange;
  case GRID_CRITICAL:  return carmen_blue;
  }

  if (closest_room(x, y, 1) == room)
    return carmen_yellow;
  
  if (rooms[closest_room(x,y,1)].type == CARMEN_ROOM_TYPE_HALLWAY)
    return carmen_orange;

  return carmen_green;
}

static void grid_to_image(int x0, int y0, int width, int height) {

  int x, y, x2, y2, cx, cy, cw, ch;
  GdkColor color;

  if (x0 < 0 || x0+width > grid_width || y0 < 0 || y0+height > grid_height)
    return;

  cx = (int) ((x0 / (double) grid_width) * canvas_width);
  cy = (int) ((1.0 - ((y0+height) / (double) grid_height)) * canvas_height);
  cw = (int) ((width / (double) grid_width) * canvas_width);
  ch = (int) ((height / (double) grid_height) * canvas_height);

  for(x = cx; x < cx + cw; x++) {
    for(y = cy; y < cy + ch; y++) {
      x2 = (int) ((x / (double) canvas_width) * grid_width);
      y2 = (int) ((1.0 - ((y+1) / (double) canvas_height)) * grid_height);
      color = grid_color(x2, y2);
      gdk_gc_set_foreground(drawing_gc, &color);
      gdk_draw_point(pixmap, drawing_gc, x, y);
    }
  }
}

static void draw_canvas(int x, int y, int width, int height) {

  gdk_draw_pixmap(canvas->window,
		  canvas->style->fg_gc[GTK_WIDGET_STATE(canvas)],
		  pixmap, x, y, x, y, width, height);

  while(gtk_events_pending())
    gtk_main_iteration_do(TRUE);
}

static void draw_grid(int x, int y, int width, int height) {

  int cx, cy, cw, ch;

  if (x < 0 || x+width > grid_width || y < 0 || y+height > grid_height)
    return;

  cx = (int) ((x / (double) grid_width) * canvas_width);
  cy = (int) ((1.0 - ((y+height+1) / (double) grid_height)) * canvas_height);
  cw = (int) ((width / (double) grid_width) * canvas_width);
  ch = (int) ((height / (double) grid_height) * canvas_height);

  draw_canvas(cx, cy, cw, ch);
}

static void vector2d_shift(GdkPoint *dst, GdkPoint *src, int n, int x, int y) {

  int i;

  for (i = 0; i < n; i++) {
    dst[i].x = src[i].x + x;
    dst[i].y = src[i].y + y;
  }
}

/*
 * rotate n points of src theta radians about (x,y) and put result in dst.
 */
static void vector2d_rotate(GdkPoint *dst, GdkPoint *src, int n,
			    int x, int y, double theta) {

  int i, x2, y2;
  double cos_theta, sin_theta;

  cos_theta = cos(theta);
  sin_theta = sin(theta);

  for (i = 0; i < n; i++) {
    x2 = src[i].x - x;
    y2 = src[i].y - y;
    dst[i].x = x + cos_theta*x2 - sin_theta*y2;
    dst[i].y = y + sin_theta*x2 + cos_theta*y2;
  }
}

static void vector2d_scale(GdkPoint *dst, GdkPoint *src, int n, int x, int y,
			   int width_percent, int height_percent) {

  int i, x2, y2;

  for (i = 0; i < n; i++) {
    x2 = src[i].x - x;
    y2 = src[i].y - y;
    dst[i].x = x + x2*width_percent/100.0;
    dst[i].y = y + y2*height_percent/100.0;
  }
}

static void draw_arrow(int x, int y, double theta, int width, int height,
		       GdkColor color) {

  // default arrow shape with x = y = theta = 0, width = 100, & height = 100
  static const GdkPoint arrow_shape[7] = {{0,0}, {-40, -50}, {-30, -10}, {-100, -10},
					  {-100, 10}, {-30, 10}, {-40, 50}};
  GdkPoint arrow[7];
  int dim_x1, dim_y1, dim_x2, dim_y2, i;

  //printf("draw_arrow(%d, %d, %.2f, %d, %d, ...)\n", x, y, theta, width, height);
  
  vector2d_scale(arrow, (GdkPoint *) arrow_shape, 7, 0, 0, width, height);
  vector2d_rotate(arrow, arrow, 7, 0, 0, -theta);
  vector2d_shift(arrow, arrow, 7, (x / (double)grid_width) * canvas_width,
		 (1.0 - y / (double)grid_height) * canvas_height);

  gdk_gc_set_foreground(drawing_gc, &color);
  gdk_draw_polygon(pixmap, drawing_gc, 1, arrow, 7);

  dim_x1 = dim_x2 = arrow[0].x;
  dim_y1 = dim_y2 = arrow[0].y;

  for (i = 1; i < 7; i++) {
    if (arrow[i].x < dim_x1)
      dim_x1 = arrow[i].x;
    else if (arrow[i].x > dim_x2)
      dim_x2 = arrow[i].x;
    if (arrow[i].y < dim_y1)
      dim_y1 = arrow[i].y;
    else if (arrow[i].y > dim_y2)
      dim_y2 = arrow[i].y;
  }

  printf("dim = (%d, %d), (%d, %d)\n", dim_x1, dim_y1, dim_x2, dim_y2);
  printf("arrow = (%d, %d), (%d, %d), (%d, %d), (%d, %d), (%d, %d), (%d, %d), (%d, %d)\n",
	 arrow[0].x, arrow[0].y, arrow[1].x, arrow[1].y, arrow[2].x, arrow[2].y,
	 arrow[3].x, arrow[3].y, arrow[4].x, arrow[4].y, arrow[5].x, arrow[5].y,
	 arrow[6].x, arrow[6].y);

  draw_canvas(dim_x1 - 1, dim_y1 - 1, dim_x2 - dim_x1 + 2, dim_y2 - dim_y1 + 2);
}

static void erase_arrow(int x, int y, double theta, int width, int height) {

  // default arrow shape with x = y = theta = 0, width = 100, & height = 100
  static const GdkPoint arrow_shape[7] = {{0,0}, {-40, -50}, {-30, -10}, {-100, -10},
					  {-100, 10}, {-30, 10}, {-40, 50}};
  GdkPoint arrow[7];
  int dim_x1, dim_y1, dim_x2, dim_y2, i;
  int gx, gy, gw, gh;

  //printf("draw_arrow(%d, %d, %.2f, %d, %d, ...)\n", x, y, theta, width, height);
  
  vector2d_scale(arrow, (GdkPoint *) arrow_shape, 7, 0, 0, width, height);
  vector2d_rotate(arrow, arrow, 7, 0, 0, -theta);
  vector2d_shift(arrow, arrow, 7, (x / (double)grid_width) * canvas_width,
		 (1.0 - y / (double)grid_height) * canvas_height);

  dim_x1 = dim_x2 = arrow[0].x;
  dim_y1 = dim_y2 = arrow[0].y;

  for (i = 1; i < 7; i++) {
    if (arrow[i].x < dim_x1)
      dim_x1 = arrow[i].x;
    else if (arrow[i].x > dim_x2)
      dim_x2 = arrow[i].x;
    if (arrow[i].y < dim_y1)
      dim_y1 = arrow[i].y;
    else if (arrow[i].y > dim_y2)
      dim_y2 = arrow[i].y;
  }

  gx = (dim_x1 / (double) canvas_width) * grid_width;
  gy = (1.0 - dim_y2 / (double) canvas_height) * grid_height;
  gw = ((dim_x2 - dim_x1) / (double) canvas_width) * grid_width;
  gh = ((dim_y2 - dim_y1) / (double) canvas_height) * grid_height;

  grid_to_image(gx-1, gy-1, gw+2, gh+2);
  draw_grid(gx-1, gy-1, gw+2, gh+2);
}

static void draw_path() {

  carmen_world_point_t world_point;
  carmen_map_point_t map_point;
  int i, r1, r2;
  double theta;

  world_point.map = map_point.map = &map;

  r1 = r2 = room;
  theta = 0.0;

  for (i = 0; i < pathlen; i++) {
    r1 = r2;
    if (doors[path[i]].room1 == r1) {
      r2 = doors[path[i]].room2;
      theta = doors[path[i]].pose.theta;
    }
    else {
      r2 = doors[path[i]].room1;
      theta = carmen_normalize_theta(doors[path[i]].pose.theta + M_PI);
    }
    world_point.pose.x = doors[path[i]].pose.x;
    world_point.pose.y = doors[path[i]].pose.y;
    carmen_world_to_map(&world_point, &map_point);
    draw_arrow(map_point.x, map_point.y, theta, 20, 10, carmen_red);
  }
}

static void erase_path() {

  carmen_world_point_t world_point;
  carmen_map_point_t map_point;
  int i, r1, r2;
  double theta;

  world_point.map = map_point.map = &map;

  r1 = r2 = room;
  theta = 0.0;

  for (i = 0; i < pathlen; i++) {
    r1 = r2;
    if (doors[path[i]].room1 == r1) {
      r2 = doors[path[i]].room2;
      theta = doors[path[i]].pose.theta;
    }
    else {
      r2 = doors[path[i]].room1;
      theta = carmen_normalize_theta(doors[path[i]].pose.theta + M_PI);
    }
    world_point.pose.x = doors[path[i]].pose.x;
    world_point.pose.y = doors[path[i]].pose.y;
    carmen_world_to_map(&world_point, &map_point);
    erase_arrow(map_point.x, map_point.y, theta, 20, 10);
  }
}

static gint button_press_event(GtkWidget *widget __attribute__ ((unused)),
			       GdkEventMotion *event) {

  int x, y;

  x = (int) ((event->x / (double) canvas_width) * grid_width);
  y = (int) ((1.0 - ((event->y+1) / (double) canvas_height)) * grid_height);
  
  set_goal(closest_room(x,y,10));
  
  return TRUE;
}

static gint canvas_configure(GtkWidget *widget,
			     gpointer p __attribute__ ((unused))) {

  int display = (drawing_gc != NULL);

  canvas_width = widget->allocation.width;
  canvas_height = widget->allocation.height;

  if (pixmap)
    gdk_pixmap_unref(pixmap);

  pixmap = gdk_pixmap_new(canvas->window, canvas_width,
			  canvas_height, -1);

  if (display) {
    grid_to_image(0, 0, grid_width, grid_height);
    draw_grid(0, 0, grid_width, grid_height);
    draw_hallway_endpoints();
  }

  return TRUE;
}

static gint canvas_expose(GtkWidget *widget __attribute__ ((unused)),
			  GdkEventExpose *event) {

  int display = (drawing_gc != NULL);
  
  if (display) {
    //grid_to_image(0, 0, grid_width, grid_height);
    //draw_grid(0, 0, grid_width, grid_height);
    gdk_draw_pixmap(canvas->window,
		    canvas->style->fg_gc[GTK_WIDGET_STATE(canvas)],
		    pixmap, event->area.x, event->area.y,
		    event->area.x, event->area.y,
		    event->area.width, event->area.height);
  }

  return TRUE;
}

static void window_destroy(GtkWidget *w __attribute__ ((unused))) {

  gtk_main_quit();
}

static void gui_init() {

  GtkWidget *vbox;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_policy(GTK_WINDOW(window), TRUE, TRUE, FALSE);
  gtk_signal_connect(GTK_OBJECT(window), "destroy",
		     GTK_SIGNAL_FUNC(window_destroy), NULL);

  vbox = gtk_vbox_new(TRUE, 0);

  canvas = gtk_drawing_area_new();

  gtk_drawing_area_size(GTK_DRAWING_AREA(canvas), canvas_width, canvas_height);

  gtk_box_pack_start(GTK_BOX(vbox), canvas, TRUE, TRUE, 0);

  gtk_signal_connect(GTK_OBJECT(canvas), "expose_event",
		     GTK_SIGNAL_FUNC(canvas_expose), NULL);

  gtk_signal_connect(GTK_OBJECT(canvas), "configure_event",
		     GTK_SIGNAL_FUNC(canvas_configure), NULL);

  gtk_signal_connect(GTK_OBJECT(canvas), "button_press_event",
		     GTK_SIGNAL_FUNC(button_press_event), NULL);

  gtk_widget_add_events(canvas, GDK_BUTTON_PRESS_MASK);

  gtk_container_add(GTK_CONTAINER(window), vbox);

  gtk_widget_show_all(window);

  drawing_gc = gdk_gc_new(canvas->window);
  carmen_graphics_setup_colors();
}

#endif

static inline int same_room(int room1, int room2) {

  /*
  printf("same_room(%d, %d):\n", room1, room2);
  if (room1 != -1)
    printf(" - room1 name = %s\n", rooms[room1].name);
  if (room2 != -1)
    printf(" - room2 name = %s\n", rooms[room2].name);
  if (room1 != -1 && room2 != -1 && !strcmp(rooms[room1].name, rooms[room2].name))
    printf(" --> SAME_ROOM\n");
  else
    printf(" --> DIFFERENT ROOM\n");
  */
  return (room1 != -1 && room2 != -1 && !strcmp(rooms[room1].name, rooms[room2].name));
}

/*
static void floor_diff_x(int room1, int room2, int *rf) {

  int d, r, rd;

  if (rf[room2] != MAX_NUM_MAPS)
    return;

  for (rd = 0; rd < rooms[room1].num_doors; rd++) {
    d = rooms[room1].doors[rd];
    r = (doors[d].room1 == room1 ? doors[d].room2 : doors[d].room1);
    if (rf[r] != MAX_NUM_MAPS)  // room r is already marked
      continue;
    if (doors[d].points.num_places == 0) {
      if (doors[d].width > 0.0)  // ceiling
	rf[r] = rf[room1] + 1;
      else  // floor
	rf[r] = rf[room1] - 1;
    }
    else
      rf[r] = rf[room1];
    floor_diff_x(r, room2, rf);
  }
}
*/

/*
 * assumes floor consistency
 */
/*
static int floor_diff(int room1, int room2) {

  int *rf; // room floors
  int r, df;

  rf = (int *) calloc(num_rooms, sizeof(int));
  carmen_test_alloc(rf);

  for (r = 0; r < num_rooms; r++)
    rf[r] = MAX_NUM_MAPS;

  rf[room1] = 0;

  floor_diff_x(room1, room2, rf);
  df = rf[room2] - rf[room1];

  free(rf);

  return df;
}
*/

static int same_floor_x(int room1, int room2, int *rf) {

  int d, r, rd;

  //  printf("same floor(%s, %s) = ", rooms[room1].name, rooms[room2].name);

  for (rd = 0; rd < rooms[room1].num_doors; rd++) {
    d = rooms[room1].doors[rd];
    if (doors[d].points.num_places == 0)
      continue;
    r = (doors[d].room1 == room1 ? doors[d].room2 : doors[d].room1);
    if (r == room2)
      break;
    if (rf[r])  // room r is already marked
      continue;
    rf[r] = 1;
    if (same_floor_x(r, room2, rf))
      break;
  }  

  if (rd < rooms[room1].num_doors) {
    //printf("true\n");
    return 1;
  }

  //printf("false\n");
  return 0;
}

/*
static int same_floor(int room1, int room2) {

  int *rf; // room floors
  int r, retval;

  rf = (int *) calloc(num_rooms, sizeof(int));
  carmen_test_alloc(rf);

  for (r = 0; r < num_rooms; r++)
    rf[r] = 0;

  rf[room1] = 1;

  retval = same_floor_x(room1, room2, rf);

  free(rf);

  return retval;
}
*/

static int get_room_floor(int r) {

  int rd, d;

  for (rd = 0; rd < rooms[r].num_doors; rd++) {
    d = rooms[r].doors[rd];
    if (doors[d].points.num_places == 0 && doors[d].width < 0.0)
      return d;
  }

  return -1;
}

static int get_room_ceiling(int r) {

  int rd, d;

  for (rd = 0; rd < rooms[r].num_doors; rd++) {
    d = rooms[r].doors[rd];
    if (doors[d].points.num_places == 0 && doors[d].width > 0.0)
      return d;
  }

  return -1;
}

static double dist_to_goal(double x, double y) {

  int r, rd, d;
  double h, h2;

  h = dist(doors[rooms[goal].doors[0]].pose.x - x,
	   doors[rooms[goal].doors[0]].pose.y - y);

  for (r = 0; r < num_rooms; r++) {
    if (same_room(r, goal)) {
      for (rd = 0; rd < rooms[r].num_doors; rd++) {
	d = rooms[r].doors[rd];
	if (!same_room(doors[d].room1, doors[d].room2)) {
	  if (doors[d].points.num_places == 0)
	    continue;
	  h2 = dist(doors[d].pose.x - x, doors[d].pose.y - y);
	  if (h2 < h)
	    h = h2;
	}
      }
    }
  }

  return h;
}

/*
static void print_rf(int *rf) {

  int r;

  for (r = 0; r < num_rooms; r++)
    printf("  rf[ %s ] = %d\n", rooms[r].name, rf[r]);
}
*/

static double door_dist_to_goal(int door) {

  static int *ds = NULL, ds_len = 0, ds_size = 0;  // door stack (history)
  int ndpop;  // num doors to pop off door stack
  int r, d, dsd, *rf;
  double base_dist, min_dist, tmp_dist;

  //  printf("door_dist_to_goal(%s - %s)\n", rooms[doors[door].room1].name,
  //	 rooms[doors[door].room2].name);

  //getchar();

  //  printf(" - door stack =\n");
  //  for (dsd = 0; dsd < ds_len; dsd++)
  //    printf("     %s - %s\n", rooms[doors[dsd].room1].name, rooms[doors[dsd].room2].name);

  //getchar();

  ndpop = 0;

  min_dist = -1.0;
  base_dist = 0.0;
  
  if (doors[door].points.num_places == 0) {

    // add door to door stack
    if (ds_len >= ds_size) {
      ds_size += MAX_NUM_MAPS;
      ds = (int *) realloc(ds, ds_size * sizeof(int));
      carmen_test_alloc(ds);
    }
    ds[ds_len++] = door;
    ndpop++;

    base_dist = LEVEL_CHANGE_COST;
    door = (doors[door].width > 0.0 ? get_room_floor(doors[door].room2) : get_room_ceiling(doors[door].room2));
  }

  r = doors[door].room1;

  rf = (int *) calloc(num_rooms, sizeof(int));
  carmen_test_alloc(rf);
  memset(rf, 0, num_rooms*sizeof(int));
  rf[r] = 1;

  if (same_floor_x(r, goal, rf)) {
    free(rf);
    if (ndpop > 0) {
      ds_len -= ndpop;
      if (ds_len == 0) {
	free(ds);
	ds = NULL;
	ds_size = 0;
      }
    }
    min_dist = dist_to_goal(doors[door].pose.x, doors[door].pose.y);
    //    printf(" ---> returning min_dist = %.2f\n", min_dist);
    return min_dist;
  }

  //print_rf(rf);

  // add door to door stack
  if (ds_len >= ds_size) {
    ds_size += MAX_NUM_MAPS;
    ds = (int *) realloc(ds, ds_size * sizeof(int));
    carmen_test_alloc(ds);
  }
  ds[ds_len++] = door;
  ndpop++;

  // for each mlm point on this level
  for (d = 0; d < num_doors; d++) {
    //    printf("doors[%d].room1 = %s\n", d, rooms[doors[d].room1].name);
    if (doors[d].points.num_places == 0 && rf[doors[d].room1]) {
      //      printf(" *** door %d ( %s - %s ) is an elevator point ***\n", d, rooms[doors[d].room1].name, rooms[doors[d].room2].name); 
      for (dsd = 0; dsd < ds_len; dsd++)
	if (ds[dsd] == d)
	  break;
      if (dsd == ds_len) {  // haven't gone through this door before on this path
	tmp_dist = door_dist_to_goal(d);
	if (tmp_dist >= 0.0) {
	  tmp_dist += base_dist + dist(doors[door].pose.x - doors[d].pose.x, doors[door].pose.y - doors[d].pose.y);
	  if (min_dist < 0.0 || tmp_dist < min_dist)
	    min_dist = tmp_dist;
	}
      }
    }
  }

  free(rf);

  ds_len -= ndpop;
  if (ds_len == 0) {
    free(ds);
    ds = NULL;
    ds_size = 0;
  }

  //  printf(" ---> returning min_dist = %.2f\n", min_dist);

  return min_dist;
}

static double pq_f(path_node_p p) {

  double g, h;

  g = p->cost;
  h = door_dist_to_goal(p->path[p->pathlen-1]);

  return g+h;
}

static path_node_p pq_insert(path_node_p pq, path_node_p p) {

  path_node_p tmp;
  double fp;
  
  fp = pq_f(p);

  if (pq == NULL || fp <= pq_f(pq)) {
    p->next = pq;
    return p;
  }

  for (tmp = pq; tmp->next; tmp = tmp->next)
    if (fp <= pq_f(tmp->next))
      break;

  p->next = tmp->next;
  tmp->next = p;

  return pq;
}

static path_node_p pq_expand(path_node_p pq) {

  path_node_p p, tmp;
  int r, pd, rd, d, d1, d2;

  p = pq;
  pq = pq->next;

  r = -1;

  if (p->pathlen == 1) {
    if (room == doors[p->path[0]].room1)
      r = doors[p->path[0]].room2;
    else
      r = doors[p->path[0]].room1;
  }
  else {
    if (doors[p->path[p->pathlen-2]].room1 == doors[p->path[p->pathlen-1]].room1 ||
	doors[p->path[p->pathlen-2]].room2 == doors[p->path[p->pathlen-1]].room1)
      r = doors[p->path[p->pathlen-1]].room2;
    else
      r = doors[p->path[p->pathlen-1]].room1;
  }    

  for (rd = 0; rd < rooms[r].num_doors; rd++) {
    d = rooms[r].doors[rd];
    for (pd = 0; pd < p->pathlen; pd++)
      if (p->path[pd] == d)
	break;
    if (pd == p->pathlen) {
      tmp = (path_node_p) calloc(1, sizeof(path_node_t));
      carmen_test_alloc(tmp);
      tmp->pathlen = p->pathlen + 1;
      tmp->path = (int *) calloc(tmp->pathlen, sizeof(int));
      carmen_test_alloc(tmp->path);
      memcpy(tmp->path, p->path, p->pathlen * sizeof(int));
      tmp->path[tmp->pathlen-1] = d;
      d1 = tmp->path[tmp->pathlen-2];  // previous door
      if (doors[d1].points.num_places == 0) {
	d2 = (doors[d1].width > 0.0 ? get_room_floor(doors[d1].room2) : get_room_ceiling(doors[d1].room2));
	tmp->cost = p->cost + LEVEL_CHANGE_COST + dist(doors[d].pose.x - doors[d2].pose.x,
						       doors[d].pose.y - doors[d2].pose.y);
      }
      else
	tmp->cost = p->cost + dist(doors[d].pose.x - doors[p->path[p->pathlen-1]].pose.x,
				   doors[d].pose.y - doors[p->path[p->pathlen-1]].pose.y);
      pq = pq_insert(pq, tmp);
    }
  }

  return pq;
}

static inline int is_goal(path_node_p p) {

  return (same_room(doors[p->path[p->pathlen-1]].room1, goal) ||
	  same_room(doors[p->path[p->pathlen-1]].room2, goal));
}

static void pq_free(path_node_p pq) {

  if (pq == NULL)
    return;

  pq_free(pq->next);

  free(pq->path);
  free(pq);
}

static path_node_p pq_init() {

  path_node_p pq, tmp;
  int rd, d;

  pq = NULL;
  d = -1;

  for (rd = 0; rd < rooms[room].num_doors; rd++) {
    d = rooms[room].doors[rd];
    tmp = (path_node_p) calloc(1, sizeof(path_node_t));
    carmen_test_alloc(tmp);
    tmp->pathlen = 1;
    tmp->path = (int *) calloc(tmp->pathlen, sizeof(int));
    tmp->path[0] = d;
    if (doors[d].points.num_places == 0)  // ceiling
      tmp->cost = LEVEL_CHANGE_COST;
    else
      tmp->cost = dist(doors[d].pose.x - global_pos.globalpos.x,
		       doors[d].pose.y - global_pos.globalpos.y);
    pq = pq_insert(pq, tmp);
  }

  return pq;
}

static void print_path(int *p, int plen) {

  int i, r;

  r = room;

  printf("path = %d", r);
  for (i = 0; i < plen; i++) {
    r = (doors[p[i]].room1 == r ? doors[p[i]].room2 : doors[p[i]].room1);
    printf(" %d", r);
  }
  printf("\n");
}

#if 0
static void pq_print(path_node_p pq) {

  path_node_p tmp;
  int i;

  printf("\n");
  for (i = 1, tmp = pq; tmp; tmp = tmp->next, i++) {
    printf("%2d.  f-value: %.2f, ", i, pq_f(tmp));
    print_path(tmp->path, tmp->pathlen);
  }
  printf("\n");
}
#endif

static int path_eq(int *path1, int pathlen1, int *path2, int pathlen2) {

  int i;

  if (path1 == NULL)
    return (path2 == NULL);

  if (path2 == NULL)
    return (path1 == NULL);

  if (pathlen1 != pathlen2)
    return 0;

  for (i = 0; i < pathlen1; i++)
    if (path1[i] != path2[i])
      return 0;

  return 1;
}

/*
 * Performs A* search with current globalpos as the starting state
 * and doors as nodes.  Returns 1 if path changed; 0 otherwise.
 */
static int get_path() {

  path_node_p pq;  //path queue
  int changed = 0;

  if (same_room(goal, room) || room == -1) {
    changed = !path_eq(path, pathlen, NULL, 0);
#ifndef NO_GRAPHICS
    if (changed)
      erase_path();
#endif
    pathlen = 0;
    if (path)
      free(path);
    path = NULL;
    goal = -1;
    return changed;
  }

  for (pq = pq_init(); pq != NULL; pq = pq_expand(pq)) {
    //pq_print(pq);
    if (is_goal(pq)) {
      changed = !path_eq(path, pathlen, pq->path, pq->pathlen);
#ifndef NO_GRAPHICS
      if (changed)
	erase_path();
#endif
      pathlen = pq->pathlen;
      path = realloc(path, pathlen * sizeof(int));
      memcpy(path, pq->path, pathlen * sizeof(int));
      pq_free(pq);
#ifndef NO_GRAPHICS
      if (changed)
	draw_path();
#endif
      return changed;
    }
  }

  changed = !path_eq(path, pathlen, NULL, -1);
#ifndef NO_GRAPHICS
  if (changed)
    erase_path();
#endif
  pathlen = -1;
  if (path)
    free(path);
  path = NULL;
#ifndef NO_GRAPHICS
  if (changed)
    draw_path();
#endif
  return changed;
}

/*
 * assumes path != NULL
 */
static void publish_path_msg() {

  static carmen_roomnav_path_msg path_msg;
  static int first = 1;
  IPC_RETURN_TYPE err;

  //fprintf(stderr, "publish_path_msg()\n");

  if (first) {
    strcpy(path_msg.host, carmen_get_tenchar_host_name());
    path_msg.path = NULL;
    path_msg.pathlen = 0;
    first = 0;
  }

  path_msg.timestamp = carmen_get_time_ms();

  path_msg.pathlen = pathlen;

  //fprintf(stderr, "pathlen = %d\n", pathlen);

  print_path(path, pathlen);

  if (path != NULL) {
    path_msg.path = (int *) realloc(path_msg.path, pathlen * sizeof(int));
    carmen_test_alloc(path_msg.path);
    memcpy(path_msg.path, path, pathlen * sizeof(int));
  }
  else
    path_msg.path = NULL;

  err = IPC_publishData(CARMEN_ROOMNAV_PATH_MSG_NAME, &path_msg);
  carmen_test_ipc_exit(err, "Could not publish", CARMEN_ROOMNAV_PATH_MSG_NAME);  

  //printf("end publish_path_msg()\n");
}

static void update_path() {

  //printf("update_path()\n");

  if (goal < 0)
    return;

  if (get_path())
    publish_path_msg();

  //printf("end update_path()\n");
}

static void publish_room_msg() {

  static carmen_roomnav_room_msg room_msg;
  static int first = 1;
  IPC_RETURN_TYPE err;
  
  //printf("publish_room_msg()\n");

  if (first) {
    strcpy(room_msg.host, carmen_get_tenchar_host_name());
    first = 0;
  }

  room_msg.timestamp = carmen_get_time_ms();
  room_msg.room = room;

  err = IPC_publishData(CARMEN_ROOMNAV_ROOM_MSG_NAME, &room_msg);
  carmen_test_ipc_exit(err, "Could not publish", CARMEN_ROOMNAV_ROOM_MSG_NAME);  

  //printf("end publish_room_msg()\n");
}

static void roomnav_room_query_handler
(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
 void *clientData __attribute__ ((unused))) {

  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err;
  carmen_roomnav_room_msg *response;

  formatter = IPC_msgInstanceFormatter(msgRef);
  IPC_freeByteArray(callData);
  
  response = (carmen_roomnav_room_msg *)
    calloc(1, sizeof(carmen_roomnav_room_msg));
  carmen_test_alloc(response);

  response->timestamp = carmen_get_time_ms();
  strcpy(response->host, carmen_get_tenchar_host_name());

  response->room = room;

  err = IPC_respondData(msgRef, CARMEN_ROOMNAV_ROOM_MSG_NAME, response);
  carmen_test_ipc(err, "Could not respond",
		  CARMEN_ROOMNAV_ROOM_MSG_NAME);
}

static void roomnav_room_from_point_query_handler
(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
 void *clientData __attribute__ ((unused))) {

  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err;
  carmen_roomnav_room_from_point_query query;
  carmen_roomnav_room_msg *response;

  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &query,
			   sizeof(carmen_roomnav_room_from_point_query));
  IPC_freeByteArray(callData);
  
  response = (carmen_roomnav_room_msg *)
    calloc(1, sizeof(carmen_roomnav_room_msg));
  carmen_test_alloc(response);

  response->room = closest_room(query.point.x, query.point.y, 10);

  response->timestamp = carmen_get_time_ms();
  strcpy(response->host, carmen_get_tenchar_host_name());

  err = IPC_respondData(msgRef, CARMEN_ROOMNAV_ROOM_MSG_NAME, response);
  carmen_test_ipc(err, "Could not respond",
		  CARMEN_ROOMNAV_ROOM_MSG_NAME);
}

static void roomnav_room_from_world_point_query_handler
(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
 void *clientData __attribute__ ((unused))) {

  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err;
  carmen_roomnav_room_from_world_point_query query;
  carmen_roomnav_room_msg *response;
  carmen_world_point_t world_point;
  carmen_map_point_t map_point;


  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &query,
			   sizeof(carmen_roomnav_room_from_world_point_query));
  IPC_freeByteArray(callData);
  
  response = (carmen_roomnav_room_msg *)
    calloc(1, sizeof(carmen_roomnav_room_msg));
  carmen_test_alloc(response);

  world_point.map = map_point.map = &map;
  world_point.pose.x = query.point.x;
  world_point.pose.y = query.point.y;
  carmen_world_to_map(&world_point, &map_point);
  response->room = closest_room(map_point.x, map_point.y, 10);

  response->timestamp = carmen_get_time_ms();
  strcpy(response->host, carmen_get_tenchar_host_name());

  err = IPC_respondData(msgRef, CARMEN_ROOMNAV_ROOM_MSG_NAME, response);
  carmen_test_ipc(err, "Could not respond",
		  CARMEN_ROOMNAV_ROOM_MSG_NAME);
}

static void roomnav_goal_query_handler
(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
 void *clientData __attribute__ ((unused))) {

  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err;
  carmen_roomnav_goal_msg *response;

  formatter = IPC_msgInstanceFormatter(msgRef);
  IPC_freeByteArray(callData);
  
  response = (carmen_roomnav_goal_msg *)
    calloc(1, sizeof(carmen_roomnav_goal_msg));
  carmen_test_alloc(response);

  response->timestamp = carmen_get_time_ms();
  strcpy(response->host, carmen_get_tenchar_host_name());

  response->goal = goal;

  err = IPC_respondData(msgRef, CARMEN_ROOMNAV_GOAL_MSG_NAME, response);
  carmen_test_ipc(err, "Could not respond",
		  CARMEN_ROOMNAV_GOAL_MSG_NAME);
}

static void roomnav_path_query_handler
(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
 void *clientData __attribute__ ((unused))) {

  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err;
  carmen_roomnav_path_msg *response;

  formatter = IPC_msgInstanceFormatter(msgRef);
  IPC_freeByteArray(callData);
  
  response = (carmen_roomnav_path_msg *)
    calloc(1, sizeof(carmen_roomnav_path_msg));
  carmen_test_alloc(response);

  response->timestamp = carmen_get_time_ms();
  strcpy(response->host, carmen_get_tenchar_host_name());

  response->path = (int *) calloc(pathlen, sizeof(int));
  carmen_test_alloc(response->path);
  memcpy(response->path, path, pathlen * sizeof(int));
  response->pathlen = pathlen;

  err = IPC_respondData(msgRef, CARMEN_ROOMNAV_PATH_MSG_NAME, response);
  carmen_test_ipc(err, "Could not respond",
		  CARMEN_ROOMNAV_PATH_MSG_NAME);
}

static void roomnav_rooms_topology_query_handler
(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
 void *clientData __attribute__ ((unused))) {

  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err;
  carmen_roomnav_rooms_topology_msg *response;
  int i;

  formatter = IPC_msgInstanceFormatter(msgRef);
  IPC_freeByteArray(callData);

  response = (carmen_roomnav_rooms_topology_msg *)
    calloc(1, sizeof(carmen_roomnav_rooms_topology_msg));
  carmen_test_alloc(response);

  response->timestamp = carmen_get_time_ms();
  strcpy(response->host, carmen_get_tenchar_host_name());

  response->topology.rooms = (carmen_room_p)
    calloc(num_rooms, sizeof(carmen_room_t));
  carmen_test_alloc(response->topology.rooms);
  memcpy(response->topology.rooms, rooms, num_rooms * sizeof(carmen_room_t));
  response->topology.num_rooms = num_rooms;

  for (i = 0; i < num_rooms; i++) {
    response->topology.rooms[i].name = (char *) calloc(strlen(rooms[i].name) + 1,
						       sizeof(char));
    carmen_test_alloc(response->topology.rooms[i].name);
    strcpy(response->topology.rooms[i].name, rooms[i].name);
    response->topology.rooms[i].doors = (int *) calloc(rooms[i].num_doors, sizeof(int));
    carmen_test_alloc(response->topology.rooms[i].doors);
    memcpy(response->topology.rooms[i].doors, rooms[i].doors,
	   rooms[i].num_doors * sizeof(int));
  }

  response->topology.doors = (carmen_door_p) calloc(num_doors, sizeof(carmen_door_t));
  carmen_test_alloc(response->topology.doors);
  memcpy(response->topology.doors, doors, num_doors * sizeof(carmen_door_t));
  response->topology.num_doors = num_doors;

  for (i = 0; i < num_doors; i++) {
    response->topology.doors[i].points.places = (carmen_place_p)
      calloc(doors[i].points.num_places, sizeof(carmen_place_t));
    carmen_test_alloc(response->topology.doors[i].points.places);
    memcpy(response->topology.doors[i].points.places, doors[i].points.places,
 	   doors[i].points.num_places * sizeof(carmen_place_t));
  }

  err = IPC_respondData(msgRef, CARMEN_ROOMNAV_ROOMS_TOPOLOGY_MSG_NAME, response);
  carmen_test_ipc(err, "Could not respond",
		  CARMEN_ROOMNAV_ROOMS_TOPOLOGY_MSG_NAME);
}

static void publish_goal_changed_message() {

  IPC_RETURN_TYPE err = IPC_OK;
  carmen_roomnav_goal_changed_msg msg;

  msg.timestamp = carmen_get_time_ms();
  strcpy(msg.host, carmen_get_tenchar_host_name());

  msg.goal = goal;

  err = IPC_publishData(CARMEN_ROOMNAV_GOAL_CHANGED_MSG_NAME, &msg);
  carmen_test_ipc(err, "Could not publish", CARMEN_ROOMNAV_GOAL_CHANGED_MSG_NAME);
}

static void set_goal(int new_goal) {

  printf("new goal: (%d) - %s\n", new_goal, rooms[new_goal].name);

  goal = new_goal;
  publish_goal_changed_message();
  update_path();
}

static void roomnav_set_goal_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
				     void *clientData __attribute__ ((unused))) {

  carmen_roomnav_set_goal_msg goal_msg;
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &goal_msg,
			   sizeof(carmen_roomnav_set_goal_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall", IPC_msgInstanceName(msgRef));

  carmen_verbose("goal = %d\n", goal_msg.goal);

  set_goal(goal_msg.goal);
}

void map_update_handler() {

  int i;

  printf("map_update_handler()\n");

  printf("map.config.map_name = %s\n", map.config.map_name);

  for (i = 0; i < num_maps; i++)
    if (!strcmp(maps[i].config.map_name, map.config.map_name))
      break;

  // map found
  if (i < num_maps)
    map_switch(i);

  // new map
  else {
    get_map();
    maps[num_maps] = map;
    grids[num_maps] = grid;
    num_maps++;
    loading_map = 0;
  }
}

static void log_room() {

  printf("log_room()\n");

  if (roomnav_logfile_fp != NULL)
    fprintf(roomnav_logfile_fp, "ROOM %d \"%s\" %.2f\n",
	    room, room < 0 ? "NA" : rooms[room].name, carmen_get_time_ms());
}

void localize_handler() {

  carmen_world_point_t world_point;
  carmen_map_point_t map_point;
  int new_room;

  //printf("localize_handler()\n");

  world_point.map = map_point.map = &map;
  world_point.pose.x = global_pos.globalpos.x;
  world_point.pose.y = global_pos.globalpos.y;
  carmen_world_to_map(&world_point, &map_point);

  new_room = closest_room(map_point.x, map_point.y, 10);
  
  if (new_room != room) {
    room = new_room;
    printf("room = %d\n", room);
    log_room();

#ifndef NO_GRAPHICS
    grid_to_image(0, 0, grid_width, grid_height);
    draw_grid(0, 0, grid_width, grid_height);
#endif

    publish_room_msg();
  }

  update_path();

  //printf("end localize_handler()\n");
}

static void ipc_init() {

  IPC_RETURN_TYPE err;

  err = IPC_defineMsg(CARMEN_ROOMNAV_ROOM_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_ROOMNAV_ROOM_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_ROOMNAV_ROOM_MSG_NAME);

  err = IPC_defineMsg(CARMEN_ROOMNAV_ROOM_QUERY_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_ROOMNAV_ROOM_QUERY_FMT);
  carmen_test_ipc_exit(err, "Could not define", 
		       CARMEN_ROOMNAV_ROOM_QUERY_NAME);

  err = IPC_defineMsg(CARMEN_ROOMNAV_ROOM_FROM_POINT_QUERY_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_ROOMNAV_ROOM_FROM_POINT_QUERY_FMT);
  carmen_test_ipc_exit(err, "Could not define", 
		       CARMEN_ROOMNAV_ROOM_FROM_POINT_QUERY_NAME);

  err = IPC_defineMsg(CARMEN_ROOMNAV_ROOM_FROM_WORLD_POINT_QUERY_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_ROOMNAV_ROOM_FROM_WORLD_POINT_QUERY_FMT);
  carmen_test_ipc_exit(err, "Could not define", 
		       CARMEN_ROOMNAV_ROOM_FROM_WORLD_POINT_QUERY_NAME);

  err = IPC_defineMsg(CARMEN_ROOMNAV_GOAL_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_ROOMNAV_GOAL_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_ROOMNAV_GOAL_MSG_NAME);

  err = IPC_defineMsg(CARMEN_ROOMNAV_GOAL_QUERY_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_ROOMNAV_GOAL_QUERY_FMT);
  carmen_test_ipc_exit(err, "Could not define", 
		       CARMEN_ROOMNAV_GOAL_QUERY_NAME);

  err = IPC_defineMsg(CARMEN_ROOMNAV_GOAL_CHANGED_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_ROOMNAV_GOAL_CHANGED_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_ROOMNAV_GOAL_CHANGED_MSG_NAME);

  err = IPC_defineMsg(CARMEN_ROOMNAV_PATH_QUERY_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_ROOMNAV_PATH_QUERY_FMT);
  carmen_test_ipc_exit(err, "Could not define", 
		       CARMEN_ROOMNAV_PATH_QUERY_NAME);

  err = IPC_defineMsg(CARMEN_ROOMNAV_ROOMS_TOPOLOGY_QUERY_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_ROOMNAV_ROOMS_TOPOLOGY_QUERY_FMT);
  carmen_test_ipc_exit(err, "Could not define", 
		       CARMEN_ROOMNAV_ROOMS_TOPOLOGY_QUERY_NAME);

  err = IPC_defineMsg(CARMEN_ROOMNAV_ROOMS_TOPOLOGY_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_ROOMNAV_ROOMS_TOPOLOGY_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define",
		       CARMEN_ROOMNAV_ROOMS_TOPOLOGY_MSG_NAME);

  err = IPC_defineMsg(CARMEN_ROOMNAV_SET_GOAL_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_ROOMNAV_SET_GOAL_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_ROOMNAV_SET_GOAL_MSG_NAME);

  err = IPC_defineMsg(CARMEN_ROOMNAV_PATH_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_ROOMNAV_PATH_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_ROOMNAV_PATH_MSG_NAME);

  carmen_localize_subscribe_globalpos_message(&global_pos, localize_handler,
					      CARMEN_SUBSCRIBE_LATEST);

  err = IPC_subscribe(CARMEN_ROOMNAV_ROOM_QUERY_NAME, 
		      roomnav_room_query_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subcribe", 
		       CARMEN_ROOMNAV_ROOM_QUERY_NAME);
  IPC_setMsgQueueLength(CARMEN_ROOMNAV_ROOM_QUERY_NAME, 100);

  err = IPC_subscribe(CARMEN_ROOMNAV_ROOM_FROM_POINT_QUERY_NAME, 
		      roomnav_room_from_point_query_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subcribe", 
		       CARMEN_ROOMNAV_ROOM_FROM_POINT_QUERY_NAME);
  IPC_setMsgQueueLength(CARMEN_ROOMNAV_ROOM_FROM_POINT_QUERY_NAME, 100);

  err = IPC_subscribe(CARMEN_ROOMNAV_ROOM_FROM_WORLD_POINT_QUERY_NAME, 
		      roomnav_room_from_world_point_query_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subcribe", 
		       CARMEN_ROOMNAV_ROOM_FROM_WORLD_POINT_QUERY_NAME);
  IPC_setMsgQueueLength(CARMEN_ROOMNAV_ROOM_FROM_WORLD_POINT_QUERY_NAME, 100);

  err = IPC_subscribe(CARMEN_ROOMNAV_GOAL_QUERY_NAME, 
		      roomnav_goal_query_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subcribe", 
		       CARMEN_ROOMNAV_GOAL_QUERY_NAME);
  IPC_setMsgQueueLength(CARMEN_ROOMNAV_GOAL_QUERY_NAME, 100);

  err = IPC_subscribe(CARMEN_ROOMNAV_PATH_QUERY_NAME, 
		      roomnav_path_query_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subcribe", 
		       CARMEN_ROOMNAV_PATH_QUERY_NAME);
  IPC_setMsgQueueLength(CARMEN_ROOMNAV_PATH_QUERY_NAME, 100);

  err = IPC_subscribe(CARMEN_ROOMNAV_ROOMS_TOPOLOGY_QUERY_NAME, 
		      roomnav_rooms_topology_query_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subcribe", 
		       CARMEN_ROOMNAV_ROOMS_TOPOLOGY_QUERY_NAME);
  IPC_setMsgQueueLength(CARMEN_ROOMNAV_ROOMS_TOPOLOGY_QUERY_NAME, 100);

  err = IPC_subscribe(CARMEN_ROOMNAV_SET_GOAL_MSG_NAME, roomnav_set_goal_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subcribe", CARMEN_ROOMNAV_SET_GOAL_MSG_NAME);
  IPC_setMsgQueueLength(CARMEN_ROOMNAV_SET_GOAL_MSG_NAME, 100);

  carmen_map_subscribe_gridmap_update_message(&map, map_update_handler,
					      CARMEN_SUBSCRIBE_LATEST);
}

#ifndef NO_GRAPHICS

static gint updateIPC(gpointer *data __attribute__ ((unused))) {

  sleep_ipc(0.01);
  carmen_graphics_update_ipc_callbacks((GdkInputFunction) updateIPC);

  return 1;
}

#endif

static void params_init(int argc __attribute__ ((unused)),
			char *argv[] __attribute__ ((unused))) {

  grid_resolution = DEFAULT_GRID_RESOLUTION;
}

void shutdown_module(int sig) {

  if (sig == SIGINT) {
    if (roomnav_logfile_fp != NULL)
      fclose(roomnav_logfile_fp);
    close_ipc();
    fprintf(stderr, "\nDisconnecting.\n");
    exit(0);
  }
}

int main(int argc, char *argv[]) {

  int i, mlm;

  mlm = 0;
  for (i = 1; i < argc; i++) {
    if (!fast && !strcmp(argv[i], "-fast"))
      fast = 1;
    //    else if (!mlm && !strcmp(carmen_file_extension(argv[i]), ".mlm"))
    //      mlm = i;
    else if (i != argc - 1 || (roomnav_logfile_fp = fopen(argv[i], "w")) == NULL)
      carmen_die("usage:  %s [-fast] [mlm file] [logfile]\n", argv[0]);
  }

  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);

  signal(SIGINT, shutdown_module);

#ifndef NO_GRAPHICS
  gtk_init(&argc, &argv);
#endif

  params_init(argc, argv);

#ifndef NO_GRAPHICS
  gui_init();
#endif

  ipc_init();

  if (mlm)
    get_maps(argv[mlm]);
  else
    get_maps(NULL);

#ifndef NO_GRAPHICS
  carmen_graphics_update_ipc_callbacks((GdkInputFunction) updateIPC);
  gtk_main();
#else
  IPC_dispatch();
#endif

  return 0;
}
