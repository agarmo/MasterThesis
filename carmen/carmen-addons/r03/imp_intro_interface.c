
#include <carmen/carmen.h>
#include "imp_intro_interface.h"


static carmen_imp_intro_gui_msg *gui_msg_ptr_ext = NULL;
static carmen_handler_t gui_msg_handler_ext = NULL;


static void gui_msg_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
			    void *clientData __attribute__ ((unused))) {
  
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);

  if (gui_msg_ptr_ext)
    err = IPC_unmarshallData(formatter, callData, gui_msg_ptr_ext,
                             sizeof(carmen_imp_intro_gui_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall", IPC_msgInstanceName(msgRef));

  if (gui_msg_handler_ext)
    gui_msg_handler_ext(gui_msg_ptr_ext);
}

void carmen_imp_intro_subscribe_gui_message(carmen_imp_intro_gui_msg *msg,
					    carmen_handler_t handler,
					    carmen_subscribe_t subscribe_how) {

  IPC_RETURN_TYPE err = IPC_OK;  

  if(subscribe_how == CARMEN_UNSUBSCRIBE) {
    IPC_unsubscribe(CARMEN_IMP_INTRO_GUI_MSG_NAME, 
		    gui_msg_handler);
    return;
  }

  if(msg)
    gui_msg_ptr_ext = msg;
  else {
    gui_msg_ptr_ext = (carmen_imp_intro_gui_msg *)
      calloc(1, sizeof(carmen_imp_intro_gui_msg));
    carmen_test_alloc(gui_msg_ptr_ext);
  }

  gui_msg_handler_ext = handler;
  err = IPC_subscribe(CARMEN_IMP_INTRO_GUI_MSG_NAME,
		      gui_msg_handler, NULL);

  if(subscribe_how == CARMEN_SUBSCRIBE_LATEST)
    IPC_setMsgQueueLength(CARMEN_IMP_INTRO_GUI_MSG_NAME, 1);
  else
    IPC_setMsgQueueLength(CARMEN_IMP_INTRO_GUI_MSG_NAME, 100);

  carmen_test_ipc(err, "Could not subscribe", CARMEN_IMP_INTRO_GUI_MSG_NAME);
}

void carmen_imp_intro_answer(int answer) {

  IPC_RETURN_TYPE err = IPC_OK;
  carmen_imp_intro_answer_msg msg;

  msg.answer = answer;
  msg.timestamp = carmen_get_time_ms();
  strcpy(msg.host, carmen_get_tenchar_host_name());

  err = IPC_publishData(CARMEN_IMP_INTRO_ANSWER_MSG_NAME, &msg);
  carmen_test_ipc(err, "Could not publish", CARMEN_IMP_INTRO_ANSWER_MSG_NAME);
}

