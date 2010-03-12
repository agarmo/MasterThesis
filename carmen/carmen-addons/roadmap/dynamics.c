#include <carmen/carmen.h>
#include <carmen/dot.h>
#include <carmen/dot_interface.h>
#include <carmen/dot_messages.h>
#include <assert.h>

#include "roadmap.h"
#include "dynamics.h"

static carmen_list_t *people = NULL, *trash = NULL, *doors = NULL;
static carmen_map_t *map;
static double robot_width;

static carmen_list_t *marked_edges = NULL;

static carmen_map_t *map;

static void people_handler(carmen_dot_all_people_msg *people_msg)
{
  int i;

  people->length = 0;
  for (i = 0; i < people_msg->num_people; i++) 
    carmen_list_add(people, people_msg->people+i);  
}

static void trash_handler(carmen_dot_all_trash_msg *trash_msg)
{
  int i;

  trash->length = 0;
  for (i = 0; i < trash_msg->num_trash; i++) 
    carmen_list_add(trash, trash_msg->trash+i);
}

static void doors_handler(carmen_dot_all_doors_msg *door_msg)
{
  int i;

  doors->length = 0;
  for (i = 0; i < door_msg->num_doors; i++) 
    carmen_list_add(doors, door_msg->doors+i);
}

void carmen_dynamics_initialize(carmen_map_t *new_map)
{
  map = new_map;

  people = carmen_list_create(sizeof(carmen_dot_person_t), 10);
  trash = carmen_list_create(sizeof(carmen_dot_trash_t), 10);
  doors = carmen_list_create(sizeof(carmen_dot_door_t), 10);

  carmen_dot_subscribe_all_people_message
    (NULL, (carmen_handler_t) people_handler,CARMEN_SUBSCRIBE_LATEST);
  carmen_dot_subscribe_all_trash_message
    (NULL, (carmen_handler_t) trash_handler, CARMEN_SUBSCRIBE_LATEST);
  carmen_dot_subscribe_all_doors_message
    (NULL, (carmen_handler_t) doors_handler,CARMEN_SUBSCRIBE_LATEST);

  if (carmen_param_get_double("width",&robot_width) < 0) {
    robot_width = .5;
    carmen_warn("Could not set robot speed. Setting to default %f\n",
		robot_width);
  }

  marked_edges = carmen_list_create(sizeof(carmen_roadmap_marked_edge_t), 10);
}

void carmen_dynamics_initialize_no_ipc(carmen_map_t *new_map)
{
  map = new_map;
  people = carmen_list_create(sizeof(carmen_dot_person_t), 10);
  trash = carmen_list_create(sizeof(carmen_dot_trash_t), 10);
  doors = carmen_list_create(sizeof(carmen_dot_door_t), 10); 

  marked_edges = carmen_list_create(sizeof(carmen_roadmap_marked_edge_t), 10);
}

void carmen_dynamics_update_person(carmen_dot_person_t *update_person)
{
  int i;
  carmen_dot_person_t *person;

  for (i = 0; i < people->length; i++) {
    person = (carmen_dot_person_t *)carmen_list_get(people, i);
    if (person->id == update_person->id) {
      *person = *update_person;
      return;
    }
  }

  carmen_list_add(people, update_person);
}


void coord_shift(double *x, double *y, double theta, 
			  double delta_x, double delta_y)
{
  *x -= delta_x;
  *y -= delta_y;
  
  *x = cos(theta) * *x + sin(theta) * *y;
  *y = -sin(theta) * *x + cos(theta) * *y;
}

