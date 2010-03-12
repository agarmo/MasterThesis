#include <carmen/carmen.h>
#include <carmen/ipc.h>
#include "hokuyo.h"
#include "hokuyo_messages.h"
#include "hokuyo_dev.h"


int allocsize[1] = {0};
float *range_buffer[1] = {NULL};


void publish_hokuyo_alive(int stalled)
{
  IPC_RETURN_TYPE err;
  carmen_hokuyo_alive_message msg;

  msg.stalled = stalled;
  err = IPC_publishData(CARMEN_HOKUYO_ALIVE_NAME, &msg);
  carmen_test_ipc_exit(err, "Could not publish", CARMEN_LASER_ALIVE_NAME);
}

void publish_hokuyo_message(hokuyo_p hokuyo)
{
  static char *host = NULL;
  static carmen_hokuyo_pb9_message msg;

  IPC_RETURN_TYPE err;
  int i;

  host = carmen_get_host();
 
  msg.config.angular_resolution = 0;
  msg.config.fov = 0;
  msg.config.start_angle = 0;
  msg.config.maximum_range = 0;
  msg.config.accuracy = 0;
  msg.config.laser_type = UMKNOWN_PROXIMITY_SENSOR;
  msg.config.remission_mode = OFF;
  msg.num_remissions = 0;
  msg.remission  = NULL;

  msg.num_readings = hokuyo->num_readings;
  msg.timestamp = hokuyo->timestamp;
  
  if(msg.num_readings != allocsize[0]) {
    range_buffer[0] = 
      realloc(range_buffer[0],
	      msg.num_readings * sizeof(float));
    carmen_test_alloc(range_buffer[0]);
    allocsize[0] = msg.num_readings;
  }

  msg.range = range_buffer[0];

  if( hokuyo->flipped == 0) {
    for(i = 0; i < hokuyo->num_readings; i++)
      msg.range[i] = hokuyo->range[i];
  }
  else  {      
    for(i = 0; i < hokuyo->num_readings; i++)
      msg.range[i] = hokuyo->range[hokuyo->num_readings-1-i] / 100.0;
  }

  err = IPC_publishData(CARMEN_HOKUYO_PB9_NAME, &msg);
  carmen_test_ipc_exit(err, "Could not publish", 
		       CARMEN_HOKUYO_PB9_NAME);
}

void ipc_initialize_messages(void)
{
  IPC_RETURN_TYPE err;
  
  err = IPC_defineMsg(CARMEN_HOKUYO_PB9_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_HOKUYO_PB9_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_HOKUYO_PB9_NAME);
  
  err = IPC_defineMsg(CARMEN_HOKUYO_ALIVE_NAME, IPC_VARIABLE_LENGTH,
                      CARMEN_HOKUYO_ALIVE_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_HOKUYO_ALIVE_NAME);
}
