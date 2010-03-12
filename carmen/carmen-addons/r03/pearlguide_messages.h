
#ifndef PEARLGUIDE_MESSAGES_H
#define PEARLGUIDE_MESSAGES_H


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
  char *goal_name;
  double timestamp;
  char host[10];
} carmen_pearlguide_goal_msg;

#define CARMEN_PEARLGUIDE_GOAL_MSG_NAME "carmen_pearlguide_goal_msg"
#define CARMEN_PEARLGUIDE_GOAL_MSG_FMT  "{string, double, [char:10]}"

typedef struct {
  int do_neck_motion;
  double timestamp;
  char host[10];
} carmen_pearlguide_neck_msg;

#define CARMEN_PEARLGUIDE_NECK_MSG_NAME "carmen_pearlguide_neck_msg"
#define CARMEN_PEARLGUIDE_NECK_MSG_FMT  "{int, double, [char:10]}"


#ifdef __cplusplus
}
#endif

#endif
