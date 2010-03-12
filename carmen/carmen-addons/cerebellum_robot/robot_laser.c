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
#include "robot_laser.h"

#define        FRONT_LASER_AVERAGE             1
#define        REAR_LASER_AVERAGE              2

static double frontlaser_offset;
static double rearlaser_offset;

static carmen_laser_laser_message front_laser, rear_laser;
static double front_laser_local_timestamp, rear_laser_local_timestamp;
static int front_laser_count = 0, rear_laser_count = 0;
static int front_laser_ready = 0, rear_laser_ready = 0;
static carmen_robot_laser_message robot_front_laser, robot_rear_laser;

static double max_front_velocity = 0;
static double min_rear_velocity = -0;

double carmen_robot_interpolate_heading(double head1, double head2, double fraction);

static void 
publish_frontlaser_message(carmen_robot_laser_message laser_msg)
{
  IPC_RETURN_TYPE err;
  
  err = IPC_publishData(CARMEN_ROBOT_FRONTLASER_NAME, &laser_msg);
  carmen_test_ipc_exit(err, "Could not publish", CARMEN_ROBOT_FRONTLASER_NAME);
}

static void 
publish_rearlaser_message(carmen_robot_laser_message laser_msg)
{
  IPC_RETURN_TYPE err;

  err = IPC_publishData(CARMEN_ROBOT_REARLASER_NAME, &laser_msg);
  carmen_test_ipc_exit(err, "Could not publish", CARMEN_ROBOT_REARLASER_NAME);
}

static void 
construct_laser_message(carmen_robot_laser_message *message, double offset, 
			int which, double fraction, int rear) 
{
  message->odom_x = carmen_robot_odometry[which].x + fraction *
    (carmen_robot_odometry[which + 1].x - carmen_robot_odometry[which].x);
  message->odom_y = carmen_robot_odometry[which].y + fraction *
    (carmen_robot_odometry[which + 1].y - carmen_robot_odometry[which].y);
  message->odom_theta = carmen_robot_interpolate_heading
    (carmen_robot_odometry[which].theta, 
     carmen_robot_odometry[which + 1].theta,
     fraction);
  message->tv = carmen_robot_odometry[which].tv + fraction *
    (carmen_robot_odometry[which + 1].tv - carmen_robot_odometry[which].tv);
  message->rv = carmen_robot_odometry[which].rv + fraction *
    (carmen_robot_odometry[which + 1].rv - carmen_robot_odometry[which].rv);
  if(rear)
    message->theta = message->odom_theta + M_PI;
  else
    message->theta = message->odom_theta;
  message->theta = carmen_normalize_theta(message->theta);
  message->x = message->odom_x + offset * cos(message->theta);
  message->y = message->odom_y + offset * sin(message->theta);
}

void 
carmen_robot_correct_laser_and_publish(void) 
{
  double front_laser_skew, rear_laser_skew;
  double odometry_skew;
  double fraction;
  int i, which;

  if(front_laser_ready) {
    if (strcmp(carmen_robot_host, front_laser.host) == 0)
      front_laser_skew = 0;
    else
      front_laser_skew = carmen_running_average_report(FRONT_LASER_AVERAGE);
    
    odometry_skew = carmen_robot_get_odometry_skew();
    which = -1;

    for(i = 0; i < MAX_READINGS; i++)
      if(front_laser.timestamp + front_laser_skew >= 
	 carmen_robot_odometry[i].timestamp + odometry_skew)
	which = i;

    if(which >= 0 && which < MAX_READINGS - 1) {
      fraction = ((front_laser.timestamp + front_laser_skew) -
		  (carmen_robot_odometry[which].timestamp + odometry_skew))
	/ (carmen_robot_odometry[which + 1].timestamp - 
	   carmen_robot_odometry[which].timestamp);

      construct_laser_message(&robot_front_laser, frontlaser_offset, 
				       which, fraction, 0);

      if(front_laser_count > ESTIMATES_CONVERGE || !carmen_robot_converge) {
	/* publish odometry corrected laser message */
	publish_frontlaser_message(robot_front_laser);
	fprintf(stderr, "f");
      }
      front_laser_ready = 0;
    }
  }

  if (rear_laser_ready) {
    if(strcmp(carmen_robot_host, rear_laser.host) == 0)
      rear_laser_skew = 0;
    else
      rear_laser_skew = carmen_running_average_report(REAR_LASER_AVERAGE);

    odometry_skew = carmen_robot_get_odometry_skew();
    which = -1;

    /* Find the index of the last odometry reading sent before this laser
       message. t gets stored in which. If we have received one *before* 
       this laser essage, but not one *after*, then don't do anything. */
    
    for(i = 0; i < MAX_READINGS; i++)
      if(rear_laser.timestamp + rear_laser_skew >= 
	 carmen_robot_odometry[i].timestamp + odometry_skew)
	which = i;

    /* If we don't have a matching laser reading, then return. */
    if (which >= 0 || which < MAX_READINGS - 1) {
      fraction = ((rear_laser.timestamp + rear_laser_skew) - 
		  (carmen_robot_odometry[which].timestamp + odometry_skew)) / 
	(carmen_robot_odometry[which + 1].timestamp - 
	 carmen_robot_odometry[which].timestamp);
      
      construct_laser_message(&robot_rear_laser, rearlaser_offset, 
				       which, fraction, 1);
      
      if(rear_laser_count > ESTIMATES_CONVERGE || !carmen_robot_converge) {
	/* publish odometry corrected laser message */
	publish_rearlaser_message(robot_rear_laser);
	fprintf(stderr, "r");
	rear_laser_ready = 0;
      }
    }
  }
}

