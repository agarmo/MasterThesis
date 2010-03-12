#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <carmen/carmen.h>
#include "carmen_hokuyo.h"
#include "hokuyolaser.h"

#ifdef __APPLE__
#include <limits.h>
#include <float.h>
#define MAXDOUBLE DBL_MAX
#else
#include <values.h>
#endif

#include "laser_messages.h"


int carmen_hokuyo_init(carmen_laser_device_t* device){
  HokuyoLaser* hokuyoLaser=malloc(sizeof(HokuyoLaser));
  carmen_test_alloc(hokuyoLaser);
  device->device_data=hokuyoLaser;
  hokuyo_init(hokuyoLaser);
  return 1;
}

int carmen_hokuyo_connect(carmen_laser_device_t * device, char* filename, int baudrate __attribute__((unused)) ){
  int result;
  HokuyoLaser* hokuyoLaser=(HokuyoLaser*)device->device_data;
  result=hokuyo_open(hokuyoLaser,filename);
  if (result<=0){
    fprintf(stderr, "error\n  Unable to opening device\n");
    return result;
  }
  return 1;
}

int carmen_hokuyo_configure(carmen_laser_device_t * device)
{
  if (device->config.laser_type == HOKUYO_URG) {
    //device->config.start_angle=hokuyo_getStartAngle(hokuyoLaser,-1);

    //Abe: not sure why FOV and start_angle were read from param file before... but I think this is better
    device->config.fov=URG_FOV;
    device->config.start_angle=URG_START_ANGLE;
    device->config.angular_resolution = URG_ANGULAR_STEP;
    device->config.accuracy = URG_ACCURACY;
    device->config.maximum_range = URG_MAX_RANGE;
  }
  else if (device->config.laser_type == HOKUYO_UTM) {
    //device->config.start_angle=hokuyo_getStartAngle(hokuyoLaser,-1);
    device->config.fov=UTM_FOV;
    device->config.start_angle=UTM_START_ANGLE;
    device->config.angular_resolution = UTM_ANGULAR_STEP;
    device->config.accuracy = UTM_ACCURACY;
    device->config.maximum_range = UTM_MAX_RANGE;
  }
  else {
    carmen_die("UNKNOWN HOKUYO TYPE\n");
  }
  return 1;
}

int carmen_hokuyo_handle_sleep(carmen_laser_device_t* device __attribute__ ((unused)) ){
  sleep(1);
  return 1;
}

int carmen_hokuyo_handle(carmen_laser_device_t* device){
  HokuyoLaser* hokuyoLaser=(HokuyoLaser*)device->device_data;

  struct timeval timestamp;
  char buf[HOKUYO_BUFSIZE];

  int c=hokuyo_readPacket(hokuyoLaser, buf, HOKUYO_BUFSIZE,10);
  HokuyoRangeReading reading;
  hokuyo_parseReading(&reading, buf,device->config.laser_type);

  if (c>0 && (reading.status==0 || reading.status==99)){
    static int normal_n_ranges =-1;
    if (normal_n_ranges<0)
      normal_n_ranges = reading.n_ranges;
    if (reading.n_ranges != normal_n_ranges){
      fprintf(stderr,"num ranages =%d, there should be %d... something weird is going on\n",reading.n_ranges, normal_n_ranges);
      return 0;
    }
    carmen_laser_laser_static_message message;
    message.id=device->laser_id;
    message.config=device->config;
    message.num_readings=reading.n_ranges;
    message.num_remissions=0;
    gettimeofday(&timestamp, NULL);
    message.timestamp=timestamp.tv_sec + 1e-6*timestamp.tv_usec;
    for (int j=0; j<reading.n_ranges; j++){
      message.range[j]=0.001*reading.ranges[j];
      if (message.range[j] <= 0.02) {
	message.range[j] += device->config.maximum_range;
      }
    }
    if (device->f_onreceive!=NULL)
      (*device->f_onreceive)(device, &message);
    return 1;
  } else {
    fprintf(stderr, "E");
  }
  return 0;
}

