#include <carmen/carmen.h>
#include <carmen/carmen.h>
#include "hokuyo.h"
#include "hokuyo_dev.h"
#include "hokuyo_ipc.h"
#include "hokuyo_messages.h"
#include "hokuyo_main.h"

int main(int argc, char **argv) 
{
  /* connect to IPC server */
  carmen_ipc_initialize(argc,argv);
  carmen_param_check_version(argv[0]);

  if(carmen_hokuyo_start(argc, argv) < 0)
    exit(-1);
  signal(SIGINT, shutdown_hokuyo);
  
  while(1) {
    carmen_hokuyo_run();
    carmen_ipc_sleep(0.1);
  }
  return 0;
}
