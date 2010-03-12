
#ifndef IMP_GUI_MESSAGES_H
#define IMP_GUI_MESSAGES_H

#ifdef __cplusplus
extern "C" {
#endif


#define CARMEN_IMP_GUI_ARROW     1
#define CARMEN_IMP_GUI_QUESTION  2
#define CARMEN_IMP_GUI_TEXT      3

#define CARMEN_IMP_GUI_ARROW_NONE      0
#define CARMEN_IMP_GUI_ARROW_STRAIGHT  1
#define CARMEN_IMP_GUI_ARROW_LEFT      2
#define CARMEN_IMP_GUI_ARROW_RIGHT     3

typedef struct {
  int cmd;
  char *text;
  int arrow_type;
  double arrow_theta;
  double timestamp;
  char host[10];
} carmen_imp_gui_msg;

#define CARMEN_IMP_GUI_MSG_NAME "carmen_imp_gui_msg"
#define CARMEN_IMP_GUI_MSG_FMT  "{int, string, int, double, double, [char:10]}"

typedef struct {
  int answer;
  double timestamp;
  char host[10];
} carmen_imp_gui_answer_msg;

#define CARMEN_IMP_GUI_ANSWER_MSG_NAME "carmen_imp_gui_answer_msg"
#define CARMEN_IMP_GUI_ANSWER_MSG_FMT  "{int, double, [char:10]}"


#ifdef __cplusplus
}
#endif

#endif