static void 
check_message_data_chunk_sizes(carmen_laser_laser_message *laser_ptr)
{
  static int first_front = 1, first_rear = 1;
  int first;
  carmen_robot_laser_message robot_laser;
  carmen_laser_laser_message laser;

  if (laser_ptr == &front_laser)
    {
      first = first_front;
      robot_laser = robot_front_laser;
    }
  else
    {
      first = first_rear;
      robot_laser = robot_rear_laser;
    }

  laser = *laser_ptr;

  if(first) 
    {
      robot_laser.num_readings = laser.num_readings;
      robot_laser.range = 
      	(float *)calloc(robot_laser.num_readings, sizeof(float));
      carmen_test_alloc(robot_laser.range);
      robot_laser.tooclose = 
	(char *)calloc(robot_laser.num_readings, sizeof(char));
      carmen_test_alloc(robot_laser.tooclose);   
      
      first = 0;
    } 
  else if(robot_laser.num_readings != laser.num_readings) 
    {
      robot_laser.num_readings = laser.num_readings;
      robot_laser.range = 
	(float *)realloc(robot_laser.range, 
			 sizeof(float) * robot_laser.num_readings);
      carmen_test_alloc(robot_laser.range);
      robot_laser.tooclose = (char *)realloc
	(robot_laser.tooclose, sizeof(char) * robot_laser.num_readings);
      carmen_test_alloc(robot_laser.tooclose);
    }

  if (laser_ptr == &front_laser)
    {
      first_front = first;
      robot_front_laser = robot_laser;
    }
  else
    {
      first_rear = first;
      robot_rear_laser = robot_laser;
    }
}

