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
 * any later version. *
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
#include "hokuyo_interface.h"

carmen_hokuyo_pb9_message hokuyo;

double first_timestamp;
int hokuyo_count = 0;

void hokuyo_handler(void)
{
  int i=0;
  for (i=0; i<hokuyo.num_readings; i++){
  	printf("%2f ",hokuyo.range[i]);
  }
  printf("\n");
  fflush(stdout);
  hokuyo_count++;
}

void shutdown_module(int x __attribute__ ((unused)))
{
  carmen_ipc_disconnect();
  printf("\nDisconnected from device.\n");
  exit(1);  
}

void print_statistics(void *clientdata __attribute__ ((unused)), 
		      unsigned long t1 __attribute__ ((unused)), 
		      unsigned long t2 __attribute__ ((unused)))
{
  double current_timestamp = carmen_get_time();

  fprintf(stderr, "H - %.2f Hz\n", 
	  hokuyo_count / (current_timestamp - first_timestamp));
}

int main(int argc , char **argv)
{
  carmen_ipc_initialize(argc, argv);
  carmen_param_check_version(argv[0]);
  signal(SIGINT, shutdown_module);
 
  carmen_hokuyo_subscribe_pb9_message(&hokuyo, 
				      (carmen_handler_t)hokuyo_handler, 
				      CARMEN_SUBSCRIBE_LATEST);

  IPC_addTimer(1000, TRIGGER_FOREVER, print_statistics, NULL);

  first_timestamp = carmen_get_time();
  IPC_dispatch();
  return 0;
}
