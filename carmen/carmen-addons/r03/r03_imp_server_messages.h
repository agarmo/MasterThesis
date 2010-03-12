
#ifndef R03_IMP_SERVER_MESSAGES_H
#define R03_IMP_SERVER_MESSAGES_H


#ifdef __cplusplus
extern "C" {
#endif

#define NO_GUIDANCE             0
#define AUDIO_GUIDANCE          2
#define ARROW_GUIDANCE          4
#define TEXT_GUIDANCE           8
#define WOZ_GUIDANCE           16

typedef struct {
  int user_id;
  int walk_num;
  int config;
  double timestamp;
  char host[10];
} carmen_r03_imp_start_session_msg;

#define CARMEN_R03_IMP_START_SESSION_MSG_NAME "carmen_r03_imp_start_session_msg"
#define CARMEN_R03_IMP_START_SESSION_MSG_FMT  "{int, int, int, double, [char:10]}"

typedef struct {
  double timestamp;
  char host[10];
} carmen_r03_imp_finish_session_msg;

#define CARMEN_R03_IMP_FINISH_SESSION_MSG_NAME "carmen_r03_imp_finish_session_msg"
#define CARMEN_R03_IMP_FINISH_SESSION_MSG_FMT  "{double, [char:10]}"

typedef struct {
  int num_progs;
  char **progs;
  int *status;
  double timestamp;
  char host[10];
} carmen_r03_imp_carmen_status_msg;

#define CARMEN_R03_IMP_CARMEN_STATUS_MSG_NAME "carmen_r03_imp_carmen_status_msg"
#define CARMEN_R03_IMP_CARMEN_STATUS_MSG_FMT  "{int, <string:1>, <int:1>, double, [char:10]}"

typedef struct {
  char *intro_file;
  double timestamp;
  char host[10];
} carmen_r03_imp_intro_msg;

#define CARMEN_R03_IMP_INTRO_MSG_NAME "carmen_r03_imp_intro_msg"
#define CARMEN_R03_IMP_INTRO_MSG_FMT  "{string, double, [char:10]}"

typedef struct {
  double timestamp;
  char host[10];
} carmen_r03_imp_shutdown_msg;

#define CARMEN_R03_IMP_SHUTDOWN_MSG_NAME "carmen_r03_imp_shutdown_msg"
#define CARMEN_R03_IMP_SHUTDOWN_MSG_FMT  "{double, [char:10]}"



#ifdef __cplusplus
}
#endif

#endif
