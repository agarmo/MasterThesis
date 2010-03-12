#include <carmen/carmen.h>
#include "pearlguide_interface.h"


void carmen_pearlguide_set_goal(char *goal_name) {

  IPC_RETURN_TYPE err = IPC_OK;
  carmen_pearlguide_goal_msg msg;

  msg.timestamp = carmen_get_time_ms();
  strcpy(msg.host, carmen_get_tenchar_host_name());

  msg.goal_name = goal_name;

  err = IPC_publishData(CARMEN_PEARLGUIDE_GOAL_MSG_NAME, &msg);
  carmen_test_ipc(err, "Could not publish", CARMEN_PEARLGUIDE_GOAL_MSG_NAME);
}

void carmen_pearlguide_set_neck_motion(int do_neck_motion) {

  IPC_RETURN_TYPE err = IPC_OK;
  carmen_pearlguide_neck_msg msg;

  msg.timestamp = carmen_get_time_ms();
  strcpy(msg.host, carmen_get_tenchar_host_name());

  msg.do_neck_motion = do_neck_motion;

  err = IPC_publishData(CARMEN_PEARLGUIDE_NECK_MSG_NAME, &msg);
  carmen_test_ipc(err, "Could not publish", CARMEN_PEARLGUIDE_NECK_MSG_NAME);
}
