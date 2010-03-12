#include <carmen/carmen.h>
#include "stracker_messages.h"


double sensor_noise_dist = 0.1;

int num_readings = 0;
float *range = NULL;
int num_background_readings = 0;
float *background_range = NULL;

int get_background_range = 1;
int display_position = 1;

double dist = -1.0;
double theta = -1.0;


static void publish_pos_msg() {

  static carmen_stracker_pos_msg msg;
  static int first = 1;
  IPC_RETURN_TYPE err;
  
  if (first) {
    strcpy(msg.host, carmen_get_tenchar_host_name());
    first = 0;
  }

  msg.timestamp = carmen_get_time_ms();
  msg.dist = dist;
  msg.theta = (theta != -1.0 ? theta * M_PI / 180.0 : -1.0);

  err = IPC_publishData(CARMEN_STRACKER_POS_MSG_NAME, &msg);
  carmen_test_ipc_exit(err, "Could not publish", CARMEN_STRACKER_POS_MSG_NAME);
}

static void track() {

  int i, n = 0;

  dist = theta = -1.0;

  for (i = 0; i < num_readings; i++) {
    if (background_range[i] - range[i] > sensor_noise_dist) {
      dist = (dist * n + range[i]) / (n + 1);
      theta = (theta * n + i * (180.0 / (float) num_readings)) / (n + 1);
      n++;
    }
  }

  if (display_position)
    printf("pos = (%f, %f)\n", dist, theta);

  publish_pos_msg();
}

static void start_handler(MSG_INSTANCE msgRef __attribute__ ((unused)),
			  BYTE_ARRAY callData __attribute__ ((unused)), 
			  void *clientData __attribute__ ((unused))) {

  get_background_range = 1;
  printf("\nCollecting background range...");
  fflush(0);
}

static void laser_handler(carmen_laser_laser_message *laser) {

  num_readings = laser->num_readings;
  if (range)
    free(range);
  if (num_readings > 0) {
    range = (float *) calloc(num_readings, sizeof(float));
      carmen_test_alloc(range);
      memcpy(range, laser->range, num_readings * sizeof(float));
  }
  else
    range = NULL;

  if (get_background_range) {
    num_background_readings = laser->num_readings;
    if (background_range)
      free(background_range);
    if (num_background_readings > 0) {
      background_range = (float *) calloc(num_background_readings, sizeof(float));
      carmen_test_alloc(background_range);
      memcpy(background_range, laser->range, num_background_readings * sizeof(float));
    }
    else
      background_range = NULL;
    get_background_range = 0;
    printf("done\n");
  }
  else
    track();
}

static void ipc_init() {

  IPC_RETURN_TYPE err;

  err = IPC_defineMsg(CARMEN_STRACKER_POS_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_STRACKER_POS_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_STRACKER_POS_MSG_NAME);

  err = IPC_defineMsg(CARMEN_STRACKER_START_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_STRACKER_START_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_STRACKER_START_MSG_NAME);

  err = IPC_subscribe(CARMEN_STRACKER_START_MSG_NAME, start_handler, NULL);
  carmen_test_ipc(err, "Could not subscribe", CARMEN_STRACKER_START_MSG_NAME);
  IPC_setMsgQueueLength(CARMEN_STRACKER_START_MSG_NAME, 100);

  carmen_laser_subscribe_frontlaser_message(NULL, (carmen_handler_t)laser_handler,
					    CARMEN_SUBSCRIBE_LATEST);
}

static void params_init(int argc, char *argv[]) {

  carmen_param_t param_list[] = {
    {"stracker", "sensor_noise_dist", CARMEN_PARAM_DOUBLE,
     &sensor_noise_dist, 1, NULL}
  };

  carmen_param_install_params(argc, argv, param_list,
			      sizeof(param_list) / sizeof(param_list[0]));
}

void shutdown_module(int sig) {

  sig = 0;

  exit(0);
}

int main(int argc, char *argv[]) {

  int c;

  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);

  signal(SIGINT, shutdown_module);

  params_init(argc, argv);
  ipc_init();

  carmen_terminal_cbreak(0);

  while (1) {
    sleep_ipc(0.01);
    c = getchar();
    if (c != EOF) {
      switch (c) {
      case ' ':
	get_background_range = 1;
	printf("\nCollecting background range...");
	fflush(0);
	break;
      case 'p':
	display_position = !display_position;
      }
    }
  }

  return 0;
}
