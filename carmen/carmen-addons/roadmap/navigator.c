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
#include "navigator.h"
#include "navigator_ipc.h"
#include "planner_interface.h"

typedef void(*handler)(int);

static carmen_map_p nav_map;
static carmen_map_placelist_t placelist;
static carmen_robot_config_t robot_config;
static carmen_navigator_config_t nav_config;

static int cheat = 0;
static int autonomous_status = 0;

static carmen_traj_point_t robot_position;

static carmen_base_odometry_message odometry;
static carmen_base_odometry_message corrected_odometry;
static carmen_base_odometry_message last_odometry;
static carmen_robot_laser_message frontlaser, rearlaser;
static carmen_localize_globalpos_message globalpos;
static int globalpos_initialised = 0;

static void generate_next_motion_command(void);

static void base_odometry_handler(void) 
{

  corrected_odometry = odometry;

  if (!globalpos_initialised) 
    return;

  carmen_localize_correct_odometry(&corrected_odometry, &globalpos);
  
  robot_position.x = corrected_odometry.x;
  robot_position.y = corrected_odometry.y;
  robot_position.theta = corrected_odometry.theta;
  
  robot_position.t_vel = corrected_odometry.tv;
  robot_position.r_vel = corrected_odometry.rv;

  carmen_planner_update_robot(&robot_position);
         
  if (autonomous_status) 
    generate_next_motion_command();

  carmen_navigator_publish_status();
  carmen_navigator_publish_plan();
  last_odometry = corrected_odometry;
}

static void 
update_positions(void)
{
  static carmen_point_t last_position = {0, 0, 0};
  static double last_timestamp = 0.0;

  globalpos_initialised = 1;
  corrected_odometry = odometry;
  carmen_localize_correct_odometry(&corrected_odometry, &globalpos);

  if(!nav_config.dont_integrate_odometry) {
    robot_position.x = corrected_odometry.x;
    robot_position.y = corrected_odometry.y;
    robot_position.theta = corrected_odometry.theta;
  }
  else {
    robot_position.x = globalpos.globalpos.x;
    robot_position.y = globalpos.globalpos.y;
    robot_position.theta = globalpos.globalpos.theta;
  }

  carmen_planner_update_robot(&robot_position);
 
  /* If the distance between the new position and the old position is further
     than twice the maximum distance we could have travelled, travelling at
     top speed, then obviously this is a localize reset, and we should reset
     the map. */

  if (0 && carmen_distance(&last_position, &(globalpos.globalpos)) > 
      20*robot_config.max_t_vel / (globalpos.timestamp - last_timestamp)) {
    carmen_warn("last position was %f %f, new position is %f %f, resetting.\n",
		last_position.x, last_position.y, globalpos.globalpos.x,
		globalpos.globalpos.y);
    carmen_planner_reset();
  }

  last_timestamp = globalpos.timestamp;
  last_position = globalpos.globalpos;

  if (autonomous_status) 
    generate_next_motion_command();

  carmen_navigator_publish_status();
  carmen_navigator_publish_plan();
}

static void 
simulator_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
		  void *clientData __attribute__ ((unused)))
{
  IPC_RETURN_TYPE err = IPC_OK;
  FORMATTER_PTR formatter;
  carmen_simulator_truepos_message msg;

  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &msg, 
			   sizeof(carmen_simulator_truepos_message));
  IPC_freeByteArray(callData);

  carmen_test_ipc_return(err, "Could not unmarshall", 
			 IPC_msgInstanceName(msgRef));

  if (!cheat)
    return;

  globalpos.globalpos = msg.truepose;
  globalpos.odometrypos = msg.odometrypose;

  update_positions();
}


static void 
robot_globalpos_handler(void) 
{
  update_positions();
}

static void 
robot_frontlaser_handler(void) 
{
  /* If globalpos.timestamp == 0, then we know we haven't received any
     globalpos messages yet. We have no idea where we are in the map, so we
     can't integrate laser data into the map.
  */

  if (globalpos.timestamp <= 0 || !nav_config.update_map ||
      !autonomous_status) 
    return;

  carmen_localize_correct_laser(&frontlaser, &globalpos);
      
  carmen_planner_update_map(&frontlaser);

  carmen_navigator_publish_status();
  carmen_navigator_publish_plan();
}

static void 
robot_rearlaser_handler(void) 
{
  carmen_verbose("Got rearlaser message\n");

  /* If globalpos.timestamp == 0, then we know we haven't received any
     globalpos messages yet. We have no idea where we are in the map, so we
     can't integrate laser data into the map.
  */

  if (globalpos.timestamp <= 0 || !nav_config.update_map) 
    return;

  carmen_localize_correct_laser(&rearlaser, &globalpos);
  
  carmen_planner_update_map(&rearlaser);
      
}

