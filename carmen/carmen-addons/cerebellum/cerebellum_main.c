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
#include "cerebellum_interface.h"

#undef _REENTRANT

#ifdef _REENTRANT
#include <pthread.h>
static pthread_t main_thread;
static int thread_is_running = 0;
#endif


// Normalize angle to domain -pi, pi
//
#define NORMALIZE(z) atan2(sin(z), cos(z))

// might need to define a longer delay to wait for acks
#define TROGDOR_DELAY_US 10000

/************************************************************************/
/* Physical constants, in meters, radians, seconds (unless otherwise noted)
 * */
//#define TROGDOR_AXLE_LENGTH    0.317
//#define TROGDOR_WHEEL_DIAM     0.10795  /* 4.25 inches */
//#define TROGDOR_WHEEL_CIRCUM   (TROGDOR_WHEEL_DIAM * M_PI)
//#define TROGDOR_TICKS_PER_REV  11600.0
//#define TROGDOR_M_PER_TICK     (TROGDOR_WHEEL_CIRCUM / TROGDOR_TICKS_PER_REV)
/* the internal PID loop runs every 1.55ms (we think) */
//#define TROGDOR_PID_FREQUENCY  (1/1.55e-3)
//#define TROGDOR_MPS_PER_TICK   (TROGDOR_M_PER_TICK * TROGDOR_PID_FREQUENCY)
/* assuming that the counts can use the full space of a signed 32-bit int
 * */
//#define TROGDOR_MAX_TICS 2147483648U
/* for safety */
//#define TROGDOR_MAX_WHEELSPEED   4.0

#define CEREBELLUM_TIMEOUT (2.0)
#define METERS_PER_INCH (0.0254)
#define INCH_PER_METRE (39.370)
#define OBOT_LOOP_FREQUENCY (1/1.9375e-3)
#define OBOT_WHEEL_CIRCUMFERENCE (4.25 * METERS_PER_INCH * 3.1415926535)
#define OBOT_TICKS_PER_REV (11600.0)
#define OBOT_WHEELBASE (.317)
#define METERS_PER_OBOT_TICK (OBOT_WHEEL_CIRCUMFERENCE / OBOT_TICKS_PER_REV)
#define TICKS_PER_MPS (1.0/(METERS_PER_OBOT_TICK * OBOT_LOOP_FREQUENCY))
#define ROT_VEL_FACT_RAD (OBOT_WHEELBASE*TICKS_PER_MPS)
#define MAX_CEREBELLUM_ACC 10
#define MIN_OBOT_TICKS (4.0)
#define MAX_READINGS_TO_DROP (10)

static char *dev_name;
static int State[4];
static int set_velocity = 0;
static int fire_gun = 0;
static int tilt_gun = 0;
static int tilt_gun_angle = 0;
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
static double voltage;
static double voltage_time_elapsed = 0;

static int temperature_fault;
static int left_temperature,right_temperature;
static double temperature_time_elapsed = 0;

