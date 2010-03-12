 /*********************************************************
 *
 * This source code is part of the Carnegie Mellon Robot
 * Navigation Toolkit (CARMEN)
 *
 * CARMEN Copyright (c) 2002 Michael Montemerlo, Nicholas
 * Roy, Sebastian Thrun, Dirk Haehnel, Cyrill Stachniss,
 * and Jared Glover
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

#include "pyCarmen.h"

//static MessageHandler *_callback;



/*Deal with the front laser handler here*/
static MessageHandler *_laser_callback;
class front_laser{
 public:
  static void lazer_msg(carmen_robot_laser_message *msg)
  {
    _laser_callback->set_front_laser_message(msg);
    if (_laser_callback) _laser_callback->run_cb("front_laser", "get_front_laser_message()");
  }

  front_laser(MessageHandler *cb)
  {
    _laser_callback = cb;
    carmen_robot_subscribe_frontlaser_message
      (NULL, (carmen_handler_t)lazer_msg, CARMEN_SUBSCRIBE_LATEST);
  }
};


/*Deal with the rear laser handler here*/
static MessageHandler *_rlaser_callback;
class rear_laser{
 public:
  static void lazer_msg_rear(carmen_robot_laser_message *msg)
  {
    _rlaser_callback->set_rear_laser_message(msg);
    if (_rlaser_callback) _rlaser_callback->run_cb("rear_laser", "get_rear_laser_message()");
  }

  rear_laser(MessageHandler *cb)
  {
    _rlaser_callback = cb;
    carmen_robot_subscribe_rearlaser_message
      (NULL, (carmen_handler_t)lazer_msg_rear, CARMEN_SUBSCRIBE_LATEST);
  }
};

/* Numbered lasers here -- robot messages */
static MessageHandler *_robot_laser1_callback;
class robot_laser1{
 public:
  static void robot_lazer1_msg(carmen_robot_laser_message *msg)
  {
    _robot_laser1_callback->set_robot_laser1_message(msg);
    if (_robot_laser1_callback) _robot_laser1_callback->run_cb("robot_laser1", "get_robot_laser1_message()");
  }

  robot_laser1(MessageHandler *cb)
  {
    _robot_laser1_callback = cb;
    carmen_robot_subscribe_frontlaser_message
      (NULL, (carmen_handler_t)robot_lazer1_msg, CARMEN_SUBSCRIBE_LATEST);
  }
};

static MessageHandler *_robot_laser2_callback;
class robot_laser2{
 public:
  static void robot_lazer2_msg(carmen_robot_laser_message *msg)
  {
    _robot_laser2_callback->set_robot_laser2_message(msg);
    if (_robot_laser2_callback) _robot_laser2_callback->run_cb("robot_laser2", "get_robot_laser2_message()");
  }

  robot_laser2(MessageHandler *cb)
  {
    _robot_laser2_callback = cb;
    carmen_robot_subscribe_rearlaser_message
      (NULL, (carmen_handler_t)robot_lazer2_msg, CARMEN_SUBSCRIBE_LATEST);
  }
};


/* Numbered lasers here -- laser messages */
static MessageHandler *_laser1_callback;
class laser1{
 public:
  static void lazer1_msg(carmen_laser_laser_message *msg)
  {
    _laser1_callback->set_laser1_message(msg);
    if (_laser1_callback) _laser1_callback->run_cb("laser1", "get_laser1_message()");
  }

  laser1(MessageHandler *cb)
  {
    _laser1_callback = cb;
    carmen_laser_subscribe_laser1_message
      (NULL, (carmen_handler_t)lazer1_msg, CARMEN_SUBSCRIBE_LATEST);
  }
};

static MessageHandler *_laser2_callback;
class laser2{
 public:
  static void lazer2_msg(carmen_laser_laser_message *msg)
  {
    _laser2_callback->set_laser2_message(msg);
    if (_laser2_callback) _laser2_callback->run_cb("laser2", "get_laser2_message()");
  }

  laser2(MessageHandler *cb)
  {
    _laser2_callback = cb;
    carmen_laser_subscribe_laser2_message
      (NULL, (carmen_handler_t)lazer2_msg, CARMEN_SUBSCRIBE_LATEST);
  }
};

