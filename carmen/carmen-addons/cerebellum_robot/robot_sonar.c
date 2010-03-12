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

#include "robot.h"
#include "robot_main.h"
#include "robot_sonar.h"

#define        SONAR_AVERAGE             3

static carmen_base_sonar_message base_sonar;
static double sonar_local_timestamp;
static carmen_robot_sonar_message robot_sonar;

static int sonar_count = 0;
static int sonar_ready = 0;

static double max_front_velocity = 0;
static double min_rear_velocity = -0;

double carmen_robot_interpolate_heading(double head1, double head2, double fraction);

static void check_message_data_chunk_sizes(void)
{
  int first=1;

  if(first) 
    {
      
      robot_sonar.num_sonars = base_sonar.num_sonars;
      robot_sonar.range = 
      	(double *)calloc(robot_sonar.num_sonars, sizeof(double));
      carmen_test_alloc(robot_sonar.range);
      robot_sonar.sonar_locations = 
	(carmen_point_p)calloc(robot_sonar.num_sonars, sizeof(carmen_point_t));
      carmen_test_alloc(robot_sonar.sonar_locations);
      robot_sonar.sonar_offsets =
	(carmen_point_p)calloc(robot_sonar.num_sonars, sizeof(carmen_point_t));
      carmen_test_alloc(robot_sonar.sonar_offsets);
      robot_sonar.tooclose = 
	(char *)calloc(robot_sonar.num_sonars, sizeof(char));
	carmen_test_alloc(robot_sonar.tooclose);
      first = 0;
    } 
  else if(robot_sonar.num_sonars != base_sonar.num_sonars) 
    {
      robot_sonar.num_sonars = base_sonar.num_sonars;
      robot_sonar.range = (double *)realloc(robot_sonar.range, sizeof(double) * robot_sonar.num_sonars);
      carmen_test_alloc(robot_sonar.range);
      robot_sonar.sonar_locations = (carmen_point_p)realloc(robot_sonar.sonar_locations,sizeof(carmen_point_t) * robot_sonar.num_sonars);
      carmen_test_alloc(robot_sonar.sonar_locations);
      robot_sonar.sonar_offsets = (carmen_point_p)realloc(robot_sonar.sonar_offsets, sizeof(carmen_point_t)* robot_sonar.num_sonars);
      carmen_test_alloc(robot_sonar.sonar_offsets);
      robot_sonar.tooclose = (char *)realloc(robot_sonar.tooclose,sizeof(char) * robot_sonar.num_sonars);
      carmen_test_alloc(robot_sonar.tooclose);
    }
}
      

static void
construct_sonar_message(carmen_robot_sonar_message *msg, int which, double fraction)
{
  int i;
  double r, t;

  msg->robot_location.x=carmen_robot_odometry[which].x + fraction *
    (carmen_robot_odometry[which + 1].x - carmen_robot_odometry[which].x);
  msg->robot_location.y= carmen_robot_odometry[which].y + fraction *
    (carmen_robot_odometry[which + 1].y - carmen_robot_odometry[which].y);
  msg->robot_location.theta=carmen_robot_interpolate_heading
    (carmen_robot_odometry[which].theta, carmen_robot_odometry[which + 1].theta,fraction);

  for(i=0; i<msg->num_sonars; i++)
    {
      r = hypot(msg->sonar_offsets[i].x, msg->sonar_offsets[i].y);
      t = carmen_normalize_theta(atan2(msg->sonar_offsets[i].y, msg->sonar_offsets[i].x) + msg->robot_location.theta);
      msg->sonar_locations[i].theta =carmen_normalize_theta(msg->sonar_offsets[i].theta + msg->robot_location.theta);
      msg->sonar_locations[i].x = msg->robot_location.x + r*cos(t);
      msg->sonar_locations[i].y = msg->robot_location.y + r*sin(t);
    }
}

static void publish_sonar_message(carmen_robot_sonar_message sonar)
{
  IPC_RETURN_TYPE err;
  err = IPC_publishData(CARMEN_ROBOT_SONAR_NAME, &sonar);
  carmen_test_ipc_exit(err, "Could not publish", CARMEN_ROBOT_SONAR_NAME);
}