static int
initialize_robot(char *dev)
{
  int result;
  double acc;

  carmen_terminal_cbreak(0);

  result = carmen_cerebellum_connect_robot(dev);
  //  if(result != 0)
  //  return -1;

  acc = robot_config.acceleration*TICKS_PER_MPS;

  // SNEAKY HACK
  result = carmen_cerebellum_ac(25);
  current_acceleration = robot_config.acceleration;

  //  printf("TROGDOR_M_PER_TICK: %lf\n", TROGDOR_M_PER_TICK);
  //printf("METERS_PER_OBOT_TICK: %lf\n", METERS_PER_OBOT_TICK);
  //printf("TROGDOR_MPS_PER_TICK: %lf\n",TROGDOR_MPS_PER_TICK);
  //printf("1/TICKS_PER_MPS: %lf\n",1.0/TICKS_PER_MPS);
  
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
  static double last_published_update;
  static int initialized = 0;
  static short last_left_tick = 0, last_right_tick = 0;
  static int transient_dropped = 0;
  
  short left_delta_tick, right_delta_tick;
  double left_delta, right_delta;
  double delta_angle;
  double delta_distance;
  double delta_time;

  int left_ticks = State[1];
  int right_ticks = State[0]; 

  //make sure that this measurement doesn't come from some time in the future
  delta_time = odometry.timestamp - last_published_update;
  if (delta_time < 0)
    {
      return 0; 
      carmen_warn("Invalid odometry timestamp.\n");
    }


  //sometimes the robot returns 0 for left tic or right tic
  //this is an error, if that is the case, the reading is ignored
  if(left_ticks == 0 || right_ticks == 0)
    {
      carmen_warn("0 odometry returned, ignoring it.\n");
      return(0);
    }

  if (!initialized)
    {
      last_left_tick = left_ticks;
      last_right_tick = right_ticks;
      initialized = 1;
      x = y = theta = 0;
      return 0;
    }

  /* update odometry message */
  left_delta_tick = left_ticks - last_left_tick;
  //this will turn a rollover into a small number
  if (left_delta_tick > SHRT_MAX/2)
    left_delta_tick += SHRT_MIN;
  if (left_delta_tick < -SHRT_MAX/2)
    left_delta_tick -= SHRT_MIN;
  left_delta = left_delta_tick*METERS_PER_OBOT_TICK;

  right_delta_tick = right_ticks - last_right_tick;
  //these ifs will turn a rollover into a small number
  if (right_delta_tick > SHRT_MAX/2)
    right_delta_tick += SHRT_MIN;
  if (right_delta_tick < -SHRT_MAX/2)
    right_delta_tick -= SHRT_MIN;
  right_delta = right_delta_tick*METERS_PER_OBOT_TICK;

  //compute translational velocity
  delta_distance = (right_delta + left_delta) / 2.0;
  odometry.tv = delta_distance / delta_time;

  //no wheel should go faster than 3.0 m/s
  if( (right_delta/delta_time) > 3.0 || (left_delta/delta_time) > 3.0)
    {
      if( transient_dropped < MAX_READINGS_TO_DROP )
	{
	  printf("CBldt: %d rdt: %d\n",left_delta_tick,right_delta_tick); 
	  printf("delta tick unreal trying a drop\n");
	  transient_dropped++;
	  return(1);
	}
      else
	{
	  printf("accepting large tick reading\n");
	  transient_dropped =0;
	}
    }

  //compute rotational velocity
  if (fabs(right_delta - left_delta) < .001) 
    delta_angle = 0;
  else
    delta_angle = (right_delta-left_delta)/OBOT_WHEELBASE;
  odometry.rv = delta_angle / delta_time;

  //the robot cannot rotate faster than 10 radians/s
  if(odometry.rv > 10)
    {
      if( transient_dropped < MAX_READINGS_TO_DROP)
	{
	  printf("CB:delta_angle: %lf, lt: %d rt: %d, olt: %d ort: %d",
		 delta_angle,left_ticks,right_ticks,last_left_tick,last_right_tick);
	  printf("dropping one reading\n");
	  transient_dropped++;
	  return(1);
	}
      else
	{
	  printf("accepting large delta reading\n");
	  transient_dropped = 0;
	}
    }

  //update internal odometry
  x += delta_distance * cos(theta);
  y += delta_distance * sin(theta);
  theta += delta_angle;
  theta = NORMALIZE(theta);

  //update odometry message
  odometry.x = x;
  odometry.y = y;
  odometry.theta = theta;

  //printf("OD t:%f x:%f, y:%f\n",theta,x,y);
  //printf("TKS l:%d r:%d\n",left_ticks, right_ticks);
  
  //keep history to compute velocities, the velocities the robot reports
  //are bogus
  last_left_tick = left_ticks;
  last_right_tick = right_ticks;
  last_published_update = odometry.timestamp;
  
  //the transient dropped flag lets us drop one and only one anomalous reading
  if( transient_dropped > 0)
    transient_dropped--;

  return 1;
}

