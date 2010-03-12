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

#include <sys/ioctl.h>
#include <values.h>
#include "cerebellum_com.h"
#include "walker_interface.h"


#define        CEREBELLUM_TIMEOUT          2.0

#define        METRES_PER_INCH        0.0254

#define        METRES_PER_CEREBELLUM_VELOCITY   .2    //(METRES_PER_INCH/10.0)
#define        METRES_PER_CEREBELLUM            (METRES_PER_INCH/10.0)

#define        WHEELBASE        (22.75 * METRES_PER_INCH)
#define        ROT_VEL_FACT_RAD (WHEELBASE/METRES_PER_CEREBELLUM_VELOCITY)

#define        MAXV                   (1.0/METRES_PER_CEREBELLUM_VELOCITY)

static char *dev_name;
static int State[4];
static int set_velocity = 0;
static int command_vl = 0, command_vr = 0;
static double last_command = 0;
static double last_update = 0;
static int timeout = 0;
static int moving = 0;
static double current_acceleration;
static double stopping_acceleration;
static carmen_robot_config_t robot_config;
static carmen_base_odometry_message odometry;
static int use_sonar = 0;

static double x, y, theta;

static int 
initialize_robot(char *dev)
{
  int result;
  double acc;

  result = carmen_cerebellum_connect_robot(dev);
  if(result != 0)
    return -1;

  acc = robot_config.acceleration/METRES_PER_CEREBELLUM_VELOCITY;
  //result = carmen_cerebellum_ac(acc);
  current_acceleration = robot_config.acceleration;

  if(result != 0)
    return 1;

  last_update = carmen_get_time_ms();

  return 1;
}

static void 
reset_status(void)
{
  char *host;
  odometry.x = 0.0;
  odometry.y = 0.0;
  odometry.theta = 0.0;
  odometry.tv = 0.0;
  odometry.rv = 0.0;
  odometry.timestamp = 0.0;

  host = carmen_get_tenchar_host_name();
  strcpy(odometry.host, host);   
}

static int
update_status(void)  /* This function takes approximately 60 ms to run */
{
  double vl, vr;
  static double last_published_update;
  static int initialized = 0;
  static short last_left_tick = 0, last_right_tick = 0;
  
  short left_delta_tick, right_delta_tick;
  double left_delta, right_delta;

  double inner_radius;
  double delta_angle;
  double delta_distance;
  double centre_x, centre_y;

  int button;

  // check remote state
  if (carmen_cerebellum_get_remote(&button) >= 0) {
    printf("button = %d\n", button);
    //if (button == 0)
     //carmen_walker_engage_clutch();
    //else
      //carmen_walker_disengage_clutch();
  }

  return 0;  //dbug

  if (last_published_update >= odometry.timestamp)
    return 0; 

  vl = ((double)State[2]) * METRES_PER_CEREBELLUM_VELOCITY;
  vr = ((double)State[3]) * METRES_PER_CEREBELLUM_VELOCITY;

  odometry.tv = 0.5 * (vl + vr);
  odometry.rv = (vl - vr) / ROT_VEL_FACT_RAD;
  
  if (!initialized)
    {
      last_left_tick = State[0];
      last_right_tick = State[1];
      initialized = 1;
      
      x = y = theta = 0;

      return 0;
    }

  /* update odometry message */

  left_delta_tick = State[0] - last_left_tick;
  if (left_delta_tick > SHRT_MAX/2)
    left_delta_tick += SHRT_MIN;
  if (left_delta_tick < -SHRT_MAX/2)
    left_delta_tick -= SHRT_MIN;
  left_delta = left_delta_tick*METRES_PER_CEREBELLUM;

  right_delta_tick = State[1] - last_right_tick;
  if (right_delta_tick > SHRT_MAX/2)
    right_delta_tick += SHRT_MIN;
  if (right_delta_tick < -SHRT_MAX/2)
    right_delta_tick -= SHRT_MIN;
  right_delta = right_delta_tick*METRES_PER_CEREBELLUM;

  if (fabs(right_delta - left_delta) < .001) 
    delta_angle = 0;
  else
    {
      if (fabs(left_delta) > 0) 
	{
	  inner_radius = (left_delta*WHEELBASE)/
	    (right_delta-left_delta);
	  
	  delta_angle = left_delta/inner_radius;  
	} 
      else
	{
	  inner_radius = (right_delta*WHEELBASE)/
	    (left_delta-right_delta);
	  
	  delta_angle = right_delta/inner_radius;
	} 
    }

  delta_distance = (left_delta + right_delta) / 2.0;
  
  if (0) //dbug
    {
      centre_x = x + inner_radius*cos(theta+M_PI/2.0);
      centre_y = y + inner_radius*sin(theta+M_PI/2.0);
      
      x = centre_x + inner_radius*cos(theta-M_PI/2.0+delta_angle);
      y = centre_x + inner_radius*sin(theta-M_PI/2.0+delta_angle);
      theta += delta_angle;
    }
  else 
    {
      x += delta_distance * cos(theta);
      y += delta_distance * sin(theta);
      theta += delta_angle;
    }

  last_left_tick = State[0];
  last_right_tick = State[1];
  
  last_published_update = odometry.timestamp;

  odometry.x = x;
  odometry.y = y;
  odometry.theta = theta;

  return 1;
}

