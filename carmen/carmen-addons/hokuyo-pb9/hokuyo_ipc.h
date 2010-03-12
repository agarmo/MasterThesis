#ifndef LASER_IPC_H
#define LASER_IPC_H

#include "hokuyo.h"

void ipc_initialize_messages(void);

void publish_hokuyo_alive(int stalled);

void publish_hokuyo_message(hokuyo_p hokuyo);

#endif