static void 
command_robot(void)
{
  double time_elapsed = carmen_get_time_ms() - last_command;
  double update_time_elapsed = carmen_get_time_ms() - last_update;
  double current_time; 
  int acc;
  int error = 0;

  char hit,where;

  if(fire_gun)
    {
      carmen_cerebellum_fire();
      fire_gun = 0;
    }

  if(tilt_gun)
    {
      carmen_cerebellum_tilt(tilt_gun_angle);
    }

  //get the shroud every loop... will this cause problems??
  error = carmen_cerebellum_get_shroud(&hit,&where);
  if(error) printf("ERROR IN GET SHROUD\n");
  else printf("Get Shroud returns hit: %d where: %d\n", (int)hit,(int)where);


  if (set_velocity)
    {
      if(command_vl == 0 && command_vr == 0) 
	{
	  moving = 0;
	  //	  printf("carmen says S\n");
	  carmen_warn("S");
	}
      else 
	{
	  //	  printf("if not moving setting acceleration1\n");
	  if (!moving)
	    {
	      acc = robot_config.acceleration*TICKS_PER_MPS;
	      acc = MAX_CEREBELLUM_ACC;
	      do 
		{
		  error = carmen_cerebellum_ac(acc);
		  if (error < 0)
		    carmen_cerebellum_reconnect_robot(dev_name);
		} 
	      while (error < 0);
	      current_acceleration = robot_config.acceleration;
	    }
	  //	  printf("if not moving setting acceleration2\n");

	  carmen_warn("V");
	  moving = 1;
	}

      set_velocity = 0;
      timeout = 0;
      do 
	{
	  //	  printf("setting velocity1\n");

	  error = carmen_cerebellum_set_velocity(command_vl, command_vr);
	  if (error < 0)
	    carmen_cerebellum_reconnect_robot(dev_name);
	  //	  printf("setting velocity2\n");

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
	  //	  printf("timeout1\n");

	  error=carmen_cerebellum_set_velocity(0, 0);      
	  if (error < 0)
	    carmen_cerebellum_reconnect_robot(dev_name);
	  //	  printf("timeout2\n");

	} 
      while (error < 0);
    }  
  else if (moving && current_acceleration == robot_config.acceleration)
    {
      acc = stopping_acceleration*TICKS_PER_MPS;
      do 
	{
	  //	  printf("stopping1\n");

	  error=carmen_cerebellum_ac(acc);
	  if (error < 0)
	    carmen_cerebellum_reconnect_robot(dev_name);
	  //	  printf("stopping2\n");

	} 
      while (error < 0);
      current_acceleration = stopping_acceleration;
    }

  do
    {
      //printf("getting state1\n");
      error=carmen_cerebellum_get_state(State+0, State+1, State+2, State+3);
      if (error < 0)
	carmen_cerebellum_reconnect_robot(dev_name);
      //printf("getting state2\n");

    } 
  while (error < 0);

  //increment the voltage check timer, check it every 3 seconds
  voltage_time_elapsed += update_time_elapsed;
  //  printf("time_elapsed: %lf\n",update_time_elapsed);
  if( voltage_time_elapsed > .3)
    {
      printf("V: ");
      error = carmen_cerebellum_get_voltage(&voltage);
      if( error < 0)
	{
	  printf("error, voltage command didn't work\n");
	}
      else
	printf("voltage: %lf\n",voltage);

      voltage_time_elapsed=0;

    }

  //increment the temperature check timer, check it every 3 seconds
  temperature_time_elapsed += update_time_elapsed;
  if( temperature_time_elapsed > .6)
    {
            printf("C: ");
      error = carmen_cerebellum_get_temperatures(&temperature_fault,
      						&left_temperature,
      						&right_temperature);
      if( error < 0)
	{
	  printf("error, temperature command didn't work\n");
	}
      else
	printf("temp: fault:%d L:%d R:%d\n", temperature_fault,
	       left_temperature,right_temperature);

      temperature_time_elapsed=0;


    }
 

  
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
  double max_wheel, min_wheel;

  FORMATTER_PTR formatter;
  
  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &vel,
                     sizeof(carmen_base_velocity_message));
  IPC_freeByteArray(callData);

  carmen_test_ipc_return(err, "Could not unmarshall", 
			 IPC_msgInstanceName(msgRef));

  /*   vl -= 0.5 * vel.rv * ROT_VEL_FACT_RAD; */
  /*   vr += 0.5 * vel.rv * ROT_VEL_FACT_RAD; */

  vl = vel.tv - 0.5 * vel.rv * OBOT_WHEELBASE;
  vr = vel.tv + 0.5 * vel.rv * OBOT_WHEELBASE;

  /* we want rotational velocity to dominate translational
     velocity in the case that the user specifies a 
     velocity that we can't actually meet */
  max_wheel = vl > vr ? vl : vr;
  min_wheel = vr > vl ? vl : vr;
  if( max_wheel > robot_config.max_t_vel )
    {
      vl -= max_wheel-robot_config.max_t_vel;
      vr -= max_wheel-robot_config.max_t_vel;
    }
  if( min_wheel < -robot_config.max_t_vel )
    {
      vl -= min_wheel+robot_config.max_t_vel;
      vr -= min_wheel+robot_config.max_t_vel;
    }

  if(vl > robot_config.max_t_vel)
    vl = robot_config.max_t_vel;
  else if(vl < -robot_config.max_t_vel)
    vl = -robot_config.max_t_vel;
  if(vr > robot_config.max_t_vel)
      vr = robot_config.max_t_vel;
  else if(vr < -robot_config.max_t_vel)
    vr = -robot_config.max_t_vel;

  vl *= TICKS_PER_MPS;
  vr *= TICKS_PER_MPS;

  //we put in a minimum ticks per loop due to a bad motor controller
  //note that if we set one to the minimum we should increase
  //the other speed in proportion
  if( vl> 0.0 && vl < MIN_OBOT_TICKS)
    {
      vr = vr/vl * MIN_OBOT_TICKS;
      vl = MIN_OBOT_TICKS;
    }
  if( vl< 0.0 && vl > -MIN_OBOT_TICKS)
    {
      vr = vr/(fabs(vl)) * MIN_OBOT_TICKS;
      vl =-MIN_OBOT_TICKS;
    }

  if( vr> 0.0 && vr < MIN_OBOT_TICKS)
    {
      vl = vl/vr * MIN_OBOT_TICKS;
      vr = MIN_OBOT_TICKS;
    }

  if( vr< 0.0 && vr > -MIN_OBOT_TICKS)
    {
      vl = vl/(fabs(vr)) * MIN_OBOT_TICKS;

      vr =-MIN_OBOT_TICKS;

    }
  command_vl = (int)vl;
  command_vr = (int)vr;

  //printf("To_int: %d %d\r\n",command_vl, command_vr);

  set_velocity = 1;
}