static MessageHandler *_laser3_callback;
class laser3{
 public:
  static void lazer3_msg(carmen_laser_laser_message *msg)
  {
    _laser3_callback->set_laser3_message(msg);
    if (_laser3_callback) _laser3_callback->run_cb("laser3", "get_laser3_message()");
  }

  laser3(MessageHandler *cb)
  {
    _laser3_callback = cb;
    carmen_laser_subscribe_laser3_message
      (NULL, (carmen_handler_t)lazer3_msg, CARMEN_SUBSCRIBE_LATEST);
  }
};

static MessageHandler *_laser4_callback;
class laser4{
 public:
  static void lazer4_msg(carmen_laser_laser_message *msg)
  {
    _laser4_callback->set_laser4_message(msg);
    if (_laser4_callback) _laser4_callback->run_cb("laser4", "get_laser4_message()");
  }

  laser4(MessageHandler *cb)
  {
    _laser4_callback = cb;
    carmen_laser_subscribe_laser4_message
      (NULL, (carmen_handler_t)lazer4_msg, CARMEN_SUBSCRIBE_LATEST);
  }
};

static MessageHandler *_laser5_callback;
class laser5{
 public:
  static void lazer5_msg(carmen_laser_laser_message *msg)
  {
    _laser5_callback->set_laser5_message(msg);
    if (_laser5_callback) _laser5_callback->run_cb("laser5", "get_laser5_message()");
  }

  laser5(MessageHandler *cb)
  {
    _laser5_callback = cb;
    carmen_laser_subscribe_laser5_message
      (NULL, (carmen_handler_t)lazer5_msg, CARMEN_SUBSCRIBE_LATEST);
  }
};

/*Deal with the global pose handler here*/
static MessageHandler *_global_pose_callback;
class global_pose{
 public:

  static void my_callback(carmen_localize_globalpos_message *msg)
  {
    _global_pose_callback->set_globalpos_message(msg);
    if (_global_pose_callback) _global_pose_callback->run_cb("global_pose", "get_globalpos_message()");
  }

  global_pose(MessageHandler *cb)
  {    
    _global_pose_callback = cb;
    carmen_localize_subscribe_globalpos_message
      (NULL, (carmen_handler_t)my_callback, CARMEN_SUBSCRIBE_LATEST);
  }
};

/*Deal with the odometry handler here*/
static MessageHandler *_odometry_callback;
class odometry {
 public:

  static void my_callback(carmen_base_odometry_message *msg)
  {
    _odometry_callback->set_odometry_message(msg);
    if (_odometry_callback) _odometry_callback->run_cb("odometry", "get_odometry_message()");
  }

  odometry(MessageHandler *cb)
  {    
    _odometry_callback = cb;
    carmen_base_subscribe_odometry_message
      (NULL, (carmen_handler_t)my_callback, CARMEN_SUBSCRIBE_LATEST);
  }
};


/*Deal with the odometry handler here*/
static MessageHandler *_sonar_callback;
class sonar {
 public:

  static void my_callback(carmen_base_sonar_message *msg)
  {
    _sonar_callback->set_sonar_message(msg);
    if (_sonar_callback) _sonar_callback->run_cb("sonar", "get_sonar_message()");
  }

  sonar(MessageHandler *cb)
  {    
    _sonar_callback = cb;
    carmen_base_subscribe_sonar_message
      (NULL, (carmen_handler_t)my_callback, CARMEN_SUBSCRIBE_LATEST);
  }
};


/*Deal with the bumper handler here*/
static MessageHandler *_bumper_callback;
class bumper {
 public:

  static void my_callback(carmen_base_bumper_message *msg)
  {
    _bumper_callback->set_bumper_message(msg);
    if (_bumper_callback) _bumper_callback->run_cb("bumper", "get_bumper_message()");
  }

  bumper(MessageHandler *cb)
  {    
    _bumper_callback = cb;
    carmen_base_subscribe_bumper_message
      (NULL, (carmen_handler_t)my_callback, CARMEN_SUBSCRIBE_LATEST);
  }
};

/*Deal with the Navigator Status handler here*/
static MessageHandler *_navigator_status_callback;
class navigator_status {
 public:

  static void my_callback(carmen_navigator_status_message *msg)
  {
    _navigator_status_callback->set_navigator_status_message(msg);
    if (_navigator_status_callback) _navigator_status_callback->run_cb("navigator_status", "get_navigator_status_message()");
  }

  navigator_status(MessageHandler *cb)
  { 
    _navigator_status_callback = cb;
    carmen_navigator_subscribe_status_message
      (NULL, (carmen_handler_t)my_callback, CARMEN_SUBSCRIBE_LATEST);
  }
};


/*Deal with the Navigator Plan handler here*/
static MessageHandler *_navigator_plan_callback;
class navigator_plan {
 public:

  static void my_callback(carmen_navigator_plan_message *msg)
  {
    _navigator_plan_callback->set_navigator_plan_message(msg);
    if (_navigator_plan_callback) _navigator_plan_callback->run_cb("navigator_plan", "get_navigator_plan_message()");
  }

  navigator_plan(MessageHandler *cb)
  { 
    _navigator_plan_callback = cb;
    carmen_navigator_subscribe_plan_message
      (NULL, (carmen_handler_t)my_callback, CARMEN_SUBSCRIBE_LATEST);
  }
};


/*Deal with the Navigator Stopped handler here*/
static MessageHandler *_navigator_stopped_callback;
class navigator_stopped {
 public:

  static void my_callback(carmen_navigator_autonomous_stopped_message *msg)
  {
    _navigator_stopped_callback->set_navigator_autonomous_stopped_message(msg);
    if (_navigator_stopped_callback) _navigator_stopped_callback->run_cb("navigator_stopped", "get_navigator_autonomous_stopped_message()");
  }

  navigator_stopped(MessageHandler *cb)
  { 
    _navigator_stopped_callback = cb;
    carmen_navigator_subscribe_autonomous_stopped_message
      (NULL, (carmen_handler_t)my_callback, CARMEN_SUBSCRIBE_LATEST);
  }
};



/*Deal with the arm handler here*/
static MessageHandler *_arm_callback;
class arm{
 public:
  static void my_callback(carmen_arm_state_message *msg)
  {
    _arm_callback->set_arm_state_message(msg);
    if (_arm_callback) _arm_callback->run_cb("arm_state", "get_arm_state_message()");
  }

  arm(MessageHandler *cb)
  {
    _arm_callback = cb;
    carmen_arm_subscribe_state_message
      (NULL, (carmen_handler_t)my_callback, CARMEN_SUBSCRIBE_LATEST);
  }
};


/*Deal with the arm handler here*/
static MessageHandler *_map_callback;
class map_change{
 public:

  static void map_update_handler(carmen_map_p msg) 
    {
      _map_callback->set_map_message(msg);
      if (_map_callback) _map_callback->run_cb("map_change", "get_map_message()");
    }

  map_change(MessageHandler *cb)
  {
    carmen_map_p the_latest_map;
    _map_callback = cb;
    
    carmen_map_subscribe_gridmap_update_message
      (NULL, (carmen_handler_t)map_update_handler, CARMEN_SUBSCRIBE_LATEST);
    
    the_latest_map = (carmen_map_p)calloc(1, sizeof(carmen_map_t));
    carmen_test_alloc(the_latest_map);
    carmen_map_get_gridmap(the_latest_map);
    map_update_handler(the_latest_map);
    carmen_map_destroy(&the_latest_map);
  }
  
  ~map_change(){
    //carmen_map_destroy(the_latest_map);
  }
};

/*Deal with the arm handler here*/
static MessageHandler *_sim_pose_callback;
class sim_global_pose{
 public:

  static void sim_pose_handler(carmen_simulator_truepos_message *msg) 
    {
      _sim_pose_callback->set_sim_truepos_message(msg);
      if (_sim_pose_callback) _sim_pose_callback->run_cb("sim_global_pose", "sim_global_pose_message()");
    }

  sim_global_pose(MessageHandler *cb)
  {
    _sim_pose_callback = cb;
    
    carmen_simulator_subscribe_truepos_message
      (NULL, (carmen_handler_t)sim_pose_handler, CARMEN_SUBSCRIBE_LATEST);
  }
  
};