#if 0
static int is_blocked(carmen_roadmap_vertex_t *n1, carmen_roadmap_vertex_t *n2,
		      double x, double y, double vx, double vxy, double vy)
{
  double e1, e2;
  double ex, ey;
  double x1, x2, y1, y2;
  double root1, root2;
  double root1y, root2y;
  double m, b;
  double d, e, f;
  double theta;
  double dist_along_line1, dist_along_line2;

  x1 = n1->x;
  y1 = n1->y;
  x2 = n2->x;
  y2 = n2->y;  

  e1 = (vx + vy)/2.0 + sqrt(4*vxy*vxy + (vx-vy)*(vx-vy))/2.0;
  e2 = (vx + vy)/2.0 - sqrt(4*vxy*vxy + (vx-vy)*(vx-vy))/2.0;
  ex = vxy;
  ey = e1 - vx;

  theta = atan2(ey, ex);

  e1 = 3*sqrt(e1);
  e2 = 3*sqrt(e2);

  coord_shift(&x1, &y1, -theta, x, y);
  coord_shift(&x2, &y2, -theta, x, y);

  m = (y2-y1)/(x2-x1);
  b = m*x2+y2;

  d = e2*e2+e1*e1*m*m;
  e = e1*e1*2*m*b;
  f = e1*e1*b*b-e1*e1*e2*e2;

  if (e*e-4*d*f <= 0)
    return 0;

  root1 = (-e + sqrt(e*e-4*d*f)) / 2*d;
  root2 = (-e - sqrt(e*e-4*d*f)) / 2*d;

  root1y = m * root1 + b;
  root2y = m * root2 + b;

  dist_along_line1 = (root1 - x1) / (x2 - x1);
  dist_along_line2 = (root2 - x1) / (x2 - x1);

  if ((dist_along_line1 > 1 && dist_along_line2 > 1) || 
      (dist_along_line1 < 0 && dist_along_line2 < 0)) {
      return 0;
  }

  return 1;
}

#endif

static inline int ray_intersect_arg(double rx, double ry, double rtheta,
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
				double cx, double cy, double cr) {

  double epsilon = 0.000001;
  double a, b, c, tan_rtheta;
  double x1, y1, x2, y2;

  x1 = x2 = y1 = y2 = 0.0;

  if (fabs(cos(rtheta)) < epsilon) {
    if (cr*cr - (rx-cx)*(rx-cx) < 0.0)
      return 0;
    x1 = x2 = rx;
    y1 = cy + sqrt(cr*cr-(rx-cx)*(rx-cx));
    y2 = cy - sqrt(cr*cr-(rx-cx)*(rx-cx));
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
  }

  if (ray_intersect_arg(rx, ry, rtheta, x1, y1, x2, y2))
    return 1;

  return 0;
}

static int segment_intersect_circle(double n1x, double n1y, double n2x, double n2y, 
				    double x, double y, double r)
{
  double theta1, theta2;

  theta1 = atan2(n2y - n1y, n2x - n1x);
  theta2 = carmen_normalize_theta(theta1 + M_PI);

  if (ray_intersect_circle(n1x, n1y, theta1, x, y, r) &&
      ray_intersect_circle(n2x, n2y, theta2, x, y, r))
    return 1;

  return 0;
}

static int ray_intersect_convex_polygon(double rx, double ry, double rtheta,
					double *xpoly, double *ypoly, int np)
{
  int i;
  double x, y, theta;
  double a, b, c, d;

  a = tan(rtheta);
  b = ry - a*rx;

  for (i = 0; i < np; i++) {
    c = (ypoly[(i+1)%np] - ypoly[i]) / (xpoly[(i+1)%np] - xpoly[i]);
    d = ypoly[i] - c*xpoly[i];
    x = (d - b) / (a - c);
    y = a*x + b;
    theta = atan2(ypoly[(i+1)%np] - ypoly[i], xpoly[(i+1)%np] - xpoly[i]);
    if (ray_intersect_arg(xpoly[i], ypoly[i], theta, x, y, x, y) &&
	ray_intersect_arg(xpoly[(i+1)%np], ypoly[(i+1)%np],
			  carmen_normalize_theta(theta+M_PI), x, y, x, y) &&
	ray_intersect_arg(rx, ry, rtheta, x, y, x, y))
      return 1;
  }

  return 0;
}

