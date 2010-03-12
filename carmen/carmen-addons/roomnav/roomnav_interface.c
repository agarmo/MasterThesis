
#include <carmen/carmen.h>
#include "roomnav_interface.h"


static carmen_roomnav_room_msg *room_msg_ptr_ext = NULL;
static carmen_handler_t room_msg_handler_ext = NULL;
//static carmen_roomnav_rooms_topology_msg *rooms_msg_ptr_ext = NULL;
//static carmen_handler_t rooms_msg_handler_ext = NULL;
static carmen_roomnav_path_msg *path_msg_ptr_ext = NULL;
static carmen_handler_t path_msg_handler_ext = NULL;
static carmen_roomnav_goal_changed_msg *goal_changed_msg_ptr_ext = NULL;
static carmen_handler_t goal_changed_msg_handler_ext = NULL;


static void room_msg_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
			     void *clientData __attribute__ ((unused))) {
  
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);

  if (room_msg_ptr_ext)
    err = IPC_unmarshallData(formatter, callData, room_msg_ptr_ext,
                             sizeof(carmen_roomnav_room_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall", IPC_msgInstanceName(msgRef));

  if (room_msg_handler_ext)
    room_msg_handler_ext(room_msg_ptr_ext);
}

void carmen_roomnav_subscribe_room_message(carmen_roomnav_room_msg *room_msg,
					carmen_handler_t handler,
					carmen_subscribe_t subscribe_how) {

  IPC_RETURN_TYPE err = IPC_OK;  

  if(subscribe_how == CARMEN_UNSUBSCRIBE) {
    IPC_unsubscribe(CARMEN_ROOMNAV_ROOM_MSG_NAME, 
		    room_msg_handler);
    return;
  }

  if(room_msg)
    room_msg_ptr_ext = room_msg;
  else {
    room_msg_ptr_ext = (carmen_roomnav_room_msg *)
      calloc(1, sizeof(carmen_roomnav_room_msg));
    carmen_test_alloc(room_msg_ptr_ext);
  }

  room_msg_handler_ext = handler;
  err = IPC_subscribe(CARMEN_ROOMNAV_ROOM_MSG_NAME, 
		      room_msg_handler, NULL);

  if(subscribe_how == CARMEN_SUBSCRIBE_LATEST)
    IPC_setMsgQueueLength(CARMEN_ROOMNAV_ROOM_MSG_NAME, 1);
  else
    IPC_setMsgQueueLength(CARMEN_ROOMNAV_ROOM_MSG_NAME, 100);

  carmen_test_ipc(err, "Could not subscribe", CARMEN_ROOMNAV_ROOM_MSG_NAME);
}

int carmen_roomnav_get_room() {

  IPC_RETURN_TYPE err;
  carmen_roomnav_query query;
  carmen_roomnav_room_msg *response;

  query.timestamp = carmen_get_time_ms();
  strcpy(query.host, carmen_get_tenchar_host_name());

  err = IPC_queryResponseData(CARMEN_ROOMNAV_ROOM_QUERY_NAME, &query, 
			      (void **) &response, 5000);
  carmen_test_ipc_return_int(err, "Could not query room",
			     CARMEN_ROOMNAV_ROOM_QUERY_NAME);

  return response->room;
}

int carmen_roomnav_get_room_from_point(carmen_map_point_t point) {

  IPC_RETURN_TYPE err;
  carmen_roomnav_room_from_point_query query;
  carmen_roomnav_room_msg *response;

  memcpy(&query.point, &point, sizeof(carmen_map_point_t));
  query.timestamp = carmen_get_time_ms();
  strcpy(query.host, carmen_get_tenchar_host_name());

  err = IPC_queryResponseData(CARMEN_ROOMNAV_ROOM_FROM_POINT_QUERY_NAME, &query, 
			      (void **) &response, 5000);
  carmen_test_ipc_return_int(err, "Could not query room",
			     CARMEN_ROOMNAV_ROOM_FROM_POINT_QUERY_NAME);

  return response->room;
}

int carmen_roomnav_get_room_from_world_point(carmen_point_t point) {

  IPC_RETURN_TYPE err;
  carmen_roomnav_room_from_world_point_query query;
  carmen_roomnav_room_msg *response;

  memcpy(&query.point, &point, sizeof(carmen_point_t));
  query.timestamp = carmen_get_time_ms();
  strcpy(query.host, carmen_get_tenchar_host_name());

  err = IPC_queryResponseData(CARMEN_ROOMNAV_ROOM_FROM_WORLD_POINT_QUERY_NAME, &query, 
			      (void **) &response, 5000);
  carmen_test_ipc_return_int(err, "Could not query room",
			     CARMEN_ROOMNAV_ROOM_FROM_WORLD_POINT_QUERY_NAME);

  return response->room;
}

int carmen_roomnav_get_goal() {

  IPC_RETURN_TYPE err;
  carmen_roomnav_query query;
  carmen_roomnav_goal_msg *response;

  query.timestamp = carmen_get_time_ms();
  strcpy(query.host, carmen_get_tenchar_host_name());

  err = IPC_queryResponseData(CARMEN_ROOMNAV_GOAL_QUERY_NAME, &query, 
			      (void **) &response, 5000);
  carmen_test_ipc_return_int(err, "Could not query goal",
			     CARMEN_ROOMNAV_GOAL_QUERY_NAME);

  return response->goal;
}

int carmen_roomnav_get_path(int **path) {

  IPC_RETURN_TYPE err;
  carmen_roomnav_query query;
  carmen_roomnav_path_msg *response;

  query.timestamp = carmen_get_time_ms();
  strcpy(query.host, carmen_get_tenchar_host_name());

  err = IPC_queryResponseData(CARMEN_ROOMNAV_PATH_QUERY_NAME, &query, 
			      (void **) &response, 5000);
  carmen_test_ipc_return_int(err, "Could not query path",
			     CARMEN_ROOMNAV_PATH_QUERY_NAME);

  if (path) {
    if (response->path) {
      *path = (int *) realloc(*path, response->pathlen * sizeof(int));
      memcpy(*path, response->path, response->pathlen * sizeof(int));
    }
    else
      *path = NULL;
  }

  return response->pathlen;
}

carmen_rooms_topology_p carmen_roomnav_get_rooms_topology() {

  IPC_RETURN_TYPE err;
  carmen_roomnav_query query;
  carmen_roomnav_rooms_topology_msg *response;

  query.timestamp = carmen_get_time_ms();
  strcpy(query.host, carmen_get_tenchar_host_name());

  err = IPC_queryResponseData(CARMEN_ROOMNAV_ROOMS_TOPOLOGY_QUERY_NAME, &query, 
			      (void **) &response, 5000);
  carmen_test_ipc_return_null(err, "Could not query rooms topology",
			      CARMEN_ROOMNAV_ROOMS_TOPOLOGY_QUERY_NAME);

  return &response->topology;
}

static void path_msg_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
			     void *clientData __attribute__ ((unused))) {
  
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);

  if (path_msg_ptr_ext)
    err = IPC_unmarshallData(formatter, callData, path_msg_ptr_ext,
                             sizeof(carmen_roomnav_path_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall", IPC_msgInstanceName(msgRef));

  if (path_msg_handler_ext)
    path_msg_handler_ext(path_msg_ptr_ext);
}

void carmen_roomnav_subscribe_path_message(carmen_roomnav_path_msg *path_msg,
					carmen_handler_t handler,
					carmen_subscribe_t subscribe_how) {

  IPC_RETURN_TYPE err = IPC_OK;

  if(subscribe_how == CARMEN_UNSUBSCRIBE) {
    IPC_unsubscribe(CARMEN_ROOMNAV_PATH_MSG_NAME, 
		    path_msg_handler);
    return;
  }

  if(path_msg)
    path_msg_ptr_ext = path_msg;
  else {
    path_msg_ptr_ext = (carmen_roomnav_path_msg *)
      calloc(1, sizeof(carmen_roomnav_path_msg));
    carmen_test_alloc(path_msg_ptr_ext);
  }

  path_msg_handler_ext = handler;
  err = IPC_subscribe(CARMEN_ROOMNAV_PATH_MSG_NAME, 
		      path_msg_handler, NULL);

  if(subscribe_how == CARMEN_SUBSCRIBE_LATEST)
    IPC_setMsgQueueLength(CARMEN_ROOMNAV_PATH_MSG_NAME, 1);
  else
    IPC_setMsgQueueLength(CARMEN_ROOMNAV_PATH_MSG_NAME, 100);

  carmen_test_ipc(err, "Could not subscribe", CARMEN_ROOMNAV_PATH_MSG_NAME);
}

void carmen_roomnav_set_goal(int goal) {

  IPC_RETURN_TYPE err = IPC_OK;
  carmen_roomnav_set_goal_msg msg;

  msg.timestamp = carmen_get_time_ms();
  strcpy(msg.host, carmen_get_tenchar_host_name());

  msg.goal = goal;

  err = IPC_publishData(CARMEN_ROOMNAV_SET_GOAL_MSG_NAME, &msg);
  carmen_test_ipc(err, "Could not publish", CARMEN_ROOMNAV_SET_GOAL_MSG_NAME);
}

static void goal_changed_msg_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
				     void *clientData __attribute__ ((unused))) {
  
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);

  if (goal_changed_msg_ptr_ext)
    err = IPC_unmarshallData(formatter, callData, goal_changed_msg_ptr_ext,
                             sizeof(carmen_roomnav_goal_changed_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall", IPC_msgInstanceName(msgRef));

  if (goal_changed_msg_handler_ext)
    goal_changed_msg_handler_ext(goal_changed_msg_ptr_ext);
}

void carmen_roomnav_subscribe_goal_changed_message
(carmen_roomnav_goal_changed_msg *msg, carmen_handler_t handler,
 carmen_subscribe_t subscribe_how) {

  IPC_RETURN_TYPE err = IPC_OK;  

  if(subscribe_how == CARMEN_UNSUBSCRIBE) {
    IPC_unsubscribe(CARMEN_ROOMNAV_GOAL_CHANGED_MSG_NAME, 
		    goal_changed_msg_handler);
    return;
  }

  if(msg)
    goal_changed_msg_ptr_ext = msg;
  else {
    goal_changed_msg_ptr_ext = (carmen_roomnav_goal_changed_msg *)
      calloc(1, sizeof(carmen_roomnav_goal_changed_msg));
    carmen_test_alloc(goal_changed_msg_ptr_ext);
  }

  goal_changed_msg_handler_ext = handler;
  err = IPC_subscribe(CARMEN_ROOMNAV_GOAL_CHANGED_MSG_NAME, 
		      goal_changed_msg_handler, NULL);

  if(subscribe_how == CARMEN_SUBSCRIBE_LATEST)
    IPC_setMsgQueueLength(CARMEN_ROOMNAV_GOAL_CHANGED_MSG_NAME, 1);
  else
    IPC_setMsgQueueLength(CARMEN_ROOMNAV_GOAL_CHANGED_MSG_NAME, 100);

  carmen_test_ipc(err, "Could not subscribe", CARMEN_ROOMNAV_GOAL_CHANGED_MSG_NAME);
}
