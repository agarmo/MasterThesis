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
#include "cerebellum_main.h"

static void 
shutdown_cerebellum(int signo) 
{
  carmen_warn("Inside shutdown_cerebellum\n");
  carmen_cerebellum_shutdown(signo);
  close_ipc();
  exit(-1);
}

int 
main(int argc, char **argv)
{
  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);

  if (carmen_cerebellum_start(argc, argv) < 0)
    exit(-1);

  signal(SIGINT, shutdown_cerebellum);

  while(1) {
    sleep_ipc(0.01);
    carmen_cerebellum_run();
  }

  return 0;
}

