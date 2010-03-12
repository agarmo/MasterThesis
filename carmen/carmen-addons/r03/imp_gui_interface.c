
#include <carmen/carmen.h>
#include "imp_gui_interface.h"


static carmen_imp_gui_msg *gui_msg_ptr_ext = NULL;
static carmen_handler_t gui_msg_handler_ext = NULL;


static void define_messages() {

  static int first = 1;
  IPC_RETURN_TYPE err;

  if (first) {
    first = 0;
    err = IPC_defineMsg(CARMEN_IMP_GUI_MSG_NAME, IPC_VARIABLE_LENGTH,
			CARMEN_IMP_GUI_MSG_FMT);
    carmen_test_ipc_exit(err, "Could not define", CARMEN_IMP_GUI_MSG_NAME);
    
    err = IPC_defineMsg(CARMEN_IMP_GUI_ANSWER_MSG_NAME, IPC_VARIABLE_LENGTH, 
			CARMEN_IMP_GUI_ANSWER_MSG_FMT);
    carmen_test_ipc_exit(err, "Could not define", CARMEN_IMP_GUI_ANSWER_MSG_NAME);
  }
}

static void gui_msg_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
			    void *clientData __attribute__ ((unused))) {
  
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);

  if (gui_msg_ptr_ext)
    err = IPC_unmarshallData(formatter, callData, gui_msg_ptr_ext,
                             sizeof(carmen_imp_gui_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall", IPC_msgInstanceName(msgRef));

  if (gui_msg_handler_ext)
    gui_msg_handler_ext(gui_msg_ptr_ext);
}

void carmen_imp_gui_subscribe_gui_message(carmen_imp_gui_msg *msg,
					  carmen_handler_t handler,
					  carmen_subscribe_t subscribe_how) {

  IPC_RETURN_TYPE err = IPC_OK;  

  define_messages();

  if(subscribe_how == CARMEN_UNSUBSCRIBE) {
    IPC_unsubscribe(CARMEN_IMP_GUI_MSG_NAME, 
		    gui_msg_handler);
    return;
  }

  if(msg)
    gui_msg_ptr_ext = msg;
  else {
    gui_msg_ptr_ext = (carmen_imp_gui_msg *)
      calloc(1, sizeof(carmen_imp_gui_msg));
    carmen_test_alloc(gui_msg_ptr_ext);
  }

  gui_msg_handler_ext = handler;
  err = IPC_subscribe(CARMEN_IMP_GUI_MSG_NAME,
		      gui_msg_handler, NULL);

  if(subscribe_how == CARMEN_SUBSCRIBE_LATEST)
    IPC_setMsgQueueLength(CARMEN_IMP_GUI_MSG_NAME, 1);
  else
    IPC_setMsgQueueLength(CARMEN_IMP_GUI_MSG_NAME, 100);

  carmen_test_ipc(err, "Could not subscribe", CARMEN_IMP_GUI_MSG_NAME);
}

void carmen_imp_gui_answer(int answer) {

  IPC_RETURN_TYPE err = IPC_OK;
  carmen_imp_gui_answer_msg msg;

  define_messages();

  msg.answer = answer;
  msg.timestamp = carmen_get_time_ms();
  strcpy(msg.host, carmen_get_tenchar_host_name());

  err = IPC_publishData(CARMEN_IMP_GUI_ANSWER_MSG_NAME, &msg);
  carmen_test_ipc(err, "Could not publish", CARMEN_IMP_GUI_ANSWER_MSG_NAME);
}

void carmen_imp_gui_publish_text_message(char *text) {

  IPC_RETURN_TYPE err = IPC_OK;
  carmen_imp_gui_msg msg;

  define_messages();

  msg.cmd = CARMEN_IMP_GUI_TEXT;
  msg.text = text;
  msg.timestamp = carmen_get_time_ms();
  strcpy(msg.host, carmen_get_tenchar_host_name());

  err = IPC_publishData(CARMEN_IMP_GUI_MSG_NAME, &msg);
  carmen_test_ipc(err, "Could not publish", CARMEN_IMP_GUI_MSG_NAME);
}

void carmen_imp_gui_publish_question_message(char *text) {

  IPC_RETURN_TYPE err = IPC_OK;
  carmen_imp_gui_msg msg;

  define_messages();

  msg.cmd = CARMEN_IMP_GUI_QUESTION;
  msg.text = text;
  msg.timestamp = carmen_get_time_ms();
  strcpy(msg.host, carmen_get_tenchar_host_name());

  err = IPC_publishData(CARMEN_IMP_GUI_MSG_NAME, &msg);
  carmen_test_ipc(err, "Could not publish", CARMEN_IMP_GUI_MSG_NAME);
}

void carmen_imp_gui_publish_arrow_message(int type, double theta) {

  IPC_RETURN_TYPE err = IPC_OK;
  carmen_imp_gui_msg msg;
  static char tmp[2] = " ";

  define_messages();

  msg.cmd = CARMEN_IMP_GUI_ARROW;
  msg.text = tmp;
  msg.arrow_theta = theta;
  msg.arrow_type = type;

  msg.timestamp = carmen_get_time_ms();
  strcpy(msg.host, carmen_get_tenchar_host_name());

  err = IPC_publishData(CARMEN_IMP_GUI_MSG_NAME, &msg);
  carmen_test_ipc(err, "Could not publish", CARMEN_IMP_GUI_MSG_NAME);
}

