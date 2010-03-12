#ifndef NECK_INTERFACE_H
#define NECK_INTERFACE_H

#include "neck_messages.h"

#ifdef __cplusplus
extern "C" {
#endif


void carmen_neck_move(double theta);
void carmen_neck_command(char *cmd);


#ifdef __cplusplus
}
#endif

#endif