static void 
laser_frontlaser_handler(void)
{
  int i;
  double safety_distance;
  double theta, delta_theta;
  carmen_traj_point_t robot_posn, obstacle_pt;
  double max_velocity, velocity;

  int tooclose = -1;
  double tooclose_theta = 0;

  int min_index;
  double min_theta = 0;
  double min_dist;
  
  int debug = carmen_carp_get_verbose();

  static double time_since_last_process = 0;
  double skip_sum = 0;

  /* We just got a new laser message. It may be that the new message contains
     a different number of laser readings than we were expecting, maybe
     because the laser server was restarted. Here, we check to make sure that
     our outgoing messages can handle the same number of readings as the
     newly-arrived incoming message. */

  check_message_data_chunk_sizes(&front_laser);

  front_laser_local_timestamp = carmen_get_time_ms();
  if(front_laser_count <= ESTIMATES_CONVERGE)
    front_laser_count++;

  carmen_running_average_add(FRONT_LASER_AVERAGE, 
			   front_laser_local_timestamp - 
			   front_laser.timestamp);    
  
  memcpy(robot_front_laser.range, front_laser.range, 
	 robot_front_laser.num_readings * sizeof(float));
  memset(robot_front_laser.tooclose, 0, robot_front_laser.num_readings);


  if (front_laser_local_timestamp - time_since_last_process < 
      1.0/carmen_robot_collision_avoidance_frequency)
    return;

  time_since_last_process = front_laser_local_timestamp;

  safety_distance = 
    carmen_robot_config.length / 2.0 + carmen_robot_config.approach_dist + 
    0.5 * carmen_robot_latest_odometry.tv * carmen_robot_latest_odometry.tv / 
    carmen_robot_config.acceleration +
    carmen_robot_latest_odometry.tv * carmen_robot_config.reaction_time;

  robot_front_laser.forward_safety_dist = safety_distance;
  robot_front_laser.side_safety_dist = 
    carmen_robot_config.width / 2.0 + carmen_robot_config.side_dist;
  robot_front_laser.turn_axis = 1e6;
  robot_front_laser.timestamp = front_laser.timestamp;
  strncpy(robot_front_laser.host, front_laser.host, 10);

  max_velocity = carmen_robot_config.max_t_vel;

  robot_posn.x = 0;
  robot_posn.y = 0;
  robot_posn.theta = 0;

  carmen_carp_set_verbose(0);
  
  theta = -M_PI/2; 
  delta_theta = M_PI/(robot_front_laser.num_readings-1);

  skip_sum = 0.0;
  for(i = 0; i < robot_front_laser.num_readings; i++, theta += delta_theta) {
    skip_sum += carmen_robot_laser_bearing_skip_rate;
    if (skip_sum > 0.95)
      {
	skip_sum = 0.0;
	robot_front_laser.tooclose[i] = -1;
	continue;
      }

    obstacle_pt.x = frontlaser_offset + 
      robot_front_laser.range[i] * cos(theta);
    obstacle_pt.y = robot_front_laser.range[i] * sin(theta);
    carmen_geometry_move_pt_to_rotating_ref_frame
      (&obstacle_pt, carmen_robot_latest_odometry.tv,
       carmen_robot_latest_odometry.rv);    
    velocity = carmen_geometry_compute_velocity
      (robot_posn, obstacle_pt, &carmen_robot_config);

    if (velocity < carmen_robot_config.max_t_vel) 
      {
	if (velocity < max_velocity)
	  {
	    tooclose = i;
	    tooclose_theta = theta;
	    max_velocity = velocity;
	  }
	robot_front_laser.tooclose[i] = 1;
    }
  } /* End of for(i = 0; i < robot_front_laser.num_readings; i++) */

  min_index = 0;
  min_dist = robot_front_laser.range[0];
  for(i = 1; i < robot_front_laser.num_readings; i++) { 
    if (robot_front_laser.range[i] < min_dist) {
      min_dist = robot_front_laser.range[i];
      min_index = i;
    }
  }

  carmen_carp_set_verbose(debug);
  if (max_velocity < carmen_robot_config.max_t_vel) {
    if (max_velocity > 0)
      carmen_verbose
	("velocity [46m%.1f[0m : distance %.4f delta_x %.4f (%.3f %d)\n", 
	 max_velocity,  robot_front_laser.range[min_index], 
	 frontlaser_offset + 
	 robot_front_laser.range[min_index]*cos(min_theta), 
	 cos(min_theta), min_index);
    else
	carmen_verbose
	  ("velocity [43m%.1f[0m : distance %.4f delta_x %.4f (%.3f %d)\n",
	   max_velocity, robot_front_laser.range[min_index], 
	   frontlaser_offset + 
	   robot_front_laser.range[min_index]*cos(min_theta), 
	   cos(min_theta), min_index);
  } else
    carmen_verbose("Ok\n");
  carmen_carp_set_verbose(0);

  front_laser_ready = 1;

  carmen_carp_set_verbose(debug);
  if (tooclose >= 0 && tooclose < robot_front_laser.num_readings) {
    if (max_velocity < carmen_robot_config.max_t_vel) {
      if (max_velocity > 0)
	carmen_verbose
	  ("velocity [42m%.1f[0m : distance %.4f delta_x %.4f (%.3f %d)\n",
	   max_velocity,  robot_front_laser.range[tooclose], 
	   frontlaser_offset + 
	   robot_front_laser.range[tooclose]*cos(tooclose_theta), 
	   cos(tooclose_theta), tooclose);
      else
	carmen_verbose
	  ("velocity [41m%.1f[0m : distance %.4f delta_x %.4f (%.3f %d)\n",
	   max_velocity, 
	   robot_front_laser.range[tooclose], 
	   frontlaser_offset + 
	   robot_front_laser.range[tooclose]*cos(tooclose_theta), 
	   cos(tooclose_theta), tooclose);
    } else
      carmen_verbose("Ok\n");
  } else {
    carmen_verbose("[44mNone too close[0m %.1f %.1f min_dist %.1f\n", 
		 carmen_robot_latest_odometry.tv, 
		 carmen_robot_latest_odometry.rv, min_dist );
  }
  carmen_carp_set_verbose(0);

  if (max_velocity >= 0 && max_velocity < MIN_ALLOWED_VELOCITY)
    max_velocity = 0.0;

  if(max_velocity <= 0.0 && carmen_robot_latest_odometry.tv > 0.0)
    {
      fprintf(stderr, "S");
      carmen_robot_stop_robot(ALLOW_ROTATE);
    }
  
  max_front_velocity = max_velocity;
  carmen_carp_set_verbose(debug);
}

double 
carmen_robot_laser_max_front_velocity(void) 
{
  return max_front_velocity;
}

double 
carmen_robot_laser_min_rear_velocity(void) 
{
  return min_rear_velocity;
}

