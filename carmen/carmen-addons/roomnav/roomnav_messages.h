
#ifndef ROOMNAV_MESSAGES_H
#define ROOMNAV_MESSAGES_H

#include "roomnav.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
  int room;
  double timestamp;
  char host[10];
} carmen_roomnav_room_msg;

#define CARMEN_ROOMNAV_ROOM_MSG_NAME "carmen_roomnav_room_msg"
#define CARMEN_ROOMNAV_ROOM_MSG_FMT  "{int, double, [char:10]}"

typedef struct {
  double timestamp;
  char host[10];
} carmen_roomnav_query;

#define CARMEN_ROOMNAV_QUERY_FMT "{double, [char:10]}"

#define CARMEN_ROOMNAV_ROOM_QUERY_NAME "carmen_roomnav_room_query"
#define CARMEN_ROOMNAV_ROOM_QUERY_FMT CARMEN_ROOMNAV_QUERY_FMT

typedef struct {
  carmen_map_point_t point;  /* WARNING: point.map will be junk! */
  double timestamp;
  char host[10];
} carmen_roomnav_room_from_point_query;

#define CARMEN_ROOMNAV_ROOM_FROM_POINT_QUERY_NAME "carmen_roomnav_room_from_point_query"
#define CARMEN_ROOMNAV_ROOM_FROM_POINT_QUERY_FMT "{{int, int, int}, double, [char:10]}"

typedef struct {
  carmen_point_t point;
  double timestamp;
  char host[10];
} carmen_roomnav_room_from_world_point_query;

#define CARMEN_ROOMNAV_ROOM_FROM_WORLD_POINT_QUERY_NAME "carmen_roomnav_room_from_world_point_query"
#define CARMEN_ROOMNAV_ROOM_FROM_WORLD_POINT_QUERY_FMT "{{double, double, double}, double, [char:10]}"

#define CARMEN_ROOMNAV_GOAL_QUERY_NAME "carmen_roomnav_goal_query"
#define CARMEN_ROOMNAV_GOAL_QUERY_FMT CARMEN_ROOMNAV_QUERY_FMT

#define CARMEN_ROOMNAV_PATH_QUERY_NAME "carmen_roomnav_path_query"
#define CARMEN_ROOMNAV_PATH_QUERY_FMT CARMEN_ROOMNAV_QUERY_FMT

#define CARMEN_ROOMNAV_ROOMS_TOPOLOGY_QUERY_NAME "carmen_roomnav_rooms_topology_query"
#define CARMEN_ROOMNAV_ROOMS_TOPOLOGY_QUERY_FMT CARMEN_ROOMNAV_QUERY_FMT

typedef struct {
  carmen_rooms_topology_t topology;
  double timestamp;
  char host[10];
} carmen_roomnav_rooms_topology_msg;

#define CARMEN_ROOMNAV_ROOMS_TOPOLOGY_MSG_NAME "carmen_roomnav_rooms_topology_msg"
#define CARMEN_ROOMNAV_ROOMS_TOPOLOGY_MSG_FMT "{{<{int, string, <int:4>, int, int, int, double, double, double, double, double, double, double, double, {double, double, double}, {double, double, double}}:2>, int, <{int, int, {<{int,int,[char:22],double,double,double,double,double,double}:2>,int},{double,double,double},double,int,int}:4>, int}, double, [char:10]}"

typedef struct {
  int goal;
  double timestamp;
  char host[10];
} carmen_roomnav_set_goal_msg, carmen_roomnav_goal_changed_msg;

#define CARMEN_ROOMNAV_SET_GOAL_MSG_NAME "carmen_roomnav_set_goal_msg"
#define CARMEN_ROOMNAV_SET_GOAL_MSG_FMT "{int, double, [char:10]}"
#define CARMEN_ROOMNAV_GOAL_CHANGED_MSG_NAME "carmen_roomnav_goal_changed_msg"
#define CARMEN_ROOMNAV_GOAL_CHANGED_MSG_FMT "{int, double, [char:10]}"

typedef struct {
  int *path;  // array of door #'s
  int pathlen;
  double timestamp;
  char host[10];
} carmen_roomnav_path_msg;

#define CARMEN_ROOMNAV_PATH_MSG_NAME "carmen_roomnav_path_msg"
#define CARMEN_ROOMNAV_PATH_MSG_FMT "{<int:2>,int,double,[char:10]}"

typedef struct {
  int goal;
  double timestamp;
  char host[10];
} carmen_roomnav_goal_msg;

#define CARMEN_ROOMNAV_GOAL_MSG_NAME "carmen_roomnav_goal_msg"
#define CARMEN_ROOMNAV_GOAL_MSG_FMT "{int,double,[char:10]}"


#ifdef __cplusplus
}
#endif

#endif
