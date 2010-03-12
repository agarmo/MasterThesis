
#ifndef IMP_INTRO_MESSAGES_H
#define IMP_INTRO_MESSAGES_H

#ifdef __cplusplus
extern "C" {
#endif


#define CARMEN_IMP_INTRO_ARROW     1
#define CARMEN_IMP_INTRO_QUESTION  2
#define CARMEN_IMP_INTRO_TEXT      3

typedef struct {
  int cmd;
  char *text;
  double theta;
  double timestamp;
  char host[10];
} carmen_imp_intro_gui_msg;

#define CARMEN_IMP_INTRO_GUI_MSG_NAME "carmen_imp_intro_gui_msg"
#define CARMEN_IMP_INTRO_GUI_MSG_FMT  "{int, string, double, double, [char:10]}"

typedef struct {
  int answer;
  double timestamp;
  char host[10];
} carmen_imp_intro_answer_msg;

#define CARMEN_IMP_INTRO_ANSWER_MSG_NAME "carmen_imp_intro_answer_msg"
#define CARMEN_IMP_INTRO_ANSWER_MSG_FMT  "{int, double, [char:10]}"


#ifdef __cplusplus
}
#endif

#endif
