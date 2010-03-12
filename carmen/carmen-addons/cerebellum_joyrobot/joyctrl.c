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

#include <carmen/cerebellum_interface.h>
#include "joyctrl.h"


int carmen_initialize_joystick(carmen_joystick_type *joystick)
{
  if ((joystick->fd = open(JOYSTICK_DEVICE, O_RDONLY | O_NONBLOCK)) < 0){
    carmen_warn("Warning: could not initialize joystick.\n");
    joystick->initialized = 0;
    return -1;
  }
  ioctl(joystick->fd, JSIOCGAXES, &joystick->nb_axes);
  ioctl(joystick->fd, JSIOCGBUTTONS, &joystick->nb_buttons);

  joystick->axes = (int *)calloc(joystick->nb_axes, sizeof(int));
  carmen_test_alloc(joystick->axes);
  joystick->buttons = (int *)calloc(joystick->nb_buttons, sizeof(int));
  carmen_test_alloc(joystick->buttons);
  joystick->initialized = 1;
  
  return 0;
}

void carmen_set_deadspot(carmen_joystick_type *joystick, 
			 int on_off, double size)
{
  joystick->deadspot = on_off;
  joystick->deadspot_size = size;
}

int carmen_get_joystick_state(carmen_joystick_type *joystick)
{
  struct js_event mybuffer[64];
  int n, i;

  if (joystick->initialized == 0)
    return -1;

  n = read (joystick->fd, mybuffer, sizeof(struct js_event) * 64);
  if (n != -1) {
    for(i = 0; i < n / (signed int)sizeof(struct js_event); i++) {
      if(mybuffer[i].type & JS_EVENT_BUTTON &~ JS_EVENT_INIT) {
	joystick->buttons[mybuffer[i].number] = mybuffer[i].value;
      }
      else if(mybuffer[i].type & JS_EVENT_AXIS &~ JS_EVENT_INIT) {
	joystick->axes[mybuffer[i].number] = mybuffer[i].value;
	
	if(mybuffer[i].number == 0 || mybuffer[i].number == 1) {
	  if(joystick->deadspot) {
	    if(abs(joystick->axes[mybuffer[i].number]) < 
	       joystick->deadspot_size * 32767)
	      joystick->axes[mybuffer[i].number] = 0;
	    else if(joystick->axes[mybuffer[i].number] > 0)
	      joystick->axes[mybuffer[i].number] = 
		(joystick->axes[mybuffer[i].number] - 
		 joystick->deadspot_size * 32767)
		/ ((1-joystick->deadspot_size) * 32767) * 32767.0;
	    else if(joystick->axes[mybuffer[i].number] < 0)
	      joystick->axes[mybuffer[i].number] = 
		(joystick->axes[mybuffer[i].number] + 
		 joystick->deadspot_size * 32767)
		/ ((1-joystick->deadspot_size) * 32767) * 32767.0;
	  } else
	    joystick->axes[mybuffer[i].number] = 
	      joystick->axes[mybuffer[i].number];
	}
	if(mybuffer[i].number == 1 || mybuffer[i].number == 5)
	  joystick->axes[mybuffer[i].number] *= -1;
      }
    }
  }
  return n;
}

void carmen_close_joystick(carmen_joystick_type *joystick)
{
  if (joystick->initialized == 0)
    return;

  close(joystick->fd);
  free(joystick->axes);
  free(joystick->buttons);
}

void carmen_joystick_control(carmen_joystick_type *joystick, double max_tv, 
			     double max_rv, double *tv, double *rv,
			     int *tilt, int *fire)
{
  if (joystick->initialized == 0)
    return;
  
  float range;
  range = (float)(CEREBELLUM_TILT_MIN - CEREBELLUM_TILT_MAX);
  if( range < 0) range *= -1.0;

  *tv = joystick->axes[1] / 32767.0 * max_tv;
  *rv = -1 * joystick->axes[0] / 32767.0 * max_rv;
  *tilt =(int)( (joystick->axes[3]/ 32767.0 +1.0)/2.0 * range + (float) CEREBELLUM_TILT_MAX);
  *fire = joystick->buttons[0];
  carmen_verbose("axes[0]: %d axes[1]: %d tv: %f rv: %f\n", 
	       joystick->axes[0], joystick->axes[1], *tv, *rv);
  //  printf("axes[0]: %d axes[1]: %d axes[2]: %d axes[3]: %d\n",
  //	 joystick->axes[0], joystick->axes[1],joystick->axes[2],joystick->axes[3]); 

  //  printf("buttons[0]: %d buttons[1]: %d buttons[2]: %d\n",
  //	 joystick->buttons[0], joystick->buttons[1],joystick->buttons[2]); 

  //printf("calc tilt: %d\n", *tilt);
  //printf("calc fire: %d\n", *fire);
}
