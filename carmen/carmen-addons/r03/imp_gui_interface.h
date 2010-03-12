
#ifndef IMP_GUI_INTERFACE_H
#define IMP_GUI_INTERFACE_H

#include "imp_gui_messages.h"

#ifdef __cplusplus
extern "C" {
#endif


void carmen_imp_gui_subscribe_gui_message(carmen_imp_gui_msg *msg,
					  carmen_handler_t handler,
					  carmen_subscribe_t subscribe_how);

void carmen_imp_gui_answer(int answer);

void carmen_imp_gui_publish_text_message(char *text);
void carmen_imp_gui_publish_question_message(char *text);
void carmen_imp_gui_publish_arrow_message(int type, double theta);


#ifdef __cplusplus
}
#endif

#endif
