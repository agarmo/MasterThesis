
#ifndef NECK_MESSAGES_H
#define NECK_MESSAGES_H


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
  double theta;
  double timestamp;
  char host[10];
} carmen_neck_move_msg;

#define CARMEN_NECK_MOVE_MSG_NAME "carmen_neck_move_msg"
#define CARMEN_NECK_MOVE_MSG_FMT  "{double, double, [char:10]}"

typedef struct {
  char *cmd;
  double timestamp;
  char host[10];
} carmen_neck_command_msg;

#define CARMEN_NECK_COMMAND_MSG_NAME "carmen_neck_command_msg"
#define CARMEN_NECK_COMMAND_MSG_FMT  "{string, double, [char:10]}"


#ifdef __cplusplus
}
#endif

#endif
