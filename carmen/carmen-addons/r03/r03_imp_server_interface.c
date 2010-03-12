#include <carmen/carmen.h>
#include "r03_imp_server_interface.h"


static carmen_r03_imp_carmen_status_msg *carmen_status_msg_ptr_ext = NULL;
static carmen_handler_t carmen_status_msg_handler_ext = NULL;


void carmen_r03_imp_start_session(int user_id, int walk_num, int config) {

  IPC_RETURN_TYPE err = IPC_OK;
  carmen_r03_imp_start_session_msg msg;

  msg.timestamp = carmen_get_time_ms();
  strcpy(msg.host, carmen_get_tenchar_host_name());

  msg.user_id = user_id;
  msg.walk_num = walk_num;
  msg.config = config;

  err = IPC_defineMsg(CARMEN_R03_IMP_START_SESSION_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_R03_IMP_START_SESSION_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_R03_IMP_START_SESSION_MSG_NAME);

  err = IPC_publishData(CARMEN_R03_IMP_START_SESSION_MSG_NAME, &msg);
  carmen_test_ipc(err, "Could not publish", CARMEN_R03_IMP_START_SESSION_MSG_NAME);
}

void carmen_r03_imp_finish_session() {

  IPC_RETURN_TYPE err = IPC_OK;
  carmen_r03_imp_finish_session_msg msg;

  msg.timestamp = carmen_get_time_ms();
  strcpy(msg.host, carmen_get_tenchar_host_name());

  err = IPC_defineMsg(CARMEN_R03_IMP_FINISH_SESSION_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_R03_IMP_FINISH_SESSION_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_R03_IMP_FINISH_SESSION_MSG_NAME);

  err = IPC_publishData(CARMEN_R03_IMP_FINISH_SESSION_MSG_NAME, &msg);
  carmen_test_ipc(err, "Could not publish", CARMEN_R03_IMP_FINISH_SESSION_MSG_NAME);
}

void carmen_r03_imp_play_intro_file(char *intro_file) {

  IPC_RETURN_TYPE err = IPC_OK;
  carmen_r03_imp_intro_msg msg;

  msg.timestamp = carmen_get_time_ms();
  strcpy(msg.host, carmen_get_tenchar_host_name());

  msg.intro_file = intro_file;

  err = IPC_defineMsg(CARMEN_R03_IMP_INTRO_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_R03_IMP_INTRO_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_R03_IMP_INTRO_MSG_NAME);

  err = IPC_publishData(CARMEN_R03_IMP_INTRO_MSG_NAME, &msg);
  carmen_test_ipc(err, "Could not publish", CARMEN_R03_IMP_INTRO_MSG_NAME);
}

void carmen_r03_imp_shutdown() {

  IPC_RETURN_TYPE err = IPC_OK;
  carmen_r03_imp_shutdown_msg msg;

  msg.timestamp = carmen_get_time_ms();
  strcpy(msg.host, carmen_get_tenchar_host_name());

  err = IPC_defineMsg(CARMEN_R03_IMP_SHUTDOWN_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_R03_IMP_SHUTDOWN_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_R03_IMP_SHUTDOWN_MSG_NAME);

  err = IPC_publishData(CARMEN_R03_IMP_SHUTDOWN_MSG_NAME, &msg);
  carmen_test_ipc(err, "Could not publish", CARMEN_R03_IMP_SHUTDOWN_MSG_NAME);
}

static void carmen_status_msg_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
				      void *clientData __attribute__ ((unused))) {
  
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);

  if (carmen_status_msg_ptr_ext)
    err = IPC_unmarshallData(formatter, callData, carmen_status_msg_ptr_ext,
                             sizeof(carmen_r03_imp_carmen_status_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall", IPC_msgInstanceName(msgRef));

  if (carmen_status_msg_handler_ext)
    carmen_status_msg_handler_ext(carmen_status_msg_ptr_ext);
}

void carmen_r03_imp_subscribe_carmen_status(carmen_r03_imp_carmen_status_msg *msg,
					    carmen_handler_t handler,
					    carmen_subscribe_t subscribe_how) {

  IPC_RETURN_TYPE err = IPC_OK;  

  err = IPC_defineMsg(CARMEN_R03_IMP_CARMEN_STATUS_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_R03_IMP_CARMEN_STATUS_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_R03_IMP_CARMEN_STATUS_MSG_NAME);  

  if(subscribe_how == CARMEN_UNSUBSCRIBE) {
    IPC_unsubscribe(CARMEN_R03_IMP_CARMEN_STATUS_MSG_NAME, 
		    carmen_status_msg_handler);
    return;
  }

  if(msg)
    carmen_status_msg_ptr_ext = msg;
  else {
    carmen_status_msg_ptr_ext = (carmen_r03_imp_carmen_status_msg *)
      calloc(1, sizeof(carmen_r03_imp_carmen_status_msg));
    carmen_test_alloc(carmen_status_msg_ptr_ext);
  }

  carmen_status_msg_handler_ext = handler;
  err = IPC_subscribe(CARMEN_R03_IMP_CARMEN_STATUS_MSG_NAME,
		      carmen_status_msg_handler, NULL);

  if(subscribe_how == CARMEN_SUBSCRIBE_LATEST)
    IPC_setMsgQueueLength(CARMEN_R03_IMP_CARMEN_STATUS_MSG_NAME, 1);
  else
    IPC_setMsgQueueLength(CARMEN_R03_IMP_CARMEN_STATUS_MSG_NAME, 100);

  carmen_test_ipc(err, "Could not subscribe", CARMEN_R03_IMP_CARMEN_STATUS_MSG_NAME);
}