int carmen_hokuyo_start(carmen_laser_device_t* device){
  HokuyoLaser* hokuyoLaser=(HokuyoLaser*)device->device_data;
  int rv=hokuyo_init(hokuyoLaser);
  if (rv<=0)
    return 0;

  //lets just hardcode these because they're not entirely intuitive
  int bmin=0;
  int bmax=0;
   if (device->config.laser_type == HOKUYO_URG) {
     bmin = URG_DETECTION_RANGE_START;
     bmax = URG_DETECTION_RANGE_END;
   }
   else if (device->config.laser_type == HOKUYO_UTM) {
     bmin = UTM_DETECTION_RANGE_START;
     bmax = UTM_DETECTION_RANGE_END;
   }
   else {
     carmen_die("UNKNOWN HOKUYO TYPE\n");
   }


  fprintf(stderr, "Configuring hokuyo continuous mode, bmin=%d, bmax=%d\n", bmin, bmax);
  rv=hokuyo_startContinuous(hokuyoLaser, bmin, bmax, 0, 0);
  if (rv<=0){
    fprintf(stderr, "Error in configuring continuous mode\n");
  }
  device->f_handle=carmen_hokuyo_handle;
  return 1;
}

int carmen_hokuyo_stop(carmen_laser_device_t* device){
  HokuyoLaser* hokuyoLaser=(HokuyoLaser*)device->device_data;
  int rv=hokuyo_stopContinuous(hokuyoLaser);
  if (rv<=0){
    fprintf(stderr, "Error in stopping continuous mode\n");
    return 0;
  }
  device->f_handle=carmen_hokuyo_handle_sleep;
  return 1;
}



//FIXME I do not want  to malloc the ranges!

int carmen_hokuyo_close(struct carmen_laser_device_t* device){
  HokuyoLaser* hokuyoLaser=(HokuyoLaser*)device->device_data;
  return hokuyo_close(hokuyoLaser);
}

carmen_laser_device_t* carmen_create_hokuyo_instance(carmen_laser_laser_config_t* config, int laser_id){
  fprintf(stderr,"init hokuyo\n");
  carmen_laser_device_t* device=(carmen_laser_device_t*)malloc(sizeof(carmen_laser_device_t));
  carmen_test_alloc(device);
  device->laser_id=laser_id;
  device->config=*config;
  device->f_init=carmen_hokuyo_init;
  device->f_connect=carmen_hokuyo_connect;
  device->f_configure=carmen_hokuyo_configure;
  device->f_start=carmen_hokuyo_start;
  device->f_stop=carmen_hokuyo_stop;
  device->f_handle=carmen_hokuyo_handle_sleep;
  device->f_close=carmen_hokuyo_close;
  device->f_onreceive=NULL;
  return device;
}

carmen_laser_laser_config_t carmen_hokuyo_valid_configs[]=
  {{HOKUYO_URG,MAXDOUBLE,MAXDOUBLE,MAXDOUBLE,MAXDOUBLE,MAXDOUBLE,REMISSION_NONE},
    {HOKUYO_UTM,MAXDOUBLE,MAXDOUBLE,MAXDOUBLE,MAXDOUBLE,MAXDOUBLE,REMISSION_NONE}};



int carmen_hokuyo_valid_configs_size=2;

int carmen_init_hokuyo_configs(void){
  int i;
  carmen_laser_laser_config_t* conf=carmen_laser_configurations+carmen_laser_configurations_num;
  for (i=0; i<carmen_hokuyo_valid_configs_size; i++){
    *conf=carmen_hokuyo_valid_configs[i];
    conf++;
  }
  carmen_laser_configurations_num+=carmen_hokuyo_valid_configs_size;
  return carmen_laser_configurations_num;
}



