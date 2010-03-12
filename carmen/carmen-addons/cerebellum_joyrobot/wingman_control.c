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
#include <carmen/joystick.h>

static int deadzone;
static double deadzone_size;
static double min_max_tv = 0.0;// , min_max_rv = 0.0;
static double max_max_tv = 0.8, max_max_rv = 0.6;

static double max_allowed_tv = 1.5, max_allowed_rv = 1.5;

static int joystick_activated=0;
static int throttle_mode=0;

void send_base_velocity_command(double tv, double rv)
{
  IPC_RETURN_TYPE err;
  char *host;
  static carmen_base_velocity_message v;
  static int first = 1;

  if(first) {
    host = carmen_get_tenchar_host_name();
    strcpy(v.host, host);
    first = 0;
  }
  v.tv = tv;
  v.rv = rv;
  v.timestamp = carmen_get_time_ms();

  if (v.tv > max_allowed_tv)
    v.tv = max_allowed_tv;
  else if (v.tv < -max_allowed_tv)
    v.tv = -max_allowed_tv;

  if (v.rv > max_allowed_rv)
    v.rv = max_allowed_rv;
  else if (v.rv < -max_allowed_rv)
    v.rv = -max_allowed_rv;

  
  if (0)
    fprintf(stderr,"%.2f %.2f\n",v.tv, v.rv);
  
  err = IPC_publishData(CARMEN_BASE_VELOCITY_NAME, &v);
  carmen_test_ipc(err, "Could not publish", CARMEN_BASE_VELOCITY_NAME);  
}

void sig_handler(int x)
{
  if(x == SIGINT) {
    send_base_velocity_command(0, 0);
    close_joystick();
    close_ipc();
    printf("Disconnected from robot.\n");
    exit(0);
  }
}

void read_parameters(int argc, char **argv)
{
  int num_items;

  carmen_param_t param_list[] = {
    {"joystick", "deadspot", CARMEN_PARAM_ONOFF, &deadzone, 1, NULL},
    {"joystick", "deadspot_size", CARMEN_PARAM_DOUBLE, &deadzone_size, 1, NULL},
    {"robot", "max_t_vel", CARMEN_PARAM_DOUBLE, &max_max_tv, 1, NULL},
    {"robot", "max_r_vel", CARMEN_PARAM_DOUBLE, &max_max_rv, 1, NULL}
  };
  
  num_items = sizeof(param_list)/sizeof(param_list[0]);
  carmen_param_install_params(argc, argv, param_list, num_items);

  // Set joystick deadspot
  set_deadspot(deadzone, deadzone_size);
}

int main(int argc, char **argv)
{
  double command_tv = 0, command_rv = 0;
  int num_axes, num_buttons;
  int axes[20], buttons[20];
  double max_tv = 0, max_rv = 0;
  double f_timestamp;

  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);
  if(init_joystick(&num_axes, &num_buttons) < 0) 
    carmen_die("Erorr: could not find joystick.\n");

  if (num_axes != 9 || num_buttons != 12)
    fprintf(stderr,"This seems to be *NOT* a \"Logitech WingMan Cordless Rumble Pad\",\nbut I will start anyway (be careful!).\n\n");
  
  read_parameters(argc, argv);
  
  signal(SIGINT, sig_handler);
  
  fprintf(stderr,"1. Set the \"throttle control\" to zero (to the left)\n2. Press \"START\" button to activate the joystick.\n");

  if (throttle_mode)
    fprintf(stderr,"3. Use the \"throttle control\" to control the speed!\n\n");

  f_timestamp = carmen_get_time_ms();
  while(1) {
    sleep_ipc(0.1);
    if(get_joystick(axes, buttons) >= 0) {

      max_tv = min_max_tv + (-axes[2] + 32767.0) / (32767.0 * 2.0) * (max_max_tv - min_max_tv);

     //      max_rv = min_max_rv + (-axes[2] + 32767.0) / (32767.0 * 2.0) * (max_max_rv - min_max_rv);
      max_rv = max_max_rv;

      if (throttle_mode) {
	command_tv = max_tv;

	if(axes[6] && command_tv > 0)
	  command_tv *= -1;

	if (fabs(axes[1] / 32767.0) > 0.25   || 
	    fabs(axes[0] / 32767.0) > 0.25)  
	  command_tv = 0;
      }
      else 
	command_tv = +1 * axes[1] / 32767.0 * max_max_tv;

      if (axes[3]) 
	command_rv = -1 * axes[3] / 32767.0 * max_max_rv;
      else 
	command_rv = 0;
      

		
      //command_tv = -1 * axes[4] / 32767.0 * max_tv;
      //command_tv = +1 * axes[1] / 32767.0 * max_tv;

      if (joystick_activated)
	send_base_velocity_command(command_tv, command_rv);

      if (buttons[7] || buttons[9] || buttons[10] || buttons[6]) {
	throttle_mode = !throttle_mode;
	fprintf(stderr,"throttle control is %s! ", (throttle_mode?"ON":"OFF"));
	if (throttle_mode)
	  fprintf(stderr,"Use the \"throttle control\" to control the speed!\n");
	else 
	  fprintf(stderr,"\n");
      }

      if (buttons[8]) {
	joystick_activated = !joystick_activated;
	if (joystick_activated)
	  fprintf(stderr,"Joystick activated!\n");
	else {
	  fprintf(stderr,"Joystick deactivated!\n");
	  send_base_velocity_command(0, 0);
	}
      }
    }
    else if(joystick_activated && carmen_get_time_ms() - f_timestamp > 0.5) {
      send_base_velocity_command(command_tv, command_rv);
      f_timestamp = carmen_get_time_ms();
    }
  }
  sig_handler(SIGINT);
  return 0;
}
