#include <carmen/carmen.h>
#include "hokuyo.h"
#include "hokuyo_dev.h"
#include "hokuyo_ipc.h"
#include "hokuyo_messages.h"

hokuyo_t hokuyo;
int quit_signal = 0;

void set_default_parameters(hokuyo_p hokuyo)
{
  strcpy(hokuyo->device_name, "/dev/ttyS0");
  hokuyo->flipped = 0;
}


void read_parameters(int argc, char **argv)
{
  char *dev="";

  carmen_param_t hokuyo_dev =  {"hokuyo", "hokuyo_dev", CARMEN_PARAM_STRING, &dev, 0, NULL};
  strcpy(hokuyo.device_name, dev);
  carmen_param_t hokuyo_params={"hokuyo", "hokuyo_flipped", CARMEN_PARAM_INT, 
     &hokuyo.flipped, 0, NULL};
  carmen_param_install_params(argc, argv, &hokuyo_dev, 1);
  carmen_param_install_params(argc, argv, &hokuyo_params,1);
  strcpy(hokuyo.device_name,dev);
}

int carmen_hokuyo_start(int argc, char **argv)
{
  /* initialize laser messages */
  ipc_initialize_messages();

  /* get laser parameters */
  set_default_parameters(&hokuyo);  
  read_parameters(argc, argv);
  printf("starting device\n");
  hokuyo_start(&hokuyo);
  return 0;
}

void carmen_hokuyo_shutdown(int signo __attribute__ ((unused)))
{
    hokuyo_shutdown(&hokuyo);
}

int carmen_hokuyo_run(void)
{
  static int first = 1;
  static double last_update;
  static double last_alive = 0;
  double current_time;
  int print_stats;
  static int stalled = 0;

  if(first) {
    last_update = carmen_get_time();
    first = 0;
  }
  current_time = carmen_get_time();
  print_stats = (current_time - last_update > 1.0);
    
  hokuyo_run(&hokuyo);
  if(hokuyo.new_reading)
    publish_hokuyo_message(&hokuyo);   
  stalled = (current_time - hokuyo.timestamp > 1.0);
  if(current_time - last_alive > 1.0) {
    publish_hokuyo_alive(stalled);
    last_alive = current_time;
  }
  return 0;
}

void shutdown_hokuyo(int x)
{
  carmen_hokuyo_shutdown(x);
  carmen_ipc_disconnect();
  exit(-1);
}

