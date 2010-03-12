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

#ifndef PLANNER_INTERFACE_H
#define PLANNER_INTERFACE_H

#include <carmen/robot_messages.h>

#include "navigator.h"
#include "navigator_messages.h"

#ifdef __cplusplus
extern "C" {
#endif
  
  typedef struct {
    carmen_traj_point_p points;
    int length;
    int capacity;
  } carmen_planner_path_t, *carmen_planner_path_p;
  
  typedef struct {
    carmen_traj_point_t robot;
    carmen_point_t goal;	
    carmen_planner_path_t path;
    int goal_set;
  } carmen_planner_status_t, *carmen_planner_status_p;
  
  
  /* returns 1 if a new path was generated, otherwise returns 0 */
  
  int carmen_planner_update_robot(carmen_traj_point_p new_position);
  
  /* returns 1 if a new path was generated, otherwise returns 0 */
  
  int carmen_planner_update_goal(carmen_world_point_p new_goal, 
				 int any_orientation);
  
  /* returns 1 if robot reached goal, returns -1 if no path
     exists, otherwise returns 0 */
  
  int carmen_planner_next_waypoint(carmen_traj_point_p waypoint, 
				   int *is_goal,
				   carmen_navigator_config_t *nav_conf);
  
  void carmen_planner_set_map(carmen_map_p map);
  
  void carmen_planner_reset(void);
  
  void carmen_planner_update_map(carmen_robot_laser_message *laser_msg);
  
  carmen_navigator_map_message* carmen_planner_get_map_message
  (carmen_navigator_map_t map_type);
  
  void carmen_planner_get_status(carmen_planner_status_p status);
  
#ifdef __cplusplus
}
#endif

#endif
