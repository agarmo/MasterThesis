
#ifndef STRACKER_MESSAGES_H
#define STRACKER_MESSAGES_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
  double dist;
  double theta;
  double timestamp;
  char host[10];
} carmen_stracker_pos_msg;

#define CARMEN_STRACKER_POS_MSG_NAME "carmen_stracker_pos_msg"
#define CARMEN_STRACKER_POS_MSG_FMT  "{double, double, double, [char:10]}"


typedef struct {
  double timestamp;
  char host[10];
} carmen_stracker_start_msg;

#define CARMEN_STRACKER_START_MSG_NAME "carmen_stracker_start_msg"
#define CARMEN_STRACKER_START_MSG_FMT  "{double, [char:10]}"


#ifdef __cplusplus
}
#endif

#endif
