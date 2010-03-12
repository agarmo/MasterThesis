#include <carmen/carmen.h>
#include <carmen/dot.h>
#include "roadmap.h"
#include "dynamics.h"

#include <limits.h>
#include <assert.h>

static void get_blocking_object_to_next(carmen_dot_t *blocking_object,
					carmen_roadmap_vertex_t *node, 
					carmen_roadmap_t *roadmap)
{
  double best_utility = FLT_MAX;
  int best_neighbour = 0;
  double utility;
  int i;
  carmen_roadmap_edge_t *edges;
  carmen_roadmap_vertex_t *node_list;

  node_list = (carmen_roadmap_vertex_t *)(roadmap->nodes->list);
  edges = (carmen_roadmap_edge_t *)(node->edges->list);

  best_neighbour = -1;
  best_utility = FLT_MAX;
  for (i = 0; i < node->edges->length; i++) {
    utility = edges[i].cost + node_list[edges[i].id].utility;
    if (utility < best_utility) {
      best_utility = utility;
      best_neighbour = i;
    }    
  }

  assert(edges[best_neighbour].blocked);

  carmen_dynamics_get_block(node, node_list+edges[best_neighbour].id,
			    blocking_object, roadmap->avoid_people);
}

static void get_blocking_object(carmen_dot_t *blocking_object, 
				carmen_world_point_t *robot,
				carmen_roadmap_t *road)
{
  int length;
  carmen_roadmap_vertex_t *n1, *n2;
  carmen_roadmap_vertex_t *node;

  node = carmen_roadmap_best_node(robot, road);  
  if (!node || carmen_dynamics_test_node(node, 1))
    return;

  length = 0;

  n1 = node;
  length = 2;
  while (n1 != NULL && n1->utility > 0) {
    n2 = carmen_roadmap_next_node(n1, road);
    if (n2 == NULL) {
      get_blocking_object_to_next(blocking_object, n1, road);
      return;
    }
    n1 = n2;
    length++;
  }

  return;
}

void carmen_roadmap_refine_get_radius(double vx, double vy, double vxy, 
				      double *radius)
{
  double e1, e2;

  e1 = (vx + vy)/2.0 + sqrt(4*vxy*vxy + (vx-vy)*(vx-vy))/2.0;
  e2 = (vx + vy)/2.0 - sqrt(4*vxy*vxy + (vx-vy)*(vx-vy))/2.0;

  e1 = 3*sqrt(e1);
  e2 = 3*sqrt(e2);

  if (e1 < 1)
    e1 = .5;
  if (e2 < 1)
    e2 = .5;

  *radius = (e1 + e2) / 2;
}

void carmen_roadmap_refine_graph(carmen_world_point_t *robot, 
				 carmen_roadmap_t *road)
{
  carmen_dot_t blocking_object;
  double varx, vary, varxy;
  double radius;
  double x, y;

  int i;
  double theta;
  carmen_world_point_t world_pt;
  carmen_map_point_t map_pt;
  carmen_roadmap_vertex_t *node_list;
  carmen_roadmap_vertex_t *goal_node;
  carmen_world_point_t goal;

  get_blocking_object(&blocking_object, robot, road);

  if (blocking_object.dot_type == carmen_dot_person) {
    x = blocking_object.data.person.x;
    y = blocking_object.data.person.y;
    radius = blocking_object.data.person.r;
    radius *= 1.1;
  } else if (blocking_object.dot_type == carmen_dot_trash) {
    //dbug: use convex hull
    x = blocking_object.data.trash.x;
    y = blocking_object.data.trash.y;
    varx = blocking_object.data.trash.vx;
    vary = blocking_object.data.trash.vy;
    varxy = blocking_object.data.trash.vxy;

    carmen_roadmap_refine_get_radius(varx, vary, varxy, &radius);
    radius *= 1.1;

  } else { // if (blocking_object.dot_type == carmen_dot_door) {
    x = blocking_object.data.door.x;
    y = blocking_object.data.door.y;
    varx = blocking_object.data.door.vx;
    vary = blocking_object.data.door.vy;
    varxy = blocking_object.data.door.vxy;

    carmen_roadmap_refine_get_radius(varx, vary, varxy, &radius);
    radius *= 1.1;
  }

  node_list = (carmen_roadmap_vertex_t *)road->nodes->list;

  for (theta = -M_PI; theta < M_PI; theta += M_PI/8) {
    world_pt.pose.x = x + radius*cos(theta);
    world_pt.pose.y = y + radius*sin(theta);
    world_pt.map = road->c_space;

    carmen_world_to_map(&world_pt, &map_pt);

    if (road->c_space->map[map_pt.x][map_pt.y] > 1)
      continue;

    for (i = 0; i < road->nodes->length; i++) {
      if (map_pt.x == node_list[i].x && map_pt.y == node_list[i].y)
	break;
    }

    if (i == road->nodes->length) {
      carmen_roadmap_add_node(road, map_pt.x, map_pt.y);    
      node_list = (carmen_roadmap_vertex_t *)road->nodes->list;
    }
  }

  goal_node = (carmen_roadmap_vertex_t *)
    carmen_list_get(road->nodes, road->goal_id);
  map_pt.x = goal_node->x;
  map_pt.y = goal_node->y;
  map_pt.map = road->c_space;
  carmen_map_to_world(&map_pt, &goal);

  carmen_warn("Refine: Goal: %f %f\n", goal.pose.x, goal.pose.y);

  carmen_roadmap_plan(road, &goal);
}