static void 
generate_next_motion_command(void)
{
  carmen_traj_point_t waypoint;
  int waypoint_status;
  int is_goal;
  command_t command;
  double distance, theta;

  waypoint = robot_position;

  waypoint_status = 
    carmen_planner_next_waypoint(&waypoint, &is_goal, &nav_config);

  /* goal is reached */

  if (waypoint_status > 0) {
    autonomous_status = 0;
    carmen_navigator_publish_autonomous_stopped
      (CARMEN_NAVIGATOR_GOAL_REACHED_v);
    
    carmen_verbose("Autonomous off Motion command %.2f %.2f\n", command.tv,
		   carmen_radians_to_degrees(command.rv));
    
    command.tv = 0;
    command.rv = 0;
    carmen_robot_velocity_command(command.tv, command.rv);  
    return;
  }
  
  if (waypoint_status < 0) {
    command.tv = 0;
    command.rv = 0;
    carmen_robot_velocity_command(command.tv, command.rv);  
    return;
  }

  theta = carmen_angle_between(&robot_position, &waypoint);
  theta = theta - robot_position.theta;
  distance = carmen_distance_traj(&robot_position, &waypoint);

  carmen_robot_move_along_vector(distance, theta);

  carmen_verbose("Current pos: %.1f %.1f %.1f next %.1f %.1f\n", 
		 robot_position.x, robot_position.y, 
		 carmen_radians_to_degrees(robot_position.theta), 
		 waypoint.x, waypoint.y);
}

carmen_map_placelist_p 
carmen_navigator_get_places(void)
{
  return &placelist;
}

void
carmen_navigator_set_max_velocity(double vel)
{
  robot_config.max_t_vel = vel;
}

void 
carmen_navigator_goal(double x, double y) 
{
  carmen_world_point_t point;

  point.pose.x = x;
  point.pose.y = y;
  point.map = nav_map;

  carmen_planner_update_goal(&point, 1);
}

void 
carmen_navigator_goal_triplet(carmen_point_p point) 
{
  carmen_world_point_t wp;

  wp.pose = *point;
  wp.map = nav_map;

  carmen_planner_update_goal(&wp, 0);
}

int 
carmen_navigator_goal_place(char *name)
{
  int index;
  carmen_world_point_t goal;

  for (index = 0; index < placelist.num_places; index++) 
    {
      if (strcmp(name, placelist.places[index].name) == 0)
	break;
    }

  if (index == placelist.num_places)
    return -1;  

  goal.pose.x = placelist.places[index].x;
  goal.pose.y = placelist.places[index].y;
  goal.map = nav_map;

  if (placelist.places[index].type == CARMEN_NAMED_POSITION_TYPE) 
    carmen_planner_update_goal(&goal, 1);
  else
    {
      goal.pose.theta = placelist.places[index].theta;
      carmen_planner_update_goal(&goal, 0);
    }

  return 0;
}

void 
carmen_navigator_start_autonomous(void) 
{  
  autonomous_status = 1;
  generate_next_motion_command();
}

void 
carmen_navigator_stop_autonomous(void) 
{
  autonomous_status = 0;
  carmen_navigator_publish_autonomous_stopped(CARMEN_NAVIGATOR_USER_STOPPED_v);
  carmen_robot_velocity_command(0, 0);
}

int
carmen_navigator_autonomous_status(void)
{
  return autonomous_status;
} 

static void 
map_update_handler(carmen_map_t *new_map) 
{
  carmen_map_destroy(&nav_map);
  nav_map = carmen_map_copy(new_map);
  carmen_planner_set_map(nav_map);
}

static void 
navigator_shutdown(int signo __attribute__ ((unused)) )
{
  static int done = 0;

  if(!done) {
    close_ipc();
    printf("Disconnected from IPC.\n");

    done = 1;
  }
  exit(0);
}

