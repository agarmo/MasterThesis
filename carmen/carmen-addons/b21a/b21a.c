#include <carmen/carmen.h>
#include "b21a_base.h"
#include "abus_sensors.h"

static carmen_base_odometry_message odometry;
static carmen_base_sonar_message sonar;
static char *baseTTY;

void main_sonar_handler(SONAR_DATA_PTR sweep, void *ignore)
{
  int i;
  DEGREES ang;
  CMS dist;
  double drad, dcos, dsin, x2, y2;

  for(i = 0; i < SONAR_READINGS_PER_SWEEP; i++) {
    ang = SONAR_INDEX_TO_DEG(i);
    dist = (double)(sweep->data[i]) / 256.0;
    drad = DEG_TO_RAD(ang);
    dcos = cos(drad);
    dsin = sin(drad);
    y2 = -dist * dcos;
    x2 = dist * dsin;
  }
  //  fprintf(stderr, "S");
}

static void main_bump_handler(DEGREES *angle)
{
  fprintf(stderr, "\nMAIN: BUMP at %.2f degrees.\n", *angle);
}

static void main_odometry_handler(void)
{
  static int first = 1;
  int err;

  odometry.x = rwi_base.pos_y / 100.0;
  odometry.y = -rwi_base.pos_x / 100.0;
  odometry.theta = carmen_normalize_theta(-rwi_base.orientation * 
					  M_PI / 180.0);
  odometry.timestamp = carmen_get_time_ms();
  if(first) {
    strcpy(odometry.host, carmen_get_tenchar_host_name());
    first = 0;
  }
  err = IPC_publishData(CARMEN_BASE_ODOMETRY_NAME, &odometry);
  carmen_test_ipc_exit(err, "Could not publish", CARMEN_BASE_ODOMETRY_NAME);
}

static void ipc_handler(void *x, void *y)
{
  sleep_ipc(0.01);
}

void b21a_shutdown(int sig)
{
  if(sig == SIGINT) {
    BASE_Halt();
    stop_abus_sensors();
    devShutdown();
    exit(1);
  }
}

static void b21a_velocity_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
				  void *clientData __attribute__ ((unused)))
{
  IPC_RETURN_TYPE err;
  carmen_base_velocity_message vel;

  FORMATTER_PTR formatter;
  
  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &vel,
			   sizeof(carmen_base_velocity_message));
  IPC_freeByteArray(callData);
  
  carmen_test_ipc_return(err, "Could not unmarshall", 
			 IPC_msgInstanceName(msgRef));
  
  BASE_RotateVelocity(fabs(vel.rv) * 180 / M_PI);
  BASE_TranslateVelocity(fabs(vel.tv) * 100.0);
  
  if(vel.rv > 0.0)
    BASE_RotateAnticlockwise();
  else if(vel.rv < 0.0)
    BASE_RotateClockwise();
  else if(vel.rv == 0.0)
    BASE_RotateVelocity(0);
  
  if(vel.tv > 0.0)
    BASE_TranslateForward();
  else if(vel.tv < 0.0)
    BASE_TranslateBackward();
  else if(vel.tv == 0.0)
    BASE_TranslateVelocity(0);
}

int carmen_b21a_initialize_messages(void)
{
  IPC_RETURN_TYPE err;
  
  /* define messages created by scout */
  err = IPC_defineMsg(CARMEN_BASE_ODOMETRY_NAME, IPC_VARIABLE_LENGTH,
                      CARMEN_BASE_ODOMETRY_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_BASE_ODOMETRY_NAME);
  
  err = IPC_defineMsg(CARMEN_BASE_SONAR_NAME, IPC_VARIABLE_LENGTH,
                      CARMEN_BASE_SONAR_FMT);
  carmen_test_ipc_exit(err, "Could not define IPC message", 
		       CARMEN_BASE_SONAR_NAME);
  
  err = IPC_defineMsg(CARMEN_BASE_VELOCITY_NAME, IPC_VARIABLE_LENGTH,
                      CARMEN_BASE_VELOCITY_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_BASE_VELOCITY_NAME);

  /* setup incoming message handlers */
  err = IPC_subscribe(CARMEN_BASE_VELOCITY_NAME, b21a_velocity_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subscribe", CARMEN_BASE_VELOCITY_NAME);

  return IPC_No_Error;
}

void process_params(int argc, char **argv)
{
  double temp;
  carmen_param_t param_list[] = {
    {"b21a", "acceleration", CARMEN_PARAM_DOUBLE, &transAcceleration, 0, NULL},
    {"b21a", "deceleration", CARMEN_PARAM_DOUBLE, &transDeceleration, 0, NULL},
    {"b21a", "timeout", CARMEN_PARAM_DOUBLE, &temp, 0, NULL},
    {"b21a", "base_dev", CARMEN_PARAM_STRING, &baseTTY, 0, NULL},
  };

  carmen_param_install_params(argc, argv, param_list, 
			      sizeof(param_list) / sizeof(param_list[0]));
  baseWatchDogTimeout = (int)rint(temp * 1000.0);
  if(baseWatchDogTimeout > 999)
    baseWatchDogTimeout = 999;
  transAcceleration *= 100.0;
  transDeceleration *= 100.0;
}

int main(int argc, char **argv)
{
  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);
  carmen_b21a_initialize_messages();
  process_params(argc, argv);

  if(!BASE_init())
    carmen_die("Error: could not start b21a base.\n");
  if(!SONAR_init())
    carmen_die("Error: could not start b21a sonar.\n");
  signal(SIGINT, b21a_shutdown);
  BASE_InstallHandler((Handler)main_bump_handler, BUMP, NULL, FALSE);
  SONAR_stream(NULL, (Handler)main_sonar_handler, NULL);
  BASE_InstallHandler((Handler)main_odometry_handler, STATUS_REPORT,
		      NULL, FALSE);
  start_abus_sensors(ipc_handler);
  devMainLoop();
  return 0;
}

