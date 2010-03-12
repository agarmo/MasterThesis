#include <carmen/carmen.h>
#include "sound_server_messages.h"
#include <sys/wait.h>
#include <sys/types.h>
#ifdef HAVE_THETA
#include <theta.h>
#endif

static void play_file(char *filename) {

  int speech_pid, status;

  speech_pid = fork();
  if (speech_pid == 0)
    {
      execlp("play", "play", filename, NULL);
      carmen_die_syserror("Could not exec play");
    }
  else if (speech_pid == -1)
    carmen_die_syserror("Could not fork to start play");
  else
    waitpid(speech_pid, &status, 0);
}

static void speech_synth(char *text) {

#ifdef NO_THETA
  text = NULL;
  carmen_warn("Error: Complied without theta; cannot do speech synthesis!\n");  
#else
  static cst_voice *vox;
  static theta_voice_desc *vlist;
  static int first = 1;
  static char tmp_filename[100];

  sprintf(tmp_filename, "/home/jrod/wavs/.tmp%d.wav", carmen_int_random(99999));

  if (first) {
    vlist = theta_enum_voices(NULL, NULL);
    first = 0;
  }
  
  vox = theta_load_voice(vlist);
  while (vox == NULL) {
    printf("ERROR: vox == NULL!\n");
    vox = theta_load_voice(vlist);
    usleep(100000);
  }
  
  //theta_set_pitch_shift(vox, 0.1, NULL);
  //theta_set_rate_stretch(vox, 1.2, NULL);
  
  theta_set_outfile(vox, tmp_filename, "riff", 0, 2);
  theta_tts(s, vox);
  theta_unload_voice(vox);

  play_file(tmp_filename);
#endif
}

static void set_volume(int volume) {

  int pid, status;
  char opts[100];

  sprintf(opts, "-v%d", volume);

  pid = fork();
  if (pid == 0) {
    execlp("aumix", "aumix", opts, NULL);
    carmen_die_syserror("Could not exec aumix");
  }
  else if (pid == -1)
    carmen_die_syserror("Could not fork to start aumix");
  else
    waitpid(pid, &status, 0);  
}

static void play_file_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
			      void *clientData __attribute__ ((unused))) {

  carmen_sound_file_msg msg;
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &msg,
			   sizeof(carmen_sound_file_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall", IPC_msgInstanceName(msgRef));

  play_file(msg.filename);
}

static void speech_synth_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
				 void *clientData __attribute__ ((unused))) {

  carmen_sound_speech_synth_msg msg;
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &msg,
			   sizeof(carmen_sound_speech_synth_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall", IPC_msgInstanceName(msgRef));

  speech_synth(msg.text);
}

static void volume_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
			   void *clientData __attribute__ ((unused))) {

  carmen_sound_volume_msg msg;
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &msg,
			   sizeof(carmen_sound_volume_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall", IPC_msgInstanceName(msgRef));

  set_volume(msg.volume);
}

static void ipc_init() {

  IPC_RETURN_TYPE err;

  err = IPC_defineMsg(CARMEN_SOUND_FILE_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_SOUND_FILE_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_SOUND_FILE_MSG_NAME);

  err = IPC_subscribe(CARMEN_SOUND_FILE_MSG_NAME, 
		      play_file_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subcribe", 
		       CARMEN_SOUND_FILE_MSG_NAME);
  IPC_setMsgQueueLength(CARMEN_SOUND_FILE_MSG_NAME, 10);

  err = IPC_defineMsg(CARMEN_SOUND_SPEECH_SYNTH_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_SOUND_SPEECH_SYNTH_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_SOUND_SPEECH_SYNTH_MSG_NAME);

  err = IPC_subscribe(CARMEN_SOUND_SPEECH_SYNTH_MSG_NAME, 
		      speech_synth_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subcribe", 
		       CARMEN_SOUND_SPEECH_SYNTH_MSG_NAME);
  IPC_setMsgQueueLength(CARMEN_SOUND_SPEECH_SYNTH_MSG_NAME, 10);

  err = IPC_defineMsg(CARMEN_SOUND_VOLUME_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_SOUND_VOLUME_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_SOUND_VOLUME_MSG_NAME);

  err = IPC_subscribe(CARMEN_SOUND_VOLUME_MSG_NAME, 
		      volume_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subcribe", 
		       CARMEN_SOUND_VOLUME_MSG_NAME);
  IPC_setMsgQueueLength(CARMEN_SOUND_VOLUME_MSG_NAME, 1);
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

#ifdef HAVE_THETA
  theta_init(NULL);
#endif

  if (argc > 1 && (!strcmp(argv[1], "--daemon") || !strcmp(argv[1], "-d")))
    daemon(0,0);

  ipc_init();
  
  IPC_dispatch();
  
  return 0;
}
