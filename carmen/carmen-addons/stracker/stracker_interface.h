
#ifndef STRACKER_INTERFACE_H
#define STRACKER_INTERFACE_H

#include "stracker_messages.h"

#ifdef __cplusplus
extern "C" {
#endif

void carmen_stracker_start();
void carmen_stracker_subscribe_pos_message(carmen_stracker_pos_msg *msg,
					   carmen_handler_t handler,
					   carmen_subscribe_t subscribe_how);


#ifdef __cplusplus
}
#endif

#endif
