
#ifndef VALET_INTERFACE_H
#define VALET_INTERFACE_H

#include <carmen/carmen.h>
#include "valet_messages.h"

#ifdef __cplusplus
extern "C" {
#endif


void carmen_valet_park();

void carmen_valet_return();


#ifdef __cplusplus
}
#endif

#endif
