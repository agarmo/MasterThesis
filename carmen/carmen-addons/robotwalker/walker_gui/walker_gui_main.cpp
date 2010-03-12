#include <qpe/qpeapplication.h>
#include <carmen/carmen.h>
#include <carmen/walker_interface.h>
#include <carmen/roomnav_interface.h>
#include "walker_gui.h"


#define dist(x,y) sqrt((x)*(x)+(y)*(y))


WalkerGui *walker_gui;

int room = -1;
int *path = NULL;
int pathlen = -1;
int goal = -1;
int dest_display_index = 0;
carmen_point_t globalpos, globalpos_prev;
carmen_rooms_topology_p topology = NULL;


void arrow_update() {

  carmen_door_p next_door;
  double x, y, theta;

  //printf("arrow_update:  room = %d, pathlen = %d\n", room, pathlen);

  if (room == -1)
    return;

  if (pathlen < 0)  // no goal
    return;
  
  if (pathlen == 0) {  // goal reached
    //draw_right_canvas(0);
    return;
  }

  next_door = &topology->doors[path[0]];
  x = globalpos.x;
  y = globalpos.y;
  theta = globalpos.theta;

  if (sqrt((next_door->pose.x-x)*(next_door->pose.x-x) +
	   (next_door->pose.y-y)*(next_door->pose.y-y)) < 1.0)
    walker_gui->updateArrow(M_PI/2.0 - theta +
			    (next_door->room1 == room ?
			     next_door->pose.theta :
			     next_door->pose.theta + M_PI));
  else
    walker_gui->updateArrow(M_PI/2.0 - theta +
			    atan2(next_door->pose.y - y, next_door->pose.x - x));
}

void room_handler(carmen_roomnav_room_msg *room_msg) {

  printf("R");
  fflush(0);

  room = room_msg->room;
  arrow_update();
}

void path_handler(carmen_roomnav_path_msg *path_msg) {

  printf("P");
  fflush(0);

  pathlen = path_msg->pathlen;
  if (path_msg->path) {
    path = (int *) realloc(path, pathlen * sizeof(int));
    memcpy(path, path_msg->path, pathlen * sizeof(int));
  }
  else
    path = NULL;
  arrow_update();
}

void goal_changed_handler(carmen_roomnav_goal_changed_msg *goal_changed_msg) {

  printf("G");
  fflush(0);

  goal = goal_changed_msg->goal;
  //  sprintf(dest_name, "To %s",
  //	  topology->rooms[goal].name);
  //  draw_dest_canvas();
}

void localize_handler(carmen_localize_globalpos_message *global_pos) {

  static int first = 1;
  static long last_update;
  struct timeval time;
  long cur_time;
  double distance;

  printf(".");
  fflush(0);

  gettimeofday(&time, NULL);
  cur_time = 1000*time.tv_sec + time.tv_usec/1000;

  distance = dist(globalpos.x - globalpos_prev.x, globalpos.y - globalpos_prev.y);
  if (distance >= 0.5) {
    //distanceTraveled += distance;
    memcpy(&globalpos_prev, &globalpos, sizeof(globalpos));
    //draw_dist_canvas();
  }

  memcpy(&globalpos, &global_pos->globalpos, sizeof(globalpos));

  if (first) {
    first = 0;
    last_update = cur_time;
    memcpy(&globalpos_prev, &globalpos, sizeof(globalpos));
    //distanceTraveled = 0.0;
    //draw_dist_canvas();
  }
  else if (cur_time - last_update > 500) {
    last_update = cur_time;
    //draw_left_canvas();
    arrow_update();
    //if (pathlen < 0 || goal < 0)
    //  *dest_name = '\0';
    /*else*/ if (goal == room) {
      //sprintf(dest_name, "Arrived");
      goal = -1;
    }
    //else
    //  sprintf(dest_name, "To %s", topology->rooms[goal].name);
    //draw_dest_canvas();
  }
}

void init_ipc() {

  carmen_localize_subscribe_globalpos_message(NULL,
					      (carmen_handler_t)localize_handler,
					      CARMEN_SUBSCRIBE_LATEST);

  carmen_roomnav_subscribe_room_message(NULL, (carmen_handler_t)room_handler,
					CARMEN_SUBSCRIBE_LATEST);

  carmen_roomnav_subscribe_path_message(NULL, (carmen_handler_t)path_handler,
					CARMEN_SUBSCRIBE_LATEST);

  carmen_roomnav_subscribe_goal_changed_message(NULL,
						(carmen_handler_t)
						goal_changed_handler,
						CARMEN_SUBSCRIBE_LATEST);
}

void shutdown_ipc() {

  IPC_disconnect();
}

void update_ipc() {

  IPC_listen(0);
}

void init_roomnav() {

  topology = carmen_roomnav_get_rooms_topology();
  while (topology == NULL) {
    sleep(1);
    topology = carmen_roomnav_get_rooms_topology();
  }
  room = carmen_roomnav_get_room();
  goal = carmen_roomnav_get_goal();
  if (goal >= 0)
    pathlen = carmen_roomnav_get_path(&path);
}

void shutdown(int sig) {

  shutdown_ipc();
  exit(sig);
}

int main(int argc, char **argv) {

  QPEApplication app(argc, argv);  

  walker_gui = new WalkerGui();

  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);

  init_roomnav();
  init_ipc();

  signal(SIGINT, shutdown);

  app.showMainWidget(walker_gui);

  while (1) {
    app.processEvents();
    update_ipc();
    usleep(10000);
  }
  
  return 0;
}
