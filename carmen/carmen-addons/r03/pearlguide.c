#include <carmen/carmen.h>
#include <carmen/roomnav_interface.h>
#include "neck_interface.h"
#include "pearlguide_messages.h"


static carmen_localize_globalpos_message globalpos;
static carmen_navigator_status_message navigator_status;
static carmen_rooms_topology_p topology = NULL;
static carmen_roomnav_room_msg room_msg;
static carmen_roomnav_path_msg path_msg;
static carmen_map_t map;
static carmen_point_t goal;
static int goal_room;
static int do_guidance;
static double last_neck_motion;
static int do_neck_motion;
static int start_navigator;
static int wait_for_localize;
static int wait_for_path;
static int wait_for_room;
static int wait_for_navigator;


static void guide() {

  static carmen_point_t old_nav_goal;
  carmen_point_t nav_goal;
  double angle_to_nav_goal;
  double theta_diff;

  if (wait_for_localize || wait_for_room || wait_for_navigator)
    return;

  if (wait_for_path) {
    if (room_msg.room == goal_room) {
      wait_for_path = 0;
      path_msg.pathlen = 0;
    }
    else
      return;
  }

  wait_for_localize = 1;

  if (path_msg.pathlen == 0)
    nav_goal = goal;
  else {
    nav_goal = topology->doors[path_msg.path[0]].pose;
    if (carmen_distance(&globalpos.globalpos, &nav_goal) < 2.0) {
      if (path_msg.pathlen == 1)
	nav_goal = goal;
      else
	nav_goal = topology->doors[path_msg.path[1]].pose;
    }
  }
  
  if (start_navigator || nav_goal.x != old_nav_goal.x || nav_goal.y != old_nav_goal.y) {
    carmen_navigator_set_goal(nav_goal.x, nav_goal.y);
    printf("setting new navigator goal\n");
  }

  old_nav_goal = nav_goal;

  if (start_navigator) {
    start_navigator = 0;
    carmen_navigator_go();
  }

  if (do_neck_motion) {
    if (carmen_get_time_ms() > last_neck_motion + 1.0) {
      angle_to_nav_goal = atan2(nav_goal.y - globalpos.globalpos.y, nav_goal.x - globalpos.globalpos.x);
      theta_diff = carmen_normalize_theta(angle_to_nav_goal - globalpos.globalpos.theta);
      carmen_neck_move(theta_diff);
      last_neck_motion = carmen_get_time_ms();
    }
  }
}

void localize_handler() {

  wait_for_localize = 0;
}

void path_handler() {

  wait_for_path = 0;
}

void room_handler() {

  wait_for_room = 0;
}

void navigator_status_handler() {

  wait_for_navigator = 0;
}

static void set_goal(char *goal_name) {

  carmen_map_point_t mp;
  carmen_map_placelist_t placelist;
  int i;

  if (carmen_map_get_placelist(&placelist) < 0)
    carmen_die("Placelist not found\n");

  for (i = 0; i < placelist.num_places; i++) {
    if (!strcmp(placelist.places[i].name, goal_name)) {
      goal.x = placelist.places[i].x;
      goal.y = placelist.places[i].y;
      do_guidance = 1;
      carmen_point_to_map(&goal, &mp, &map);
      goal_room = carmen_roomnav_get_room_from_point(mp);
      if (goal_room < 0) {
	carmen_warn("Goal room not found\n");
	return;
      }      
      carmen_roomnav_set_goal(goal_room);
      wait_for_path = 1;
      start_navigator = 1;
      return;
    }
  }

  carmen_warn("Unknown goal: %s\n", goal_name);
}

void goal_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
		  void *clientData __attribute__ ((unused))) {

  carmen_pearlguide_goal_msg msg;
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &msg,
			   sizeof(carmen_pearlguide_goal_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall", IPC_msgInstanceName(msgRef));

  set_goal(msg.goal_name);
}

void neck_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
		  void *clientData __attribute__ ((unused))) {

  carmen_pearlguide_neck_msg msg;
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &msg,
			   sizeof(carmen_pearlguide_neck_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall", IPC_msgInstanceName(msgRef));

  do_neck_motion = msg.do_neck_motion;
  if (!do_neck_motion)
    carmen_neck_move(0.0);
}

static void ipc_init() {

  IPC_RETURN_TYPE err;

  err = IPC_defineMsg(CARMEN_PEARLGUIDE_GOAL_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_PEARLGUIDE_GOAL_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_PEARLGUIDE_GOAL_MSG_NAME);

  err = IPC_subscribe(CARMEN_PEARLGUIDE_GOAL_MSG_NAME, 
		      goal_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subcribe", 
		       CARMEN_PEARLGUIDE_GOAL_MSG_NAME);
  IPC_setMsgQueueLength(CARMEN_PEARLGUIDE_GOAL_MSG_NAME, 1);

  err = IPC_defineMsg(CARMEN_PEARLGUIDE_NECK_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_PEARLGUIDE_NECK_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_PEARLGUIDE_NECK_MSG_NAME);

  err = IPC_subscribe(CARMEN_PEARLGUIDE_NECK_MSG_NAME, 
		      neck_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subcribe", 
		       CARMEN_PEARLGUIDE_NECK_MSG_NAME);
  IPC_setMsgQueueLength(CARMEN_PEARLGUIDE_NECK_MSG_NAME, 1);

  carmen_localize_subscribe_globalpos_message(&globalpos,
					      (carmen_handler_t)localize_handler,
					      CARMEN_SUBSCRIBE_LATEST);
  carmen_roomnav_subscribe_path_message(&path_msg,
					(carmen_handler_t) path_handler,
					CARMEN_SUBSCRIBE_LATEST);
  carmen_roomnav_subscribe_room_message(&room_msg,
					(carmen_handler_t) room_handler,
					CARMEN_SUBSCRIBE_LATEST);
  carmen_navigator_subscribe_status_message(&navigator_status,
					    (carmen_handler_t) navigator_status_handler,
					    CARMEN_SUBSCRIBE_LATEST);
}

static void guidance_init() {

  printf("initializing guidance");
  fflush(0);
  do_guidance = 0;
  do_neck_motion = 0;
  if (carmen_map_get_gridmap(&map) < 0)
    carmen_die("Couldn't get gridmap\n");
  printf(".");
  fflush(0);
  topology = carmen_roomnav_get_rooms_topology();
  while (topology == NULL) {
    sleep(5);
    topology = carmen_roomnav_get_rooms_topology();
    printf(".");
    fflush(0);
  }
  printf(".");
  fflush(0);
  room_msg.room = carmen_roomnav_get_room();
  while (room_msg.room < 0) {
    sleep(5);
    room_msg.room = carmen_roomnav_get_room();
    printf(".");
    fflush(0);
  }
  printf(".");
  fflush(0);
  printf("done\n");
}

void shutdown_module(int sig) {

  if (sig == SIGINT) {
    close_ipc();
    fprintf(stderr, "\nDisconnecting.\n");
    exit(0);
  }
}

int main(int argc __attribute__ ((unused)), char **argv) {

  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);
  
  signal(SIGINT, shutdown_module); 

  guidance_init();
  ipc_init();

  //IPC_dispatch();

  while (1) {
    sleep_ipc(0.01);
    if (do_guidance)
      guide();
  }
  
  return 0;
}
