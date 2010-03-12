
#include <carmen/carmen.h>
#include "stracker_interface.h"


static carmen_stracker_pos_msg *pos_msg_ptr_ext = NULL;
static carmen_handler_t pos_msg_handler_ext = NULL;


static void pos_msg_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
			    void *clientData __attribute__ ((unused))) {
  
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);

  if (pos_msg_ptr_ext)
    err = IPC_unmarshallData(formatter, callData, pos_msg_ptr_ext,
                             sizeof(carmen_stracker_pos_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall",
			 IPC_msgInstanceName(msgRef));

  if (pos_msg_handler_ext)
    pos_msg_handler_ext(pos_msg_ptr_ext);
}

void carmen_stracker_subscribe_pos_message(carmen_stracker_pos_msg *msg,
					   carmen_handler_t handler,
					   carmen_subscribe_t subscribe_how) {

  IPC_RETURN_TYPE err = IPC_OK;  

  if(subscribe_how == CARMEN_UNSUBSCRIBE) {
    IPC_unsubscribe(CARMEN_STRACKER_POS_MSG_NAME, 
		    pos_msg_handler);
    return;
  }

  if (msg)
    pos_msg_ptr_ext = msg;
  else {
    pos_msg_ptr_ext = (carmen_stracker_pos_msg *)
      calloc(1, sizeof(carmen_stracker_pos_msg));
    carmen_test_alloc(pos_msg_ptr_ext);
  }

  pos_msg_handler_ext = handler;
  err = IPC_subscribe(CARMEN_STRACKER_POS_MSG_NAME, 
		      pos_msg_handler, NULL);

  if(subscribe_how == CARMEN_SUBSCRIBE_LATEST)
    IPC_setMsgQueueLength(CARMEN_STRACKER_POS_MSG_NAME, 1);
  else
    IPC_setMsgQueueLength(CARMEN_STRACKER_POS_MSG_NAME, 100);

  carmen_test_ipc(err, "Could not subscribe", CARMEN_STRACKER_POS_MSG_NAME);
}

void carmen_stracker_start() {

  IPC_RETURN_TYPE err;
  static carmen_stracker_start_msg msg;

  err = IPC_defineMsg(CARMEN_STRACKER_START_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_STRACKER_START_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define message", 
		       CARMEN_STRACKER_START_MSG_NAME);
  
  strcpy(msg.host, carmen_get_tenchar_host_name());
  msg.timestamp = carmen_get_time_ms();
  err = IPC_publishData(CARMEN_STRACKER_START_MSG_NAME, &msg);
  carmen_test_ipc_return(err, "Could not start tracking", 
			 CARMEN_STRACKER_START_MSG_NAME);
}
