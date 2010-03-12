#ifndef WALKER_MESSAGES_H
#define WALKER_MESSAGES_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
  int goal;
  double timestamp;
  char host[10];
} carmen_walker_set_goal_msg, carmen_walker_goal_changed_msg;

#define CARMEN_WALKER_SET_GOAL_MSG_NAME "carmen_walker_set_goal_msg"
#define CARMEN_WALKER_SET_GOAL_MSG_FMT "{int, double, [char:10]}"
#define CARMEN_WALKER_GOAL_CHANGED_MSG_NAME "carmen_walker_goal_changed_msg"
#define CARMEN_WALKER_GOAL_CHANGED_MSG_FMT "{int, double, [char:10]}"

#ifdef __cplusplus
}
#endif

#endif
