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

#include <carmen/carmen.h>
#include <cstdio>
#include <iostream>

class MessageHandler {
 private:
  carmen_robot_laser_message the_latest_msg_front;
  carmen_robot_laser_message the_latest_msg_rear;
  carmen_robot_laser_message the_latest_msg_robot_laser1;
  carmen_robot_laser_message the_latest_msg_robot_laser2;
  carmen_laser_laser_message the_latest_msg_laser1;
  carmen_laser_laser_message the_latest_msg_laser2;
  carmen_laser_laser_message the_latest_msg_laser3;
  carmen_laser_laser_message the_latest_msg_laser4;
  carmen_laser_laser_message the_latest_msg_laser5;
  carmen_localize_globalpos_message the_latest_globalpos;
  carmen_base_odometry_message the_latest_odometry;
  carmen_base_sonar_message the_latest_sonar;
  carmen_base_bumper_message the_latest_bumper;
  carmen_navigator_status_message the_latest_navigator_status;
  carmen_navigator_plan_message the_latest_navigator_plan;
  carmen_navigator_autonomous_stopped_message the_latest_navigator_autonomous_stopped;
  carmen_arm_state_message the_latest_arm_state;
  carmen_map_p the_latest_map;
  carmen_simulator_truepos_message the_latest_truepos;
 public:
        MessageHandler(){
	  the_latest_map = 0;
	}
        virtual ~MessageHandler() 
	  {}
	virtual void run_cb(char* type, char* msg) //carmen_robot_laser_message *msg) 
	  {}

	/*Front Laser Messages*/
	void set_front_laser_message(carmen_robot_laser_message *msg)
	  {memcpy(&the_latest_msg_front, msg, sizeof(carmen_robot_laser_message));}
	carmen_robot_laser_message* get_front_laser_message(void)
	  {return &the_latest_msg_front;}
	
	/*Rear Laser Messages*/
	void set_rear_laser_message(carmen_robot_laser_message *msg)
	  {memcpy(&the_latest_msg_rear, msg, sizeof(carmen_robot_laser_message));}
	carmen_robot_laser_message* get_rear_laser_message(void)
	  {return &the_latest_msg_rear;}

	/*laser numbered messages, from robot */
	void set_robot_laser1_message(carmen_robot_laser_message *msg)
	  {memcpy(&the_latest_msg_robot_laser1, msg, sizeof(carmen_robot_laser_message));}
	carmen_robot_laser_message* get_robot_laser1_message(void)
	  {return &the_latest_msg_robot_laser1;}

	void set_robot_laser2_message(carmen_robot_laser_message *msg)
	  {memcpy(&the_latest_msg_robot_laser2, msg, sizeof(carmen_robot_laser_message));}
	carmen_robot_laser_message* get_robot_laser2_message(void)
	  {return &the_latest_msg_robot_laser2;}

	/*laser numbered messages, from laser */
	void set_laser1_message(carmen_laser_laser_message *msg)
	  {memcpy(&the_latest_msg_laser1, msg, sizeof(carmen_laser_laser_message));}
	carmen_laser_laser_message* get_laser1_message(void)
	  {return &the_latest_msg_laser1;}

	void set_laser2_message(carmen_laser_laser_message *msg)
	  {memcpy(&the_latest_msg_laser2, msg, sizeof(carmen_laser_laser_message));}
	carmen_laser_laser_message* get_laser2_message(void)
	  {return &the_latest_msg_laser2;}

	void set_laser3_message(carmen_laser_laser_message *msg)
	  {memcpy(&the_latest_msg_laser3, msg, sizeof(carmen_laser_laser_message));}
	carmen_laser_laser_message* get_laser3_message(void)
	  {return &the_latest_msg_laser3;}

	void set_laser4_message(carmen_laser_laser_message *msg)
	  {memcpy(&the_latest_msg_laser4, msg, sizeof(carmen_laser_laser_message));}
	carmen_laser_laser_message* get_laser4_message(void)
	  {return &the_latest_msg_laser4;}

