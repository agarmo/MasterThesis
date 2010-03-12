
#ifndef IMP_INTRO_INTERFACE_H
#define IMP_INTRO_INTERFACE_H

#include "imp_intro_messages.h"

#ifdef __cplusplus
extern "C" {
#endif


void carmen_imp_intro_subscribe_gui_message(carmen_imp_intro_gui_msg *msg,
					    carmen_handler_t handler,
					    carmen_subscribe_t subscribe_how);

void carmen_imp_intro_answer(int answer);


#ifdef __cplusplus
}
#endif

#endif