static void 
read_parameters(int argc, char **argv)
{
  int num_items;

  carmen_param_t param_list[] = {
    {"robot", "max_t_vel", CARMEN_PARAM_DOUBLE, 
     &robot_config.max_t_vel, 1, NULL},
    {"robot", "max_r_vel", CARMEN_PARAM_DOUBLE, 
     &robot_config.max_r_vel, 1, NULL},
    {"robot", "min_approach_dist", CARMEN_PARAM_DOUBLE, 
     &robot_config.approach_dist, 1, NULL},
    {"robot", "min_side_dist", CARMEN_PARAM_DOUBLE, 
     &robot_config.side_dist, 1, NULL},
    {"robot", "length", CARMEN_PARAM_DOUBLE, 
     &robot_config.length, 0, NULL},
    {"robot", "width", CARMEN_PARAM_DOUBLE, 
     &robot_config.width, 0, NULL},
    {"robot", "acceleration", CARMEN_PARAM_DOUBLE, 
     &robot_config.acceleration, 1, NULL},
    {"robot", "reaction_time", CARMEN_PARAM_DOUBLE, 
     &robot_config.reaction_time, 0, NULL},
    {"robot", "curvature", CARMEN_PARAM_DOUBLE, 
     &robot_config.curvature, 1, NULL},

    {"navigator", "num_lasers", CARMEN_PARAM_INT, 
     &nav_config.num_lasers_to_use, 1, NULL},
    {"navigator", "update_map", CARMEN_PARAM_ONOFF, 
     &nav_config.update_map, 1, NULL},
    {"navigator", "max_laser_range", CARMEN_PARAM_INT, 
     &nav_config.max_range, 1, NULL},
    {"navigator", "max_collision_range", CARMEN_PARAM_INT, 
     &nav_config.max_collision_range, 1, NULL},
    {"navigator", "replan_frequency", CARMEN_PARAM_DOUBLE, 
     &nav_config.replan_frequency, 1, NULL},
    {"navigator", "smooth_path", CARMEN_PARAM_ONOFF, 
     &nav_config.smooth_path, 1, NULL},
    {"navigator", "goal_size", CARMEN_PARAM_DOUBLE,
     &nav_config.goal_size, 1, NULL},
    {"navigator", "goal_theta_tolerance", CARMEN_PARAM_DOUBLE,
     &nav_config.goal_theta_tolerance, 1, NULL},
    {"navigator", "dont_integrate_odometry", CARMEN_PARAM_ONOFF,
     &nav_config.dont_integrate_odometry, 1, NULL}
  };

  num_items = sizeof(param_list)/sizeof(param_list[0]);
  
  carmen_param_install_params(argc, argv, param_list, num_items);

  if (nav_config.goal_size < robot_config.approach_dist) {
    carmen_warn("%sBad things will happen when the approach distance is\n"
		"less than the navigator goal size. Changing navigator\n"
		"goal size to be %f (robot approach distance).%s\n\n",
		carmen_red_code, robot_config.approach_dist,
		carmen_normal_code);
    nav_config.goal_size = robot_config.approach_dist;
  }

  carmen_param_get_onoff("cheat", &cheat);
}

int 
main(int argc, char **argv)
{
  int x, y;
  carmen_offlimits_p offlimits;
  int num_offlimits_segments;
  char *goal_string;	
  unsigned int seed;
  
  carmen_initialize_ipc(argv[0]);

  carmen_param_check_version(argv[0]);
  IPC_setVerbosity(IPC_Print_Warnings);
  
  
  seed = carmen_randomize(&argc, &argv);
  carmen_warn("Seed: %u\n", seed);
  read_parameters(argc, argv);

  if(carmen_navigator_initialize_ipc() < 0)
    carmen_die("Error: could not connect to IPC Server\n");

  signal(SIGINT, navigator_shutdown);
	
  nav_map = (carmen_map_p)calloc(1, sizeof(carmen_map_t));
  carmen_test_alloc(nav_map);

  carmen_map_get_gridmap(nav_map);
  carmen_map_get_offlimits(&offlimits, &num_offlimits_segments);
  carmen_map_apply_offlimits_chunk_to_map(offlimits, num_offlimits_segments, 
					  nav_map);
  carmen_map_get_placelist(&placelist);



  carmen_planner_set_map(nav_map);

  if(!nav_config.dont_integrate_odometry)
    carmen_base_subscribe_odometry_message
      (&odometry, (carmen_handler_t)base_odometry_handler,
       CARMEN_SUBSCRIBE_LATEST);
  
  carmen_robot_subscribe_frontlaser_message
    (&frontlaser, (carmen_handler_t)robot_frontlaser_handler,
     CARMEN_SUBSCRIBE_LATEST);

  carmen_robot_subscribe_rearlaser_message
    (&rearlaser, (carmen_handler_t)robot_rearlaser_handler,
     CARMEN_SUBSCRIBE_LATEST);
  
  carmen_map_subscribe_gridmap_update_message
    (NULL, (carmen_handler_t)map_update_handler, CARMEN_SUBSCRIBE_LATEST);

  if (cheat) {
    if (IPC_isMsgDefined(CARMEN_SIMULATOR_TRUEPOS_NAME)) {
      IPC_subscribe(CARMEN_SIMULATOR_TRUEPOS_NAME,simulator_handler,NULL);
      IPC_setMsgQueueLength(CARMEN_SIMULATOR_TRUEPOS_NAME, 1);
    } else {
      carmen_warn("Can't cheat: not using simulator (truepos "
		  "message not defined).\n");
      cheat = 0;
    }
  } else {
    carmen_localize_subscribe_globalpos_message
      (&globalpos, (carmen_handler_t)robot_globalpos_handler,
       CARMEN_SUBSCRIBE_LATEST);
  }
  
  if (carmen_param_get_string("init_goal", &goal_string) == 1) {
    sscanf(goal_string, "%d %d", &x, &y);
    carmen_navigator_goal(x, y);
  }

  IPC_dispatch();

  return 0;
}
