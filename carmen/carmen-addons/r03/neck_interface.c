#include <carmen/carmen.h>
#include "neck_interface.h"


void carmen_neck_move(double theta) {

  IPC_RETURN_TYPE err = IPC_OK;
  carmen_neck_move_msg msg;

  msg.timestamp = carmen_get_time_ms();
  strcpy(msg.host, carmen_get_tenchar_host_name());

  msg.theta = theta;

  err = IPC_publishData(CARMEN_NECK_MOVE_MSG_NAME, &msg);
  carmen_test_ipc(err, "Could not publish", CARMEN_NECK_MOVE_MSG_NAME);
}

void carmen_neck_command(char *cmd) {

  IPC_RETURN_TYPE err = IPC_OK;
  carmen_neck_command_msg msg;

  msg.timestamp = carmen_get_time_ms();
  strcpy(msg.host, carmen_get_tenchar_host_name());

  msg.cmd = cmd;

  err = IPC_publishData(CARMEN_NECK_COMMAND_MSG_NAME, &msg);
  carmen_test_ipc(err, "Could not publish", CARMEN_NECK_COMMAND_MSG_NAME);
}