void carmen_robot_correct_sonar_and_publish(void) 
{
  
  double sonar_skew;
  double odometry_skew;
  double fraction;
  int i, which;
  
  if(sonar_ready)
    {
      if (strcmp(carmen_robot_host, base_sonar.host) == 0)
	sonar_skew = 0;
      else 
	sonar_skew = carmen_running_average_report(SONAR_AVERAGE);

      odometry_skew = carmen_robot_get_odometry_skew();
      which = -1;
      
      for(i = 0; i < MAX_READINGS; i++)
	if(base_sonar.timestamp + sonar_skew >= carmen_robot_odometry[i].timestamp + odometry_skew)
	  which = i;
      
      if(which >= 0 && which < MAX_READINGS - 1)
	{
	  fraction = ((base_sonar.timestamp + sonar_skew) - (carmen_robot_odometry[which].timestamp + odometry_skew))
	    / (carmen_robot_odometry[which + 1].timestamp - carmen_robot_odometry[which].timestamp);
	  
	  construct_sonar_message(&robot_sonar, which, fraction);
	  
	  if(sonar_count > ESTIMATES_CONVERGE || !carmen_robot_converge) 
	    {	      
	      // publish odometry corrected laser message 
	      publish_sonar_message(robot_sonar);
	      fprintf(stderr, "s");
	    }
	  sonar_ready = 0;
	}
    }
}
static void sonar_handler(void)
{
  int i;
  double safety_distance;
  
  double theta;
  carmen_traj_point_t robot_posn;
  carmen_traj_point_t obstacle_pt;
  double max_velocity;
  double velocity;

  int tooclose = -1;
  double tooclose_theta = 0;

  int min_index;
  //double min_theta = 0;
  double min_dist;
  
  int debug = carmen_carp_get_verbose();
  

  check_message_data_chunk_sizes();
  
  sonar_local_timestamp = carmen_get_time_ms();
  if(sonar_count <= ESTIMATES_CONVERGE)
    sonar_count++;

  carmen_running_average_add(SONAR_AVERAGE, sonar_local_timestamp-base_sonar.timestamp);    
  
  memcpy(robot_sonar.range, base_sonar.range, robot_sonar.num_sonars * sizeof(double));
  memcpy(robot_sonar.sonar_offsets, base_sonar.sonar_offsets, robot_sonar.num_sonars*sizeof(carmen_point_t));
  memset(robot_sonar.sonar_locations, 0, robot_sonar.num_sonars*sizeof(carmen_point_t));
  memset(robot_sonar.tooclose, 0, robot_sonar.num_sonars);

  safety_distance = 
    carmen_robot_config.length / 2.0 + carmen_robot_config.approach_dist + 
    0.5 * carmen_robot_latest_odometry.tv * carmen_robot_latest_odometry.tv / 
    carmen_robot_config.acceleration +
    carmen_robot_latest_odometry.tv * carmen_robot_config.reaction_time;
  
  robot_sonar.forward_safety_dist = safety_distance;
  robot_sonar.side_safety_dist = carmen_robot_config.width / 2.0 + carmen_robot_config.side_dist;
  
  robot_sonar.turn_axis = 1e6;
  robot_sonar.timestamp = base_sonar.timestamp;
  robot_sonar.sensor_angle=base_sonar.sensor_angle;
  strncpy(robot_sonar.host, base_sonar.host, 10);
  
  max_velocity = carmen_robot_config.max_t_vel;
  
  robot_posn.x = 0;
  robot_posn.y = 0;
  robot_posn.theta = 0;
  
  carmen_carp_set_verbose(0);
    
  for(i=0; i<robot_sonar.num_sonars; i++)
    {
      theta=robot_sonar.sonar_offsets[i].theta;
      obstacle_pt.x=robot_sonar.sonar_offsets[i].x+robot_sonar.range[i]*cos(theta);
      obstacle_pt.y=robot_sonar.sonar_offsets[i].y+robot_sonar.range[i]*sin(theta);
      carmen_geometry_move_pt_to_rotating_ref_frame(&obstacle_pt, carmen_robot_latest_odometry.tv, carmen_robot_latest_odometry.rv);
      velocity=carmen_geometry_compute_velocity(robot_posn, obstacle_pt, &carmen_robot_config);
      if(velocity < carmen_robot_config.max_t_vel)
	{
	  if(velocity<max_velocity)
	    {
	      tooclose=i;
	      tooclose_theta=theta;
	      max_velocity = velocity;
	    }
	  robot_sonar.tooclose[i]=1;
	}

      if(0 && i == robot_sonar.num_sonars/2)
	carmen_warn("output statement");
    }
  
  min_index=0;
  min_dist = robot_sonar.range[0];
  for(i=1; i<robot_sonar.num_sonars; i++)
    {
      if(robot_sonar.range[i]<min_dist)
	{
	  min_dist=robot_sonar.range[i];
	  min_index=i;
	}
    }
  
  carmen_carp_set_verbose(debug);
  
  if(max_velocity < carmen_robot_config.max_t_vel)
    {
      if(max_velocity > 0)
	carmen_verbose("junk goes here");
      else
	carmen_verbose("more junk");
    }
  else
    carmen_verbose("no junk!");
  carmen_carp_set_verbose(0);
  sonar_ready=1;
  
  if(max_velocity<=0 && max_velocity<MIN_ALLOWED_VELOCITY)
      max_velocity=0.0;
  if(max_velocity<=0 && carmen_robot_latest_odometry.tv>0.0)
    carmen_robot_stop_robot(ALLOW_ROTATE);
  max_front_velocity=max_velocity;
  carmen_carp_set_verbose(debug);
  
}

double carmen_robot_sonar_max_front_velocity(void) 
{
  return max_front_velocity;
}

double carmen_robot_sonar_min_rear_velocity(void) 
{
  return min_rear_velocity;
}

void carmen_robot_add_sonar_handler(void) 
{
  carmen_base_subscribe_sonar_message(&base_sonar,
				    (carmen_handler_t)sonar_handler,
				    CARMEN_SUBSCRIBE_LATEST);
}

void carmen_robot_add_sonar_parameters(char *progname __attribute__ ((unused)) ) 
{

}

