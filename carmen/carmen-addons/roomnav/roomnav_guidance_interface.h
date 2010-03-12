
#ifndef ROOMNAV_GUIDANCE_INTERFACE_H
#define ROOMNAV_GUIDANCE_INTERFACE_H

#include "roomnav_guidance_messages.h"

#ifdef __cplusplus
extern "C" {
#endif


void carmen_roomnav_subscribe_guidance_message(carmen_roomnav_guidance_msg *msg,
					       carmen_handler_t handler,
					       carmen_subscribe_t subscribe_how);

void carmen_roomnav_guidance_config(int text, int audio);

#ifdef __cplusplus
}
#endif

#endif