static void 
laser_rearlaser_handler(void) 
{
  int i;
  double safety_distance;
  double theta, delta_theta;
  carmen_traj_point_t robot_posn, obstacle_pt;
  double min_velocity, velocity;

  int tooclose = -1;
  double tooclose_theta = 0;

  static double time_since_last_process = 0;
  double skip_sum = 0;

  /* We just got a new laser message. It may be that the new message contains
     a different number of laser readings than we were expecting, maybe
     because the laser server was restarted. Here, we check to make sure that
     our outgoing messages can handle the same number of readings as the
     newly-arrived incoming message. */

  check_message_data_chunk_sizes(&rear_laser);

  rear_laser_local_timestamp = carmen_get_time_ms();
  if(rear_laser_count <= ESTIMATES_CONVERGE)
    rear_laser_count++;

  carmen_running_average_add(REAR_LASER_AVERAGE, 
			   rear_laser_local_timestamp - 
			   rear_laser.timestamp);    
  
  memcpy(robot_rear_laser.range, rear_laser.range, 
	 robot_rear_laser.num_readings * sizeof(float));
  memset(robot_rear_laser.tooclose, 0, robot_rear_laser.num_readings);

  if (rear_laser_local_timestamp - time_since_last_process < 
      1.0/carmen_robot_collision_avoidance_frequency)
    return;

  time_since_last_process = rear_laser_local_timestamp;

  safety_distance = 
    carmen_robot_config.length / 2.0 + carmen_robot_config.approach_dist + 
    0.5 * carmen_robot_latest_odometry.tv * carmen_robot_latest_odometry.tv / 
    carmen_robot_config.acceleration +
    carmen_robot_latest_odometry.tv * carmen_robot_config.reaction_time;

  robot_rear_laser.forward_safety_dist = safety_distance;
  robot_rear_laser.side_safety_dist = 
    carmen_robot_config.width / 2.0 + carmen_robot_config.side_dist;
  robot_rear_laser.turn_axis = 1e6;
  robot_rear_laser.timestamp = rear_laser.timestamp;
  strncpy(robot_rear_laser.host, rear_laser.host, 10);

  min_velocity = -carmen_robot_config.max_t_vel;

  robot_posn.x = 0;
  robot_posn.y = 0;
  robot_posn.theta = 0;

  theta = -M_PI/2; 
  delta_theta = M_PI/(robot_rear_laser.num_readings-1);
  skip_sum = 0.0;
  for(i = 0; i < robot_rear_laser.num_readings; i++, theta += delta_theta) {
    skip_sum += carmen_robot_laser_bearing_skip_rate;
    if (skip_sum > 0.95)
      {
	skip_sum = 0.0;
	robot_rear_laser.tooclose[i] = -1;
	continue;
      }

    obstacle_pt.x = rearlaser_offset + robot_rear_laser.range[i] * cos(theta);
    obstacle_pt.y = robot_rear_laser.range[i] * sin(theta);
    carmen_geometry_move_pt_to_rotating_ref_frame
      (&obstacle_pt, carmen_robot_latest_odometry.tv,
       carmen_robot_latest_odometry.rv);    
    velocity = carmen_geometry_compute_velocity
      (robot_posn, obstacle_pt, &carmen_robot_config);    
    velocity = -velocity;

    if (velocity > -carmen_robot_config.max_t_vel) {
      if (velocity > min_velocity)
	{
	  tooclose = i;
	  tooclose_theta = theta;
	  min_velocity = velocity;
	}
      robot_rear_laser.tooclose[i] = 1;
    }
  } /* End of for(i = 0; i < robot_rear_laser.num_readings; i++) */

  rear_laser_ready = 1;

  if (min_velocity <= 0 && min_velocity > -.03)
    min_velocity = 0.0;

  if(min_velocity >= 0.0 && carmen_robot_latest_odometry.tv < 0.0)
    {
      fprintf(stderr, "S");
      carmen_robot_stop_robot(ALLOW_ROTATE);
    }
  
  min_rear_velocity = min_velocity;
}

void 
carmen_robot_add_laser_handlers(void) 
{
  carmen_laser_subscribe_frontlaser_message
    (&front_laser, (carmen_handler_t)laser_frontlaser_handler,
     CARMEN_SUBSCRIBE_LATEST);
  carmen_laser_subscribe_rearlaser_message
    (&rear_laser, (carmen_handler_t)laser_rearlaser_handler,
     CARMEN_SUBSCRIBE_LATEST);
  carmen_running_average_clear(FRONT_LASER_AVERAGE);
  carmen_running_average_clear(REAR_LASER_AVERAGE);
}

void 
carmen_robot_add_laser_parameters(char *progname) 
{
  int error;

  error = carmen_param_get_double("frontlaser_offset",&frontlaser_offset);
  carmen_param_handle_error(error, carmen_robot_usage, progname);
  error = carmen_param_get_double("rearlaser_offset", &rearlaser_offset);
  carmen_param_handle_error(error, carmen_robot_usage, progname);
}