	void set_laser5_message(carmen_laser_laser_message *msg)
	  {memcpy(&the_latest_msg_laser5, msg, sizeof(carmen_laser_laser_message));}
	carmen_laser_laser_message* get_laser5_message(void)
	  {return &the_latest_msg_laser5;}

	/*Global Position Message*/
	void set_globalpos_message(carmen_localize_globalpos_message *msg)
	  {memcpy(&the_latest_globalpos, msg, sizeof(carmen_localize_globalpos_message));}
	carmen_localize_globalpos_message* get_globalpos_message()
	  {return &the_latest_globalpos;}

	/*Odometry Message*/
	void set_odometry_message(carmen_base_odometry_message *msg){
	  memcpy(&the_latest_odometry, msg, sizeof(carmen_base_odometry_message));
	}
	carmen_base_odometry_message* get_odometry_message() {return &the_latest_odometry;}

	/*Sonar Message*/
	void set_sonar_message(carmen_base_sonar_message *msg){
	  memcpy(&the_latest_sonar, msg, sizeof(carmen_base_sonar_message));
	}
	carmen_base_sonar_message* get_sonar_message() {return &the_latest_sonar;}

	
	/*Bumper Message*/
	void set_bumper_message(carmen_base_bumper_message *msg){
	  memcpy(&the_latest_bumper, msg, sizeof(carmen_base_bumper_message));
	}
	carmen_base_bumper_message* get_bumper_message() {return &the_latest_bumper;}


	/*Navigator Status Message*/
	void set_navigator_status_message(carmen_navigator_status_message *msg){
	  memcpy(&the_latest_navigator_status, msg, sizeof(carmen_navigator_status_message));
	}
	carmen_navigator_status_message* get_navigator_status_message() {return &the_latest_navigator_status;}

	/*Navigator Plan Message*/
	void set_navigator_plan_message(carmen_navigator_plan_message *msg){
	  memcpy(&the_latest_navigator_plan, msg, sizeof(carmen_navigator_plan_message));
	}
	carmen_navigator_plan_message* get_navigator_plan_message() {return &the_latest_navigator_plan;}

	/*Navigator Stop Message*/
	void set_navigator_autonomous_stopped_message(carmen_navigator_autonomous_stopped_message *msg){
	  memcpy(&the_latest_navigator_autonomous_stopped, msg, sizeof(carmen_navigator_autonomous_stopped_message));
	}
	carmen_navigator_autonomous_stopped_message* get_navigator_autonomous_stopped_message() {return &the_latest_navigator_autonomous_stopped;}

	/*Arm State Message*/
	void set_arm_state_message(carmen_arm_state_message *msg){
	  memcpy(&the_latest_arm_state, msg, sizeof(carmen_arm_state_message));
	}
	carmen_arm_state_message* get_arm_state_message() {return &the_latest_arm_state;}

	/*Map Update Message*/
	/*void init_map_message(){
	  the_latest_map = 0;
	  }*/

	void set_map_message(carmen_map_p msg){
	  if(!the_latest_map == 0)
	    carmen_map_destroy(&the_latest_map);
	  the_latest_map = carmen_map_copy(msg);
	}
	carmen_map_t* get_map_message() {return the_latest_map;}


	void set_sim_truepos_message(carmen_simulator_truepos_message *msg){
	  memcpy(&the_latest_truepos, msg, sizeof(carmen_simulator_truepos_message));
	}
	carmen_simulator_truepos_message* sim_global_pose_message() {return &the_latest_truepos;}


	/*void get_current_map(void){

	  the_latest_map = (carmen_map_t *)calloc(1, sizeof(carmen_map_t));
	  carmen_test_alloc(the_latest_map);
	  carmen_map_get_gridmap(the_latest_map);
	  }*/

	//_map_callback->set_map_message(msg);
	//if (_map_callback) _map_callback->run_cb("map_state", "get_map_message()");

};



