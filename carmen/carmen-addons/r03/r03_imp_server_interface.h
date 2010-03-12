#ifndef R03_IMP_SERVER_INTERFACE_H
#define R03_IMP_SERVER_INTERFACE_H

#include "r03_imp_server_messages.h"

#ifdef __cplusplus
extern "C" {
#endif


void carmen_r03_imp_start_session(int user_id, int walk_num, int config);
void carmen_r03_imp_finish_session();
void carmen_r03_imp_subscribe_carmen_status(carmen_r03_imp_carmen_status_msg *msg,
					    carmen_handler_t handler,
					    carmen_subscribe_t subscribe_how);
void carmen_r03_imp_play_intro_file(char *intro_file);
void carmen_r03_imp_shutdown();


#ifdef __cplusplus
}
#endif

#endif
