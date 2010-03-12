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

#include <math.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <linux/joystick.h>

#define         JOYSTICK_DEVICE         "/dev/js0"

int joystick_fd;
unsigned char anum, bnum;
int *axes, *buttons;

static int deadspot;
static double deadspot_size;

int init_joystick(int *a, int *b)
{
  int i;

  if((joystick_fd = open(JOYSTICK_DEVICE, O_RDONLY | O_NONBLOCK)) < 0) {
    perror("init_joystick");
    return -1;
  }
  ioctl(joystick_fd, JSIOCGAXES, &anum);
  ioctl(joystick_fd, JSIOCGBUTTONS, &bnum);
  *a=anum;
  *b=bnum;

  axes = (int *)calloc(anum, sizeof(int));  /* check_alloc checked */
  if (axes == NULL) {
    fprintf(stderr, "Out of memory in %s (%s, line %d).\n", __FUNCTION__,
	    __FILE__, __LINE__);
    exit(-1);
  }

  buttons = (int *)calloc(bnum, sizeof(int));  /* check_alloc checked */
  if (buttons == NULL) {
    fprintf(stderr, "Out of memory in %s (%s, line %d).\n", __FUNCTION__,
	    __FILE__, __LINE__);
    exit(-1);
  }
  for(i = 0; i < anum; i++)
    axes[i] = 0;
  for(i = 0; i < bnum; i++)
    buttons[i] = 0;
  return 0;
}

void close_joystick(void)
{
  close(joystick_fd);
  free(axes);
  free(buttons);
}

void set_deadspot(int on_off, double size) {
  deadspot = on_off;
  deadspot_size = size;
}

int get_joystick(int *a, int *b)
{
  struct js_event mybuffer[64];
  int n, i;

  n = read (joystick_fd, mybuffer, sizeof(struct js_event) * 64);
  if (n != -1) {
    for(i = 0; i < n / (signed int)sizeof(struct js_event); i++) {
      if(mybuffer[i].type & JS_EVENT_BUTTON &~ JS_EVENT_INIT) {
	buttons[mybuffer[i].number] = mybuffer[i].value;
      }
      else if(mybuffer[i].type & JS_EVENT_AXIS &~ JS_EVENT_INIT) {
	axes[mybuffer[i].number] = mybuffer[i].value;
	
	if(mybuffer[i].number == 0 || mybuffer[i].number == 1) {
	  if (deadspot) {
	    if(abs(axes[mybuffer[i].number]) < deadspot_size * 32767)
	      axes[mybuffer[i].number] = 0;
	    else if(axes[mybuffer[i].number] > 0)
	      axes[mybuffer[i].number] = 
		(axes[mybuffer[i].number] - deadspot_size * 32767)
		/ ((1-deadspot_size) * 32767) * 32767.0;
	    else if(axes[mybuffer[i].number] < 0)
	      axes[mybuffer[i].number] = 
		(axes[mybuffer[i].number] + deadspot_size * 32767)
		/ ((1-deadspot_size) * 32767) * 32767.0;
	  } else {
	    axes[mybuffer[i].number] = axes[mybuffer[i].number];
	  }
	}
	if(mybuffer[i].number == 1 || mybuffer[i].number == 5)
	  axes[mybuffer[i].number] *= -1;
      }
    }
  }

  for(i = 0; i < anum; i++)
    a[i] = axes[i];
  for(i = 0; i < bnum; i++)
    b[i] = buttons[i];

  return n;
}