static void 
gun_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
		 void *clientData __attribute__ ((unused)))
{
  IPC_RETURN_TYPE err;
  CerebellumFireGunMessage v;

  FORMATTER_PTR formatter;
  
  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &v,
                     sizeof(CerebellumFireGunMessage));
  IPC_freeByteArray(callData);

  carmen_test_ipc_return(err, "Could not unmarshall", 
			 IPC_msgInstanceName(msgRef));

  printf("got a fire gun message\r\n");

  fire_gun = 1;
}

static void 
tilt_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
		 void *clientData __attribute__ ((unused)))
{
  IPC_RETURN_TYPE err;
  CerebellumTiltGunMessage v;

  FORMATTER_PTR formatter;
  
  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &v,
                     sizeof(CerebellumTiltGunMessage));
  IPC_freeByteArray(callData);

  carmen_test_ipc_return(err, "Could not unmarshall", 
			 IPC_msgInstanceName(msgRef));

  printf("got a tilt gun message t: %d\r\n", v.tilt_angle);

  tilt_gun = 1;
  tilt_gun_angle = v.tilt_angle;
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


  err = IPC_defineMsg(CEREBELLUM_FIRE_GUN_MESSAGE_NAME, IPC_VARIABLE_LENGTH,
                      CEREBELLUM_FIRE_GUN_MESSAGE_FMT);
  carmen_test_ipc_exit(err, "Could not define", CEREBELLUM_FIRE_GUN_MESSAGE_NAME);

  err = IPC_defineMsg(CEREBELLUM_TILT_GUN_MESSAGE_NAME, IPC_VARIABLE_LENGTH,
                      CEREBELLUM_TILT_GUN_MESSAGE_FMT);
  carmen_test_ipc_exit(err, "Could not define", CEREBELLUM_TILT_GUN_MESSAGE_NAME);

  /* setup incoming message handlers */

  err = IPC_subscribe(CARMEN_BASE_VELOCITY_NAME, velocity_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subscribe", CARMEN_BASE_VELOCITY_NAME);
  IPC_setMsgQueueLength(CARMEN_BASE_VELOCITY_NAME, 1);
  
  err = IPC_subscribe(CEREBELLUM_FIRE_GUN_MESSAGE_NAME, gun_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subscribe",CEREBELLUM_FIRE_GUN_MESSAGE_NAME );
  IPC_setMsgQueueLength(CEREBELLUM_FIRE_GUN_MESSAGE_NAME, 1);

  err = IPC_subscribe(CEREBELLUM_TILT_GUN_MESSAGE_NAME, tilt_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subscribe",CEREBELLUM_TILT_GUN_MESSAGE_NAME );
  IPC_setMsgQueueLength(CEREBELLUM_TILT_GUN_MESSAGE_NAME, 1);

  return IPC_No_Error;
}

#ifdef _REENTRANT
static void *
start_thread(void *data __attribute__ ((unused))) 
{
  sigset_t set;

  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGPIPE);
  sigprocmask(SIG_BLOCK, &set, NULL);

  while (1) {    
    command_robot();
    usleep(30000);
  }
  return(NULL);
}  
#endif


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

  if(initialize_robot(dev_name) < 0)
    {
      carmen_warn("\nError: Could not connect to robot. "
		  "Did you remember to turn the base on?\n");
      return -1;
    }

  reset_status();

  strcpy(odometry.host, carmen_get_tenchar_host_name());

#ifdef _REENTRANT
  pthread_create(&main_thread, NULL, start_thread, NULL);
  thread_is_running = 1;
#endif
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
#ifndef _REENTRANT
  command_robot();
#endif
  
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