static int segment_intersect_convex_polygon(double n1x, double n1y, double n2x, double n2y, 
					    double *xhull, double *yhull, int hull_size)
{
  int i;
  double theta1, theta2;
  double xmin, xmax, ymin, ymax;

  xmin = xmax = xhull[0];
  ymin = ymax = yhull[0];
  for (i = 1; i < hull_size; i++) {
    if (xhull[i] < xmin)
      xmin = xhull[i];
    else if (xhull[i] > xmax)
      xmax = xhull[i];
    if (yhull[i] < ymin)
      ymin = yhull[i];
    else if (yhull[i] > ymax)
      ymax = yhull[i];
  }

  if ((n1x < xmin && n2x < xmin) || (n1x > xmax && n2x > xmax) ||
      (n1y < ymin && n2y < ymin) || (n1y > ymax && n2y > ymax))
    return 0;

  theta1 = atan2(n2y - n1y, n2x - n1x);
  theta2 = carmen_normalize_theta(theta1 + M_PI);
  if (ray_intersect_convex_polygon(n1x, n1y, theta1, xhull, yhull, hull_size) &&
      ray_intersect_convex_polygon(n2x, n2y, theta2, xhull, yhull, hull_size))
    return 1;

  return 0;  
}

static int is_blocked_by_person(double n1x, double n1y, double n2x, double n2y, 
				double x, double y, double r)
{
  double theta, dx, dy;

  theta = carmen_normalize_theta(atan2(n2y - n1y, n2x - n1x) + M_PI/2.0);
  dx = (robot_width/2.0)*cos(theta);
  dy = (robot_width/2.0)*sin(theta);

  if (segment_intersect_circle(n1x+dx, n1y+dy, n2x+dx, n2y+dy, x, y, r) ||
      segment_intersect_circle(n1x, n1y, n2x, n2y, x, y, r) ||
      segment_intersect_circle(n1x-dx, n1y-dy, n2x-dx, n2y-dy, x, y, r))
    return 1;

  return 0;  
}

static int is_blocked_by_trash(double n1x, double n1y, double n2x, double n2y, 
			       double *xhull, double *yhull, int hull_size)
{
  double theta, dx, dy;

  theta = carmen_normalize_theta(atan2(n2y - n1y, n2x - n1x) + M_PI/2.0);
  dx = (robot_width/2.0)*cos(theta);
  dy = (robot_width/2.0)*sin(theta);

  if (segment_intersect_convex_polygon(n1x+dx, n1y+dy, n2x+dx, n2y+dy, xhull, yhull, hull_size) ||
      segment_intersect_convex_polygon(n1x, n1y, n2x, n2y, xhull, yhull, hull_size) ||
      segment_intersect_convex_polygon(n1x-dx, n1y-dy, n2x-dx, n2y-dy, xhull, yhull, hull_size))
    return 1;

  return 0;  
}

static int is_blocked(double n1x, double n1y, double n2x, double n2y, 
		      double x, double y, double vx, double vxy, double vy)
{
  double numerator;
  double denominator;
  double i_x, i_y;
  double dist_along_line;
  double radius;
  double theta;

  carmen_roadmap_refine_get_radius(vx, vy, vxy, &radius);

  numerator = (n2x - n1x)*(n1y - y) - (n1x - x)*(n2y - n1y);
  denominator = hypot(n2x-n1x, n2y-n1y);
  
  if (fabs(denominator) < 1e-9 || fabs(numerator)/denominator > radius) 
    return 0;

  theta = fabs(atan2(n2y-n1y, n2x - n1x));
  if (theta > M_PI/4 && theta < 3*M_PI/4) {
    i_y = numerator/denominator * (n2x - n1x)/denominator + y;
    dist_along_line = (i_y - n1y) / (n2y - n1y);
  } else {
    i_x = numerator/denominator * (n2y - n1y)/denominator + x;
    dist_along_line = (i_x - n1x) / (n2x - n1x);
  }

  if (dist_along_line > 1 || dist_along_line < 0) {
    if (hypot(n2x - x, n2y - y) > radius && 
	hypot(n1x - x, n1y - y) > radius)
      return 0;
  }

  return 1;
}

static void fill_in_object(carmen_dot_t *blocking_object, void *data, 
			   carmen_dot_enum_t type)
{
  blocking_object->dot_type = type;
  if (type == carmen_dot_person) 
    blocking_object->data.person = *(carmen_dot_person_t *)data;
  else if (type == carmen_dot_trash) 
    blocking_object->data.trash = *(carmen_dot_trash_t *)data;
  else if (type == carmen_dot_door) 
    blocking_object->data.door = *(carmen_dot_door_t *)data;
}

