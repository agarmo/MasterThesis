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
#include <carmen/cerebellum_interface.h>
#include "joystick.h"
#include "joyctrl.h"

static int no_joystick = 1;
static carmen_joystick_type joystick;
static int deadzone;
static double deadzone_size;
static double max_tv, max_rv;

void sig_handler(int x)
{
  if(x == SIGINT) {
    carmen_robot_velocity_command(0, 0);
    if(!no_joystick)
      carmen_close_joystick(&joystick);
    close_ipc();
    printf("Disconnected from robot.\n");
    exit(0);
  }
}

void read_parameters(int argc, char **argv)
{
  int num_items;

  carmen_param_t param_list[] = {
    {"robot", "max_t_vel", CARMEN_PARAM_DOUBLE, &max_tv, 1, NULL},
    {"robot", "max_r_vel", CARMEN_PARAM_DOUBLE, &max_rv, 1, NULL},
    {"joystick", "deadspot", CARMEN_PARAM_ONOFF, &deadzone, 1, NULL},
    {"joystick", "deadspot_size", CARMEN_PARAM_DOUBLE, &deadzone_size, 1, NULL}};

  num_items = sizeof(param_list)/sizeof(param_list[0]);
  carmen_param_install_params(argc, argv, param_list, num_items);

  // Set joystick deadspot
  carmen_set_deadspot(&joystick,deadzone, deadzone_size);
}

int main(int argc, char **argv)
{
  double command_tv = 0.0, command_rv = 0.0;
  int tilt=0;
  char c;
  int quit = 0,fire=0;
  double f_timestamp;

  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);

  // Initialize keybord, joystick and IPC
  carmen_initialize_keyboard();
  if (carmen_initialize_joystick(&joystick) >= 0) 
    no_joystick = 0;

  read_parameters(argc, argv);

  signal(SIGINT, sig_handler);
  
  f_timestamp = carmen_get_time_ms();
  while(quit>=0) {
    sleep_ipc(0.05);
    if(!no_joystick && carmen_get_joystick_state(&joystick) >= 0) {
      carmen_joystick_control(&joystick, max_tv, max_rv,
			      &command_tv, &command_rv, &tilt,&fire);
      carmen_robot_velocity_command(command_tv, command_rv);
      carmen_cerebellum_tilt_command(tilt);
      if(fire)
	carmen_cerebellum_fire_command(fire);

      f_timestamp = carmen_get_time_ms();
    }
    else if(carmen_read_char(&c)) {
      quit = carmen_keyboard_control(c, max_tv, max_rv,
				     &command_tv, &command_rv);
      carmen_robot_velocity_command(command_tv, command_rv);
      f_timestamp = carmen_get_time_ms();
    }
    else if(carmen_get_time_ms() - f_timestamp > 0.5) {
      carmen_robot_velocity_command(command_tv, command_rv);
      f_timestamp = carmen_get_time_ms();
    }
  }
  sig_handler(SIGINT);
  return 0;
}
