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

/********************************************************
 *
 * The CARMEN Canon module was written by 
 * Mike Montemerlo (mmde+@cs.cmu.edu)
 * Chunks of code were taken from the Canon library that
 * is part of Gphoto2.  http://www.gphoto.com
 *
 ********************************************************/

#include <carmen/carmen.h>
#include "canon_messages.h"
#include "canon_ipc.h"
#include "canon.h"

usb_dev_handle *camera_handle;

void shutdown_module(int x)
{
  if(x == SIGINT) {
    canon_stop_capture(camera_handle);
    canon_close_camera(camera_handle);
    close_ipc();
    fprintf(stderr, "Closed connection to camera.\n");
    exit(-1);
  }
}

void alive_timer(void *clientdata __attribute__ ((unused)),
                 unsigned long t1 __attribute__ ((unused)),
                 unsigned long t2 __attribute__ ((unused)))
{
  IPC_RETURN_TYPE err;
  
  err = IPC_publishData(CARMEN_CANON_ALIVE_NAME, NULL);
  carmen_test_ipc_exit(err, "Could not publish", 
                       CARMEN_CANON_ALIVE_NAME);
}

int main(int argc __attribute__ ((unused)), char **argv)
{
  /* initialize IPC */
  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);
  initialize_ipc_messages();
  signal(SIGINT, shutdown_module);

  /* initialize the digital camera */
  camera_handle = canon_open_camera();
  if(camera_handle == NULL)
    carmen_die("Erorr: could not open connection to camera.\n");
  if(canon_initialize_camera(camera_handle) < 0)
    carmen_die("Erorr: could not open initialize camera.\n");
  if(canon_initialize_capture(camera_handle, FULL_TO_DRIVE | THUMB_TO_PC,
			      FLASH_AUTO) < 0)
    carmen_die("Error: could not start image capture.\n");
  if(canon_initialize_preview(camera_handle) < 0)
    carmen_die("Error: could not initialize preview mode.\n");

  /* run the main loop */
  fprintf(stderr, "Camera is ready to capture.\n");

  IPC_addTimer(1000, TRIGGER_FOREVER, alive_timer, NULL);
  IPC_dispatch();
  return 0;
}