static void 
command_robot(void)
{
  double time_elapsed = carmen_get_time_ms() - last_command;
  double current_time; 
  int acc;
  int error = 0;

  if (set_velocity)
    {
      if(command_vl == 0 && command_vr == 0) 
	{
	  moving = 0;
	  carmen_warn("S");
	}
      else 
	{
	  if (!moving)
	    {
	      acc = robot_config.acceleration/METRES_PER_CEREBELLUM_VELOCITY;
	      do 
		{
		  error = 0; //carmen_cerebellum_ac(acc);
		  if (error < 0)
		    carmen_cerebellum_connect_robot(dev_name);
		} 
	      while (error < 0);
	      current_acceleration = robot_config.acceleration;
	    }
	  carmen_warn("V");
	  moving = 1;
	}

      set_velocity = 0;
      timeout = 0;
      do 
	{
	  printf("---------------engaging clutch-------------------\n");
	  //carmen_cerebellum_engage_clutch(); //dbug
	  error = carmen_cerebellum_set_velocity(command_vl, command_vr);
	  if (error < 0)
	    carmen_cerebellum_connect_robot(dev_name);
	} 
      while (error < 0);

      last_command = carmen_get_time_ms();
    }
  else if(time_elapsed > CEREBELLUM_TIMEOUT && !timeout) 
    {
      carmen_warn("T");
      command_vl = 0;
      command_vr = 0;
      timeout = 1;
      moving = 0;
      do
	{
	  carmen_cerebellum_set_velocity(0, 0);      
	  if (error < 0)
	    carmen_cerebellum_connect_robot(dev_name);
	} 
      while (error < 0);
    }  
  else if (moving && current_acceleration == robot_config.acceleration)
    {
      acc = stopping_acceleration/METRES_PER_CEREBELLUM_VELOCITY;
      do 
	{
	  carmen_cerebellum_ac(acc);
	  if (error < 0)
	    carmen_cerebellum_connect_robot(dev_name);
	} 
      while (error < 0);
      current_acceleration = stopping_acceleration;
    }

  do
    {
      carmen_cerebellum_get_state(State+0, State+1, State+2, State+3);
      if (error < 0)
	carmen_cerebellum_connect_robot(dev_name);
    } 
  while (error < 0);
  
  current_time = carmen_get_time_ms();
  last_update = current_time;
  odometry.timestamp = current_time;

  if(use_sonar) 
    {
      carmen_warn("Sonar not supported by Cerebellum module\n");
      use_sonar = 0;
      carmen_param_set_variable("robot_use_sonar", "off", NULL);
    }
}

static int
read_cerebellum_parameters(int argc, char **argv)
{
  int num_items;

  carmen_param_t param_list[] = {
    {"cerebellum", "dev", CARMEN_PARAM_STRING, &dev_name, 0, NULL},
    {"robot", "use_sonar", CARMEN_PARAM_ONOFF, &use_sonar, 1, NULL}, 
    {"robot", "max_t_vel", CARMEN_PARAM_DOUBLE, &(robot_config.max_t_vel), 
     1, NULL},
    {"robot", "max_r_vel", CARMEN_PARAM_DOUBLE, &(robot_config.max_r_vel), 
     1, NULL},
    {"robot", "acceleration", CARMEN_PARAM_DOUBLE, 
     &(robot_config.acceleration), 1, NULL},
    {"robot", "deceleration", CARMEN_PARAM_DOUBLE, 
     &(stopping_acceleration), 1, NULL}};

  num_items = sizeof(param_list)/sizeof(param_list[0]);
  carmen_param_install_params(argc, argv, param_list, num_items);

  printf("dev = %s\n", dev_name);

  if(use_sonar)
    {
      carmen_warn("Sonar not supported by Cerebellum module\n");
      use_sonar = 0;
      carmen_param_set_variable("robot_use_sonar", "off", NULL);
    }

  if (robot_config.acceleration > stopping_acceleration) 
    carmen_die("ERROR: robot_deceleration must be greater or equal "
	       "than robot_acceleration\n");
  
  return 0;
}

