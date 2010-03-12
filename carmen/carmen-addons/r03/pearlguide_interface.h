#ifndef PEARLGUIDE_INTERFACE_H
#define PEARLGUIDE_INTERFACE_H

#include "pearlguide_messages.h"

#ifdef __cplusplus
extern "C" {
#endif


void carmen_pearlguide_set_goal(char *goal_name);
void carmen_pearlguide_set_neck_motion(int do_neck_motion);


#ifdef __cplusplus
}
#endif

#endif
