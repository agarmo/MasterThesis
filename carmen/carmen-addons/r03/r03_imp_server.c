#include <carmen/carmen.h>
#include "r03_imp_server_messages.h"
#include <sys/wait.h>
#include <sys/types.h>

static void play_intro_file(char *intro_file) {

  int pid, status;

  pid = fork();
  if (pid == 0) {
    execlp("imp_intro", "imp_intro", "--daemon", intro_file, NULL);
    carmen_die_syserror("Could not exec imp_intro");
  }
  else if (pid == -1)
    carmen_die_syserror("Could not fork to start imp_intro");
  else
    waitpid(pid, &status, 0);  
}

static void start_session(int user_id, int walk_num, int config) {

  int pid, status, pfirst;
  char user_id_buf[8];
  char walk_num_buf[8];
  char config_buf[32];

  pid = fork();
  if (pid == 0) {
    sprintf(user_id_buf, "%d", user_id);
    sprintf(walk_num_buf, "%d", walk_num);
    if (config == NO_GUIDANCE)
      sprintf(config_buf, "baseline");
    pfirst = 1;
    if (config & AUDIO_GUIDANCE) {
      if (pfirst) {
	pfirst = 0;
	sprintf(config_buf, "audio");
      }
      else
	strcat(config_buf, "_audio");
    }
    if (config & ARROW_GUIDANCE) {
      if (pfirst) {
	pfirst = 0;
	sprintf(config_buf, "arrow");
      }
      else
	strcat(config_buf, "_arrow");
    }
    if (config & TEXT_GUIDANCE) {
      if (pfirst) {
	pfirst = 0;
	sprintf(config_buf, "text");
      }
      else
	strcat(config_buf, "_text");
    }
    if (config & WOZ_GUIDANCE) {
      if (pfirst) {
	pfirst = 0;
	sprintf(config_buf, "woz");
      }
      else
	strcat(config_buf, "_woz");
    }
    execlp("/home/jrod/r03_imp_start_guidance.sh", "r03_imp_start_guidance.sh", user_id_buf, walk_num_buf, config_buf, NULL);
    carmen_die_syserror("Could not exec r03_imp_start_guidance.sh");
  }
  else if (pid == -1)
    carmen_die_syserror("Could not fork to start r03_imp_start_guidance.sh");
  else
    waitpid(pid, &status, 0);  
}

static void finish_session() {

  int pid, status;

  pid = fork();
  if (pid == 0) {
    execlp("/home/jrod/r03_imp_stop_guidance.sh", "r03_imp_stop_guidance.sh", NULL);
    carmen_die_syserror("Could not exec r03_imp_stop_guidance.sh");
  }
  else if (pid == -1)
    carmen_die_syserror("Could not fork to start r03_imp_stop_guidance.sh");
  else
    waitpid(pid, &status, 0);  
}

static void play_intro_file_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
				    void *clientData __attribute__ ((unused))) {

  carmen_r03_imp_intro_msg msg;
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &msg,
			   sizeof(carmen_r03_imp_intro_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall", IPC_msgInstanceName(msgRef));

  play_intro_file(msg.intro_file);
}

static void start_session_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
				  void *clientData __attribute__ ((unused))) {

  carmen_r03_imp_start_session_msg msg;
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &msg,
			   sizeof(carmen_r03_imp_start_session_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall", IPC_msgInstanceName(msgRef));

  start_session(msg.user_id, msg.walk_num, msg.config);
}

static void finish_session_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
				   void *clientData __attribute__ ((unused))) {

  carmen_r03_imp_finish_session_msg msg;
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &msg,
			   sizeof(carmen_r03_imp_finish_session_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall", IPC_msgInstanceName(msgRef));

  finish_session();
}

static void ipc_init() {

  IPC_RETURN_TYPE err;

  err = IPC_defineMsg(CARMEN_R03_IMP_INTRO_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_R03_IMP_INTRO_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_R03_IMP_INTRO_MSG_NAME);

  err = IPC_subscribe(CARMEN_R03_IMP_INTRO_MSG_NAME, 
		      play_intro_file_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subcribe", 
		       CARMEN_R03_IMP_INTRO_MSG_NAME);
  IPC_setMsgQueueLength(CARMEN_R03_IMP_INTRO_MSG_NAME, 10);

  err = IPC_defineMsg(CARMEN_R03_IMP_START_SESSION_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_R03_IMP_START_SESSION_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_R03_IMP_START_SESSION_MSG_NAME);

  err = IPC_subscribe(CARMEN_R03_IMP_START_SESSION_MSG_NAME, 
		      start_session_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subcribe", 
		       CARMEN_R03_IMP_START_SESSION_MSG_NAME);
  IPC_setMsgQueueLength(CARMEN_R03_IMP_START_SESSION_MSG_NAME, 10);

  err = IPC_defineMsg(CARMEN_R03_IMP_FINISH_SESSION_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_R03_IMP_FINISH_SESSION_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_R03_IMP_FINISH_SESSION_MSG_NAME);

  err = IPC_subscribe(CARMEN_R03_IMP_FINISH_SESSION_MSG_NAME, 
		      finish_session_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subcribe", 
		       CARMEN_R03_IMP_FINISH_SESSION_MSG_NAME);
  IPC_setMsgQueueLength(CARMEN_R03_IMP_FINISH_SESSION_MSG_NAME, 10);
}

void shutdown_module(int sig) {

  if (sig == SIGINT) {
    close_ipc();
    fprintf(stderr, "\nDisconnecting.\n");
    exit(0);
  }
}

void sigchld_handler() {

  int status;

  waitpid(-1, &status, WNOHANG);
}

int main(int argc, char **argv) {

  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);

  carmen_randomize(&argc, &argv);
  
  signal(SIGCHLD, sigchld_handler);
  signal(SIGINT, shutdown_module); 

  if (argc > 1 && (!strcmp(argv[1], "--daemon") || !strcmp(argv[1], "-d")))
    daemon(0,0);

  ipc_init();
  
  IPC_dispatch();
  
  return 0;
}
