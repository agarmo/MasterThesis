
#include <carmen/carmen.h>
#include <carmen/map_io.h>
#include <carmen/navigator_interface.h>
#include <carmen/robot_interface.h>

#include "valet_interface.h"

typedef enum {
  CARMEN_VALET_PARKING,
  CARMEN_VALET_PARKED,
  CARMEN_VALET_RETURNING,
  CARMEN_VALET_RETURNED
} carmen_valet_status_t;

static carmen_valet_status_t valet_status = CARMEN_VALET_RETURNED;
static carmen_map_placelist_t parking_places;
static carmen_localize_globalpos_message globalpos;
static carmen_point_t return_pos;
static int turning = 0;

static inline double dist(double x, double y) {

  return sqrt(x*x + y*y);
}

#define abs(x) (x < 0 ? -x : x)

static void valet_park() {

  int i, j;

  printf("valet_park\n");

  if (valet_status == CARMEN_VALET_RETURNED) {
    printf("globalpos = (%f, %f), return_pos = (%f, %f, %f)\n",
	   globalpos.globalpos.x, globalpos.globalpos.y,
	   return_pos.x, return_pos.y, return_pos.theta);
    return_pos.x = globalpos.globalpos.x;
    return_pos.y = globalpos.globalpos.y;
    return_pos.theta = globalpos.globalpos.theta;
    printf("new return_pos = (%.2f, %.2f, %.2f)\n",
	   return_pos.x, return_pos.y, return_pos.theta);
  }

  valet_status = CARMEN_VALET_PARKING;

  for (i = 1, j = 0; i < parking_places.num_places; i++)
    if (dist(globalpos.globalpos.x - parking_places.places[i].x,
	     globalpos.globalpos.y - parking_places.places[i].y) <
	dist(globalpos.globalpos.x - parking_places.places[j].x,
	     globalpos.globalpos.y - parking_places.places[j].y))
      j = i;

  carmen_navigator_set_goal(parking_places.places[j].x,
			    parking_places.places[j].y);
  carmen_navigator_go();
}

static void valet_return() {

  valet_status = CARMEN_VALET_RETURNING;

  printf("returning to (%d, %d)\n", (int) return_pos.x, (int) return_pos.y);

  carmen_navigator_set_goal(return_pos.x, return_pos.y);
  carmen_navigator_go();
}

static void valet_park_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
			       void *clientData __attribute__ ((unused))) {

  FORMATTER_PTR formatter;

  formatter = IPC_msgInstanceFormatter(msgRef);
  IPC_freeByteArray(callData);

  valet_park();
}

static void valet_return_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
				 void *clientData __attribute__ ((unused))) {

  FORMATTER_PTR formatter;

  formatter = IPC_msgInstanceFormatter(msgRef);
  IPC_freeByteArray(callData);

  valet_return();
}

void stopped_handler(carmen_navigator_autonomous_stopped_message *msg) {

  printf("stopped\n");

  if (msg->reason != CARMEN_NAVIGATOR_GOAL_REACHED_v)
    return;

  if (valet_status == CARMEN_VALET_PARKING) {
    valet_status = CARMEN_VALET_PARKED;
    printf("parked\n");
  }
  else if (valet_status == CARMEN_VALET_RETURNING) {
    valet_status = CARMEN_VALET_RETURNED;
    turning = 1;
    printf("turning to angle %.2f\n", return_pos.theta);
  }
}

void localize_handler() {

  if (turning) {
    if (abs(globalpos.globalpos.theta - return_pos.theta) < 0.1) {
      turning = 0;
      printf("returned\n");
    }
    else
      carmen_robot_move_along_vector(0, return_pos.theta -
				     globalpos.globalpos.theta);
  }
}

static void ipc_init() {

  IPC_RETURN_TYPE err;

  err = IPC_defineMsg(CARMEN_VALET_PARK_MSG_NAME, IPC_VARIABLE_LENGTH,
		      CARMEN_VALET_PARK_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_VALET_PARK_MSG_NAME);

  err = IPC_defineMsg(CARMEN_VALET_RETURN_MSG_NAME, IPC_VARIABLE_LENGTH,
		      CARMEN_VALET_RETURN_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_VALET_RETURN_MSG_NAME);

  err = IPC_subscribe(CARMEN_VALET_PARK_MSG_NAME, valet_park_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subscribe", CARMEN_VALET_PARK_MSG_NAME);
  IPC_setMsgQueueLength(CARMEN_VALET_PARK_MSG_NAME, 1);

  err = IPC_subscribe(CARMEN_VALET_RETURN_MSG_NAME,
		      valet_return_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subscribe",
		       CARMEN_VALET_RETURN_MSG_NAME);
  IPC_setMsgQueueLength(CARMEN_VALET_RETURN_MSG_NAME, 1);

  carmen_localize_subscribe_globalpos_message
    (&globalpos, localize_handler, CARMEN_SUBSCRIBE_LATEST);

  carmen_navigator_subscribe_autonomous_stopped_message
    (NULL, (carmen_handler_t) stopped_handler,
     CARMEN_SUBSCRIBE_LATEST);
}

void valet_init() {

  int i, j;

  carmen_map_get_placelist(&parking_places);

  i = j = 0;

  while (j < parking_places.num_places) {
    while (j < parking_places.num_places &&
	   (parking_places.places[j].name[0] != 'p' ||
	    !isdigit(parking_places.places[j].name[1])))
      j++;
    if (j == parking_places.num_places)
      break;
    memcpy(&parking_places.places[i], &parking_places.places[j],
	   sizeof(carmen_place_t));
    i++;
    j++;
  }

  if (i == 0)
    carmen_die("Error: no parking spots found.\n");

  parking_places.num_places = i;
}

int main(int argc __attribute__ ((unused)), char **argv) {

  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);

  ipc_init();
  valet_init();

  IPC_dispatch();

  return 0;
}
