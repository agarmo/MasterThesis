#include <carmen/carmen.h>
#include "neck_messages.h"

#define NECK_PER_RADIANS (0.5*(180.0/M_PI))
#define rad2neck(theta) ((int)((theta)*NECK_PER_RADIANS))


// forward is 0.0 radians
static void move_neck(double theta) {
  
  printf("do EXPR(0.5, shake(%d))\n", rad2neck(theta));
  fflush(0);
}

static void command_neck(char *cmd) {
  
  printf("%s\n", cmd);
  fflush(0);
}

static void neck_move_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
			      void *clientData __attribute__ ((unused))) {

  carmen_neck_move_msg msg;
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &msg,
			   sizeof(carmen_neck_move_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall", IPC_msgInstanceName(msgRef));

  move_neck(msg.theta);
}

static void neck_command_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
				 void *clientData __attribute__ ((unused))) {

  carmen_neck_command_msg msg;
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &msg,
			   sizeof(carmen_neck_command_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall", IPC_msgInstanceName(msgRef));

  command_neck(msg.cmd);
}

static void ipc_init() {

  IPC_RETURN_TYPE err;

  err = IPC_defineMsg(CARMEN_NECK_MOVE_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_NECK_MOVE_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_NECK_MOVE_MSG_NAME);

  err = IPC_subscribe(CARMEN_NECK_MOVE_MSG_NAME, 
		      neck_move_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subcribe", 
		       CARMEN_NECK_MOVE_MSG_NAME);
  IPC_setMsgQueueLength(CARMEN_NECK_MOVE_MSG_NAME, 1);

  err = IPC_defineMsg(CARMEN_NECK_COMMAND_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_NECK_COMMAND_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_NECK_COMMAND_MSG_NAME);

  err = IPC_subscribe(CARMEN_NECK_COMMAND_MSG_NAME, 
		      neck_command_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subcribe", 
		       CARMEN_NECK_COMMAND_MSG_NAME);
  IPC_setMsgQueueLength(CARMEN_NECK_COMMAND_MSG_NAME, 1);
}

void shutdown_module(int sig) {

  if (sig == SIGINT) {
    close_ipc();
    fprintf(stderr, "\nDisconnecting.\n");
    exit(0);
  }
}

int main(int argc __attribute__ ((unused)), char **argv) {

  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);
  
  signal(SIGINT, shutdown_module); 

  ipc_init();
  
  IPC_dispatch();
  
  return 0;
}