static int do_blocking(double n1x, double n1y, double n2x, double n2y, 
		       carmen_dot_t *blocking_object, int avoid_people)
{
  carmen_dot_person_t *person;
  carmen_dot_trash_t *trash_bin;
  carmen_dot_door_t *door;

  int i;

  if (avoid_people) {
    for (i = 0; i < people->length; i++) {
      person = (carmen_dot_person_t *)carmen_list_get(people, i);
      if (is_blocked_by_person(n1x, n1y, n2x, n2y, person->x, person->y, person->r)) {
	if (blocking_object) 
	  fill_in_object(blocking_object, person, carmen_dot_person);
	return 1;
      }
    }
  }

  for (i = 0; i < trash->length; i++) {
    trash_bin = (carmen_dot_trash_t *)carmen_list_get(trash, i);
    if (is_blocked_by_trash(n1x, n1y, n2x, n2y, trash_bin->xhull,
			    trash_bin->yhull, trash_bin->hull_size)) {
      if (blocking_object) 
	fill_in_object(blocking_object, trash_bin, carmen_dot_trash);
      return 1;
    }
  }

  for (i = 0; i < doors->length; i++) {
    door = (carmen_dot_door_t *)carmen_list_get(doors, i);
    if (is_blocked(n1x, n1y, n2x, n2y, door->x, door->y, door->vx, 
		   door->vxy, door->vy)) {
      if (blocking_object) 
	fill_in_object(blocking_object, door, carmen_dot_door);
      return 1;
    }
  }

  return 0;
}

int carmen_dynamics_test_for_block(carmen_roadmap_vertex_t *n1, 
				   carmen_roadmap_vertex_t *n2,
				   int avoid_people)
{
  carmen_world_point_t pt1, pt2;
  carmen_map_point_t map_pt;

  map_pt.x = n1->x;
  map_pt.y = n1->y;
  map_pt.map = map;
  carmen_map_to_world(&map_pt, &pt1);

  map_pt.x = n2->x;
  map_pt.y = n2->y;
  carmen_map_to_world(&map_pt, &pt2);


  return do_blocking(pt1.pose.x, pt1.pose.y, pt2.pose.x, pt2.pose.y, 
		     NULL, avoid_people);
}

int carmen_dynamics_test_point_for_block(carmen_roadmap_vertex_t *n, 
					 carmen_world_point_t *point,
					 int avoid_people)
{
  carmen_world_point_t world_pt;
  carmen_map_point_t map_pt;

  map_pt.x = n->x;
  map_pt.y = n->y;
  map_pt.map = map;
  carmen_map_to_world(&map_pt, &world_pt);

  return do_blocking(world_pt.pose.x, world_pt.pose.y, 
		     point->pose.x, point->pose.y, NULL, avoid_people);  
}

int carmen_dynamics_get_block(carmen_roadmap_vertex_t *n1, 
			      carmen_roadmap_vertex_t *n2,
			      carmen_dot_t *blocking_object,
			      int avoid_people)
{
  carmen_world_point_t pt1, pt2;
  carmen_map_point_t map_pt;
  
  map_pt.x = n1->x;
  map_pt.y = n1->y;
  map_pt.map = map;
  carmen_map_to_world(&map_pt, &pt1);

  map_pt.x = n2->x;
  map_pt.y = n2->y;
  carmen_map_to_world(&map_pt, &pt2);

  return do_blocking(pt1.pose.x, pt1.pose.y, pt2.pose.x, pt2.pose.y, 
		     blocking_object, avoid_people);
}

static int is_too_close(double nx, double ny, double x, double y, 
			double vx, double vxy, double vy)
{
  double radius;
  double e1, e2;

  e1 = (vx + vy)/2.0 + sqrt(4*vxy*vxy + (vx-vy)*(vx-vy))/2.0;
  e2 = (vx + vy)/2.0 - sqrt(4*vxy*vxy + (vx-vy)*(vx-vy))/2.0;

  e1 = sqrt(e1);
  e2 = sqrt(e2);

  if (e1 < 1)
    e1 = .5;
  if (e2 < 1)
    e2 = .5;

  radius = (e1 + e2) / 2;

  if (hypot(nx-x, ny-y) > radius) 
    return 0;

  return 1;
}


