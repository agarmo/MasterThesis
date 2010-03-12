
#include <carmen/carmen.h>
#include "roomnav_guidance_interface.h"


static carmen_roomnav_guidance_msg *guidance_msg_ptr_ext = NULL;
static carmen_handler_t guidance_msg_handler_ext = NULL;


static void guidance_msg_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
				 void *clientData __attribute__ ((unused))) {
  
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);

  if (guidance_msg_ptr_ext)
    err = IPC_unmarshallData(formatter, callData, guidance_msg_ptr_ext,
                             sizeof(carmen_roomnav_guidance_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall", IPC_msgInstanceName(msgRef));

  if (guidance_msg_handler_ext)
    guidance_msg_handler_ext(guidance_msg_ptr_ext);
}

void carmen_roomnav_subscribe_guidance_message(carmen_roomnav_guidance_msg *msg,
					carmen_handler_t handler,
					carmen_subscribe_t subscribe_how) {

  IPC_RETURN_TYPE err = IPC_OK;  

  if(subscribe_how == CARMEN_UNSUBSCRIBE) {
    IPC_unsubscribe(CARMEN_ROOMNAV_GUIDANCE_MSG_NAME, 
		    guidance_msg_handler);
    return;
  }

  if(msg)
    guidance_msg_ptr_ext = msg;
  else {
    guidance_msg_ptr_ext = (carmen_roomnav_guidance_msg *)
      calloc(1, sizeof(carmen_roomnav_guidance_msg));
    carmen_test_alloc(guidance_msg_ptr_ext);
  }

  guidance_msg_handler_ext = handler;
  err = IPC_subscribe(CARMEN_ROOMNAV_GUIDANCE_MSG_NAME, 
		      guidance_msg_handler, NULL);

  if(subscribe_how == CARMEN_SUBSCRIBE_LATEST)
    IPC_setMsgQueueLength(CARMEN_ROOMNAV_GUIDANCE_MSG_NAME, 1);
  else
    IPC_setMsgQueueLength(CARMEN_ROOMNAV_GUIDANCE_MSG_NAME, 100);

  carmen_test_ipc(err, "Could not subscribe", CARMEN_ROOMNAV_GUIDANCE_MSG_NAME);
}

void carmen_roomnav_guidance_config(int text, int audio) {

  IPC_RETURN_TYPE err = IPC_OK;
  carmen_roomnav_guidance_config_msg msg;

  msg.timestamp = carmen_get_time_ms();
  strcpy(msg.host, carmen_get_tenchar_host_name());

  msg.text = text;
  msg.audio = audio;

  err = IPC_publishData(CARMEN_ROOMNAV_GUIDANCE_CONFIG_MSG_NAME, &msg);
  carmen_test_ipc(err, "Could not publish", CARMEN_ROOMNAV_GUIDANCE_CONFIG_MSG_NAME);
}
