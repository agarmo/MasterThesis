
#ifndef ROOMNAV_INTERFACE_H
#define ROOMNAV_INTERFACE_H

#include "roomnav_messages.h"

#ifdef __cplusplus
extern "C" {
#endif


int carmen_roomnav_get_room();
int carmen_roomnav_get_room_from_point(carmen_map_point_t point);
int carmen_roomnav_get_room_from_world_point(carmen_point_t point);

void carmen_roomnav_subscribe_room_message(carmen_roomnav_room_msg *room_msg,
					   carmen_handler_t handler,
					   carmen_subscribe_t subscribe_how);

int carmen_roomnav_get_path(int **path);
void carmen_roomnav_subscribe_path_message(carmen_roomnav_path_msg *path_msg,
					   carmen_handler_t handler,
					   carmen_subscribe_t subscribe_how);

void carmen_roomnav_set_goal(int goal);
int carmen_roomnav_get_goal();
void carmen_roomnav_subscribe_goal_changed_message
(carmen_roomnav_goal_changed_msg *msg, carmen_handler_t handler,
 carmen_subscribe_t subscribe_how);

carmen_rooms_topology_p carmen_roomnav_get_rooms_topology();


#ifdef __cplusplus
}
#endif

#endif
