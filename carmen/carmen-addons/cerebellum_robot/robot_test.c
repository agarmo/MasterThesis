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

void robot_frontlaser_handler(carmen_robot_laser_message *front_laser)
{
  carmen_warn("front_laser\n");
  carmen_warn("%.2f %.2f %.2f\n", front_laser->odom_x, front_laser->odom_y, 
	    carmen_radians_to_degrees(front_laser->odom_theta));
}

void robot_rearlaser_handler(carmen_robot_laser_message *rear_laser)
{
  carmen_warn("\nrear_laser\n");
  carmen_warn("%.2f %.2f %.2f\n", rear_laser->odom_x, rear_laser->odom_y, 
	    carmen_radians_to_degrees(rear_laser->odom_theta));
}

void robot_sonar_handler(carmen_robot_sonar_message *sonar_message)
{
  int i;
  carmen_warn("\nrobot_sonar\n");

  carmen_warn("num_sonars %i\n",sonar_message->num_sonars);

  carmen_warn("sensor_angle %.3f\n",sonar_message->sensor_angle);

  carmen_warn("range ");
  for(i=0; i<sonar_message->num_sonars; i++)
    carmen_warn("%.1f ",sonar_message->range[i]);
  carmen_warn("\n");

  carmen_warn("tooclose ");
  for(i=0; i<sonar_message->num_sonars; i++)
    carmen_warn("%i ",(int)sonar_message->tooclose[i]);
  carmen_warn("\n");

  carmen_warn("robot_location (%.3f,%.3f,%.3f)\n",
	  sonar_message->robot_location.x,
	  sonar_message->robot_location.y,
	  sonar_message->robot_location.theta);

  carmen_warn("sonar_locations ");
  for(i=0; i<sonar_message->num_sonars; i++)
    carmen_warn("(%.3f,%.3f,%.3f) ",
	    sonar_message->sonar_locations[i].x,
	    sonar_message->sonar_locations[i].y,
	    sonar_message->sonar_locations[i].theta);
  carmen_warn("\n");

  carmen_warn("sonar_offsets ");
  for(i=0; i<sonar_message->num_sonars; i++)
    carmen_warn("(%.3f,%.3f,%.3f) ",
	    sonar_message->sonar_offsets[i].x,
	    sonar_message->sonar_offsets[i].y,
	    sonar_message->sonar_offsets[i].theta);
  carmen_warn("\n");
					       
  carmen_warn("forward_safety_dist %f\n",
	  sonar_message->forward_safety_dist);

  carmen_warn("side_safety_dist %f\n", 
	  sonar_message->side_safety_dist);

  carmen_warn("turn_axis %f\n", sonar_message->turn_axis);

  carmen_warn("timestamp %f\n", sonar_message->timestamp);

  carmen_warn("host %s\n\n", sonar_message->host);
}

void base_odometry_handler(carmen_base_odometry_message *odometry)
{
  carmen_warn("\nrear_laser\n");
  carmen_warn("%.2f %.2f %.2f\n", odometry->x, odometry->y, 
	    carmen_radians_to_degrees(odometry->theta));
}


void shutdown_module(int x)
{
  if(x == SIGINT) {
    carmen_robot_velocity_command(0, 0);
    close_ipc();
    printf("Disconnected from robot.\n");
    exit(0);
  }
}

int main(int argc __attribute__ ((unused)), char **argv __attribute__ ((unused)))
{
  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);
  
  signal(SIGINT, shutdown_module);

  carmen_robot_subscribe_frontlaser_message
    (NULL, (carmen_handler_t)robot_frontlaser_handler,
     CARMEN_SUBSCRIBE_LATEST);
  carmen_robot_subscribe_rearlaser_message
    (NULL,(carmen_handler_t)robot_rearlaser_handler,
     CARMEN_SUBSCRIBE_LATEST);
  carmen_robot_subscribe_sonar_message
    (NULL, (carmen_handler_t)robot_sonar_handler,
     CARMEN_SUBSCRIBE_LATEST);
  carmen_base_subscribe_odometry_message
    (NULL, (carmen_handler_t)base_odometry_handler,
     CARMEN_SUBSCRIBE_LATEST);

  carmen_robot_move_along_vector(0, M_PI/2);
  
  sleep(10);
  carmen_robot_move_along_vector(2, 0);

  while(1) {
    sleep_ipc(0.1);
    carmen_warn(".");
  }
  return 0;
}