int carmen_dynamics_test_node(carmen_roadmap_vertex_t *n1, int avoid_people)
{
  carmen_dot_person_t *person;
  carmen_dot_trash_t *trash_bin;
  carmen_dot_door_t *door;
  carmen_world_point_t world_pt;
  carmen_map_point_t map_pt;

  int i;

  map_pt.x = n1->x;
  map_pt.y = n1->y;
  map_pt.map = map;
  carmen_map_to_world(&map_pt, &world_pt);

  if (avoid_people) {
    for (i = 0; i < people->length; i++) {
      person = (carmen_dot_person_t *)carmen_list_get(people, i);
      if (is_too_close(world_pt.pose.x, world_pt.pose.y, person->x, person->y, 
		       person->vx, person->vxy, person->vy))
	return 1;
    }
  }

  for (i = 0; i < trash->length; i++) {
    trash_bin = (carmen_dot_trash_t *)carmen_list_get(trash, i);
    if (is_too_close(world_pt.pose.x, world_pt.pose.y, trash_bin->x, 
		     trash_bin->y, trash_bin->vx, trash_bin->vxy, 
		     trash_bin->vy))
      return 1;
  }

  for (i = 0; i < doors->length; i++) {
    door = (carmen_dot_door_t *)carmen_list_get(doors, i);
    if (is_too_close(world_pt.pose.x, world_pt.pose.y, door->x, door->y, 
		     door->vx, door->vxy, door->vy))
      return 1;
  }

  return 0;
}

int carmen_dynamics_find_edge_and_block(carmen_roadmap_vertex_t *parent_node, 
					int child_id)
{
  carmen_roadmap_edge_t *edges;
  int length;
  int j;

  length = parent_node->edges->length;
  edges = (carmen_roadmap_edge_t *)
    parent_node->edges->list;
  for (j = 0; j < length; j++) {
    if (edges[j].id == child_id)
      break;
  }
  assert (j < length);
  edges[j].blocked = 1;

  return j;
}


void carmen_dynamics_mark_blocked(int node1_id, int edge1_id, 
				  int node2_id, int edge2_id)
{
  carmen_roadmap_marked_edge_t marked_edge;

  marked_edge.n1 = node1_id;
  marked_edge.e1 = edge1_id;
  marked_edge.n2 = node2_id;
  marked_edge.e2 = edge2_id;

  //  carmen_warn("Adding %d %d %d %d\n", node1_id, edge1_id, node2_id, edge2_id);

  carmen_list_add(marked_edges, &marked_edge);
}

void carmen_dynamics_clear_all_blocked(carmen_roadmap_t *roadmap)
{  
  carmen_roadmap_marked_edge_t *edge_list;
  carmen_roadmap_vertex_t *node_list;
  int i;
  int n1, e1, n2, e2;

  if (!marked_edges || !marked_edges->list)
    return;

  edge_list = (carmen_roadmap_marked_edge_t *)marked_edges->list;
  node_list = (carmen_roadmap_vertex_t *)roadmap->nodes->list;
  for (i = 0; i < marked_edges->length; i++) {    
    n1 = edge_list[i].n1;
    e1 = edge_list[i].e1;
    n2 = edge_list[i].n2;
    e2 = edge_list[i].e2;
    //    carmen_warn("Erasing %d %d %d %d\n", n1, e1, n2, e2);
    ((carmen_roadmap_edge_t *)(node_list[n1].edges->list))[e1].blocked = 0;
    ((carmen_roadmap_edge_t *)(node_list[n2].edges->list))[e2].blocked = 0;
  }
  marked_edges->length = 0;
}

int carmen_dynamics_num_blocking_people(carmen_traj_point_t *p1, 
					carmen_traj_point_t *p2)
{
  int i;
  int count = 0;
  carmen_dot_person_t *person;

  for (i = 0; i < people->length; i++) {
    person = (carmen_dot_person_t *)carmen_list_get(people, i);
    if (is_blocked(p1->x, p1->y, p2->x, p2->y, person->x, person->y, 
		   person->vx, person->vxy, person->vy)) 
      count++;
  }
  
  return count;
}

