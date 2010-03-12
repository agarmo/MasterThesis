
#ifndef DOT_INTERFACE_H
#define DOT_INTERFACE_H

#include "dot_messages.h"

#ifdef __cplusplus
extern "C" {
#endif

void carmen_dot_reset();
int carmen_dot_get_all_people(carmen_dot_person_p *people, int *num_people);
int carmen_dot_get_all_trash(carmen_dot_trash_p *trash, int *num_trash);
int carmen_dot_get_all_doors(carmen_dot_door_p *doors, int *num_doors);

void carmen_dot_subscribe_all_people_message(carmen_dot_all_people_msg *msg,
					     carmen_handler_t handler,
					     carmen_subscribe_t subscribe_how);
void carmen_dot_subscribe_all_trash_message(carmen_dot_all_trash_msg *msg,
					    carmen_handler_t handler,
					    carmen_subscribe_t subscribe_how);
void carmen_dot_subscribe_all_doors_message(carmen_dot_all_doors_msg *msg,
					    carmen_handler_t handler,
					    carmen_subscribe_t subscribe_how);

void carmen_dot_subscribe_person_message(carmen_dot_person_msg *msg,
					 carmen_handler_t handler,
					 carmen_subscribe_t subscribe_how);
void carmen_dot_subscribe_trash_message(carmen_dot_trash_msg *msg,
					carmen_handler_t handler,
					carmen_subscribe_t subscribe_how);
void carmen_dot_subscribe_door_message(carmen_dot_door_msg *msg,
				       carmen_handler_t handler,
				       carmen_subscribe_t subscribe_how);

#ifdef __cplusplus
}
#endif

#endif
