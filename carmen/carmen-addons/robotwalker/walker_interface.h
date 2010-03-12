
#ifndef WALKER_INTERFACE_H
#define WALKER_INTERFACE_H

#include "walker_messages.h"

#ifdef __cplusplus
extern "C" {
#endif


void carmen_walker_set_goal(int goal);
void carmen_walker_engage_clutch();
void carmen_walker_disengage_clutch();


#ifdef __cplusplus
}
#endif

#endif
