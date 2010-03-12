/*********************************************************
 *
 * This source code is part of the Carnegie Mellon Robot
 * Navigation Toolkit (CARMEN)
 *
 * CARMEN Copyright (c) 2002 Michael Montemerlo, Nicholas
 * Roy, and Sebastian Thrun
 *
 * CARMEN is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public 
 * License as published by the Free Software Foundation; 
 * either version 2 of the License, or (at your option)
 * any later version.
 *
 * CARMEN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied 
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more 
 * details.
 *
 * You should have received a copy of the GNU General 
 * Public License along with CARMEN; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, 
 * Suite 330, Boston, MA  02111-1307 USA
 *
 ********************************************************/

#include <carmen/carmen.h>
#include <assert.h>
#include "roadmap.h"
#include "dynamics.h"
#include "mdp_roadmap.h"
#include "planner_interface.h"
#include "navigator.h"

static carmen_map_t *map = NULL;

static int have_goal = 0, have_robot = 0;
static int allow_any_orientation = 0;
static carmen_world_point_t goal;
static carmen_traj_point_t robot;
static double max_t_vel;
static double approach_dist;

static carmen_roadmap_mdp_t *roadmap = NULL;
static carmen_list_t *path = NULL;

int carmen_planner_update_goal(carmen_world_point_p new_goal, 
			       int any_orientation)
{
  allow_any_orientation = any_orientation;
  have_goal = 1;
  goal = *new_goal;

  if (map) {
    carmen_roadmap_mdp_plan(roadmap, new_goal);
  }

  carmen_verbose("Set goal to %.1f %.1f, done planning\n",
	      new_goal->pose.x, new_goal->pose.y);

  return 1;
}

int carmen_planner_update_robot(carmen_traj_point_p new_position)
{
  robot = *new_position;
  have_robot = 1;

  path = carmen_roadmap_mdp_generate_path(&robot, roadmap);

  return 0;
}

void carmen_planner_set_map(carmen_map_p new_map)
{
  map = new_map;

  carmen_verbose("Initialized with map\n");

  roadmap = carmen_roadmap_mdp_initialize(new_map);

  carmen_param_set_module("robot");
  carmen_param_get_double("max_t_vel", &max_t_vel);
  carmen_param_get_double("min_approach_dist", &approach_dist);
  carmen_param_subscribe_double("robot", "max_t_vel", &max_t_vel, NULL);
  carmen_param_subscribe_double("robot", "min_approach_dist", 
				&approach_dist, NULL);

  carmen_warn("max robot speed %f\n", max_t_vel);

  carmen_dynamics_initialize(new_map);
}

void carmen_planner_reset(void)
{
  have_goal = 0;
  have_robot = 0;
}

void carmen_planner_update_map(carmen_robot_laser_message *laser_msg)
{
  laser_msg = laser_msg;

}

void carmen_planner_get_status(carmen_planner_status_p status) 
{
  carmen_traj_point_t *traj_point;
  int i;

  status->goal_set = have_goal;

  status->path.length = 0;
  status->path.points = NULL;

  if (have_goal)
    status->goal = goal.pose;

  if (have_robot)
    status->robot = robot;

  if (!have_robot || !have_goal)
    return;  

  if (path == NULL) {
    status->path.length = 0;
    status->path.points = NULL;
  } else {
    status->path.length = path->length;
    status->path.points = (carmen_traj_point_p)
      calloc(status->path.length, sizeof(carmen_traj_point_t));
    carmen_test_alloc(status->path.points);
    for (i = 0; i < path->length; i++) {
      traj_point = (carmen_traj_point_t *)carmen_list_get(path, i);
      status->path.points[i] = *traj_point;
    }
  }

  return;
}

int carmen_planner_next_waypoint(carmen_traj_point_p waypoint, int *is_goal,
				 carmen_navigator_config_t *nav_conf)
{  
  carmen_traj_point_t start, way_pt, next_way_pt;
  double dist_to_waypoint, delta_theta;
  int node_is_too_close;
  int i;

  if (!have_robot || !have_goal || !map || !path)
    return -1;

  path = carmen_roadmap_mdp_generate_path(&robot, roadmap);

  if (path == NULL)
    return -1;

  start = *waypoint;
  i = 1;
  node_is_too_close = 1;
  way_pt = *((carmen_traj_point_t *)carmen_list_get(path, i));
  dist_to_waypoint = carmen_distance_traj(&start, &way_pt);
  while (i < path->length && node_is_too_close) {
    dist_to_waypoint = carmen_distance_traj(&start, &way_pt);
    if (dist_to_waypoint < approach_dist && i < path->length-1) {
      next_way_pt = *((carmen_traj_point_t *)
		      carmen_list_get(path, i+1));
      if (carmen_roadmap_points_are_visible(&start, &next_way_pt, 
					    roadmap->c_space))
	way_pt = next_way_pt;
      else
	node_is_too_close = 0;
    } else {
      node_is_too_close = 0;
    }
    i++;
  } 

  if (i == path->length && dist_to_waypoint < nav_conf->goal_size) {
    *is_goal = 1;
    if (allow_any_orientation)
      return 1;
    delta_theta = fabs(start.theta - goal.pose.theta);
    if (delta_theta < nav_conf->goal_theta_tolerance)
      return 1;
    waypoint->theta = goal.pose.theta;
  }

  waypoint->x = way_pt.x;
  waypoint->y = way_pt.y;

  return 0;
}

carmen_navigator_map_message *
carmen_planner_get_map_message(carmen_navigator_map_t map_type) 
{
  carmen_navigator_map_message *reply;
  int size, x_Size, y_Size;
  float *map_ptr;

  reply = (carmen_navigator_map_message *)
    calloc(1, sizeof(carmen_navigator_map_message));
  carmen_test_alloc(reply);
  
  x_Size = map->config.x_size;
  y_Size = map->config.y_size;
  size = x_Size * y_Size;

  reply->data = (unsigned char *)calloc(size, sizeof(float));
  carmen_test_alloc(reply->data);

  reply->config = map->config;

  switch (map_type) {
  case CARMEN_NAVIGATOR_MAP_v:
    map_ptr = map->complete_map;
    break;
  case CARMEN_NAVIGATOR_COST_v:
    map_ptr = roadmap->c_space->complete_map;
    break;
  default:
    carmen_warn("Request for unsupported data type : %d.\n", map_type);
    reply->size = 0;
    /*    gcc 3.3.3 does not compile with: */
    /*    (int)(reply->map_type) = -1; */
    reply->map_type = -1;
    return reply;
  }
  reply->size = size*sizeof(float);
  reply->map_type = map_type;
  memcpy(reply->data, map_ptr, size*sizeof(float));
  if (map_type == CARMEN_NAVIGATOR_UTILITY_v)
    free(map_ptr);
  return reply;
}