static void 
velocity_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
		 void *clientData __attribute__ ((unused)))
{
  IPC_RETURN_TYPE err;
  carmen_base_velocity_message vel;
  double vl, vr;

  FORMATTER_PTR formatter;
  
  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &vel,
                     sizeof(carmen_base_velocity_message));
  IPC_freeByteArray(callData);

  carmen_test_ipc_return(err, "Could not unmarshall", 
			 IPC_msgInstanceName(msgRef));

  printf("tv = %.2lf, rv = %.2lf\n", vel.tv, vel.rv);

  vl = vel.tv / METRES_PER_CEREBELLUM_VELOCITY;
  vr = vel.tv / METRES_PER_CEREBELLUM_VELOCITY;  

  vl -= 0.5 * vel.rv * ROT_VEL_FACT_RAD;
  vr += 0.5 * vel.rv * ROT_VEL_FACT_RAD;

  if (vl > MAXV) {
    vr *= MAXV / vl;
    vl = MAXV;
  }
  else if (vl < -MAXV) {
    vr *= -MAXV / vl;
    vl = -MAXV;
  }
  if (vr > MAXV) {
    vl *= MAXV / vr;
    vr = MAXV;
  }
  else if (vr < -MAXV) {
    vl *= -MAXV / vr;
    vr = -MAXV;
  }

  printf("vl = %.2lf, vr = %.2lf\n", vl, vr);

  command_vl = (int)vl;
  command_vr = (int)vr;

  printf("command_vl = %d, command_vr = %d\n", command_vl, command_vr);

  set_velocity = 1;
}

int 
carmen_cerebellum_initialize_ipc(void)
{
  IPC_RETURN_TYPE err;

  /* define messages created by cerebellum */
  err = IPC_defineMsg(CARMEN_BASE_ODOMETRY_NAME, IPC_VARIABLE_LENGTH,
                      CARMEN_BASE_ODOMETRY_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_BASE_ODOMETRY_NAME);

  err = IPC_defineMsg(CARMEN_BASE_VELOCITY_NAME, IPC_VARIABLE_LENGTH,
                      CARMEN_BASE_VELOCITY_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_BASE_VELOCITY_NAME);

  /* setup incoming message handlers */

  err = IPC_subscribe(CARMEN_BASE_VELOCITY_NAME, velocity_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subscribe", CARMEN_BASE_VELOCITY_NAME);
  IPC_setMsgQueueLength(CARMEN_BASE_VELOCITY_NAME, 1);

  return IPC_No_Error;
}

int 
carmen_cerebellum_start(int argc, char **argv)
{
  if (read_cerebellum_parameters(argc, argv) < 0)
    return -1;

  if (carmen_cerebellum_initialize_ipc() < 0) 
    {
      carmen_warn("\nError: Could not initialize IPC.\n");
      return -1;
    }

  if (initialize_robot(dev_name) < 0)
    {
      carmen_warn("\nError: Could not connect to robot. "
      	  "Did you remember to turn the base on?\n");
      //return -1;
    }

  reset_status();

  strcpy(odometry.host, carmen_get_tenchar_host_name());

  return 0;
}

int 
carmen_cerebellum_run(void) 
{
  IPC_RETURN_TYPE err;

  if (update_status() == 1)
    {
      err = IPC_publishData(CARMEN_BASE_ODOMETRY_NAME, &odometry);
      carmen_test_ipc_exit(err, "Could not publish", 
			   CARMEN_BASE_ODOMETRY_NAME);
    }

  command_robot();
  
  return 1;
}

void 
carmen_cerebellum_shutdown(int x)
{
  if(x == SIGINT) 
    {
      carmen_verbose("\nShutting down robot...");
      sleep(1);
      carmen_cerebellum_set_velocity(0, 0);
      carmen_cerebellum_limp();
      last_update = carmen_get_time_ms();
      carmen_cerebellum_disconnect_robot();
      carmen_verbose("done.\n");
    }
}

void 
carmen_cerebellum_emergency_crash(int x __attribute__ ((unused)))
{
  carmen_cerebellum_set_velocity(0, 0);
  last_update = carmen_get_time_ms();
}
