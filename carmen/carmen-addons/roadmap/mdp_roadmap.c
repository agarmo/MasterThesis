#include <carmen/carmen.h>
#include <limits.h>
#include <assert.h>
#include "roadmap.h"
#include "mdp_roadmap.h"
#include "dynamics.h"

static double compute_path_time(carmen_roadmap_t *roadmap)
{
  int i;
  double total = 0;
  carmen_traj_point_t *start, *dest;
  int num_blocking_people;

  assert (roadmap->path->length > 0);

  start = (carmen_traj_point_t *)carmen_list_get(roadmap->path, 0);
  for (i = 1; i < roadmap->path->length; i++) {
    dest = (carmen_traj_point_t *)carmen_list_get(roadmap->path, i);
    total += carmen_distance_traj(start, dest)/roadmap->max_t_vel;
    if (0)
      carmen_warn("%f : %f %f -> %f %f\n", total, start->x, start->y,
		  dest->x, dest->y);
    if (!roadmap->avoid_people) {
      num_blocking_people = carmen_dynamics_num_blocking_people(start, dest);
      total += 10*num_blocking_people;
    }
    start = dest;
  }

  return total;
}

static void synchronize_node_lists(carmen_roadmap_t *roadmap,
				   carmen_roadmap_t *roadmap_without_people)
{
  carmen_roadmap_vertex_t *node;
  int i;

  if (roadmap_without_people->nodes->length >= roadmap->nodes->length) 
    return;

  for (i = roadmap_without_people->nodes->length; i < roadmap->nodes->length;
       i++) {
    node = (carmen_roadmap_vertex_t *)carmen_list_get(roadmap->nodes, i);
    carmen_roadmap_add_node(roadmap_without_people, node->x, node->y);
  }
}

carmen_list_t *carmen_roadmap_mdp_generate_path(carmen_traj_point_t *robot,
						carmen_roadmap_mdp_t *roadmap)
{
  int path_ok, path_without_people_ok;
  double time, time_without_people;

  if (roadmap->current_roadmap) {
    path_ok = carmen_roadmap_check_path(robot, roadmap->current_roadmap, 0);
    if (path_ok > 0) {
      carmen_roadmap_generate_path(robot, roadmap->current_roadmap);
      return roadmap->current_roadmap->path;
    }
  }

  //  carmen_warn("About to regenerate with people\n");

  path_ok = carmen_roadmap_check_path(robot, roadmap->roadmap, 1);

  //  carmen_warn("Generated path with people: %d\n", path_ok);

  if (path_ok > 0) {
    carmen_roadmap_generate_path(robot, roadmap->roadmap);
    time = compute_path_time(roadmap->roadmap);
    //    carmen_warn("Computed path with people: %f\n", time);
  } else 
    time = FLT_MAX;
  
  //  carmen_warn("Synchronizing lists\n");

  synchronize_node_lists(roadmap->roadmap, roadmap->roadmap_without_people);

  //  carmen_warn("Synchronizing lists done\n");

  //  carmen_warn("About to generate path without people\n");

  path_without_people_ok = 
    carmen_roadmap_check_path(robot, roadmap->roadmap_without_people, 1);

  //  carmen_warn("Generated path without people: %d\n", path_without_people_ok);

  if (path_without_people_ok > 0) {
    carmen_roadmap_generate_path(robot, roadmap->roadmap_without_people);
    time_without_people = 
      compute_path_time(roadmap->roadmap_without_people);
    //    carmen_warn("Computed path without people: %f\n", time_without_people);
  } else 
    time_without_people = FLT_MAX;

  if (time > FLT_MAX/2 && time_without_people > FLT_MAX/2)
    return NULL;

  carmen_warn("Avoiding people: %f Ignoring people: %f\n",
	      time, time_without_people);
  
  if (time > time_without_people) {
    roadmap->nodes = roadmap->roadmap_without_people->nodes;
    roadmap->path = roadmap->roadmap_without_people->path;
    roadmap->current_roadmap = roadmap->roadmap_without_people;
    return roadmap->roadmap_without_people->path;
  }

  roadmap->nodes = roadmap->roadmap->nodes;
  roadmap->path = roadmap->roadmap->path;
  roadmap->current_roadmap = roadmap->roadmap;

  return roadmap->roadmap->path;
}

carmen_roadmap_mdp_t *carmen_roadmap_mdp_initialize(carmen_map_t *map)
{
  carmen_roadmap_mdp_t *roadmap;

  roadmap = (carmen_roadmap_mdp_t *)calloc(1, sizeof(carmen_roadmap_mdp_t));
  carmen_test_alloc(roadmap);

  roadmap->roadmap = carmen_roadmap_initialize(map);
  roadmap->roadmap_without_people = carmen_roadmap_copy(roadmap->roadmap);
  roadmap->roadmap_without_people->avoid_people = 0;

  roadmap->c_space = roadmap->roadmap->c_space;
  roadmap->max_t_vel = roadmap->roadmap->max_t_vel;
  roadmap->max_r_vel = roadmap->roadmap->max_r_vel;
  roadmap->nodes = roadmap->roadmap->nodes;

  roadmap->path = NULL;

  return roadmap;
}

void carmen_roadmap_mdp_plan(carmen_roadmap_mdp_t *roadmap, 
			     carmen_world_point_t *goal)
{
  roadmap->current_roadmap = NULL;
  carmen_roadmap_plan(roadmap->roadmap, goal);
  carmen_roadmap_plan(roadmap->roadmap_without_people, goal);

  assert(roadmap->roadmap->goal_id == 
	 roadmap->roadmap_without_people->goal_id);

  roadmap->goal_id = roadmap->roadmap->goal_id;
}

carmen_roadmap_vertex_t *carmen_roadmap_mdp_nearest_node
(carmen_world_point_t *robot, carmen_roadmap_mdp_t *roadmap)
{
  if (roadmap->nodes == roadmap->roadmap_without_people->nodes)
    return carmen_roadmap_nearest_node(robot, roadmap->roadmap_without_people);

  return carmen_roadmap_nearest_node(robot, roadmap->roadmap);
}

carmen_roadmap_vertex_t *carmen_roadmap_mdp_next_node
(carmen_roadmap_vertex_t *node, carmen_roadmap_mdp_t *roadmap)
{
  if (roadmap->nodes == roadmap->roadmap_without_people->nodes)
    return carmen_roadmap_next_node(node, roadmap->roadmap_without_people);

  return carmen_roadmap_next_node(node, roadmap->roadmap);
}

void carmen_mdp_dynamics_clear_all_blocked(carmen_roadmap_mdp_t *roadmap)
{
  carmen_dynamics_clear_all_blocked(roadmap->roadmap);
  carmen_dynamics_clear_all_blocked(roadmap->roadmap_without_people);
}
