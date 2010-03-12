#include <carmen/carmen.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <theta.h>
#include "imp_gui_interface.h"


static FILE *fp;
static int waiting_for_answer = 0;
static int answer = 0;
static int r03_session_num = 0;

static char *format_gui_text(char *text) {

  char *pos;
  int len;

  pos = text;
  while (1) {
    pos = strstr(pos, "NL");
    if (pos == NULL)
      break;
    len = 2;
    if (*(pos+2) == ' ' || *(pos+2) == '\t')
      len++;
    if (pos > text && (*(pos-1) == ' ' || *(pos-1) == '\t')) {
      pos--;
      len++;
    }
    *pos = '\n';
    memmove(pos+1, pos+len, sizeof(char)*(strlen(pos+len) + 1));
    pos += len;
  }

  return text;
}

static void answer_msg_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
			       void *clientData __attribute__ ((unused))) {

  carmen_imp_gui_answer_msg msg;
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &msg,
			   sizeof(carmen_imp_gui_answer_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall", IPC_msgInstanceName(msgRef));

  answer = msg.answer;
  waiting_for_answer = 0;

  printf("  --> answer = %d\n", answer);
}

static void ipc_init() {

  IPC_RETURN_TYPE err;

  err = IPC_defineMsg(CARMEN_IMP_GUI_MSG_NAME, IPC_VARIABLE_LENGTH,
		      CARMEN_IMP_GUI_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_IMP_GUI_MSG_NAME);
  
  err = IPC_defineMsg(CARMEN_IMP_GUI_ANSWER_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_IMP_GUI_ANSWER_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_IMP_GUI_ANSWER_MSG_NAME);
  
  err = IPC_subscribe(CARMEN_IMP_GUI_ANSWER_MSG_NAME, 
		      answer_msg_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subcribe", 
		       CARMEN_IMP_GUI_ANSWER_MSG_NAME);
  IPC_setMsgQueueLength(CARMEN_IMP_GUI_ANSWER_MSG_NAME, 1);
}

void shutdown_module(int sig) {

  if (sig == SIGINT) {
    fprintf(stderr, "\nDisconnecting.\n");
    if (fp != NULL)
      fclose(fp);
    close_ipc();
    exit(0);
  }
}

/*
void sigchld_handler() {

  int status;

  waitpid(-1, &status, WNOHANG);
}
*/

static void fork_speech(char *s, int question, int answer) {

  static cst_voice *vox;
  static theta_voice_desc *vlist;
  static int first = 1;
  static int num = 0;
  static int num2 = 0;
  char wav_filename[100];
  int speech_pid;
  int status;
  
  //s = NULL;  //dbug

  if (question) {
    if (answer)
      sprintf(wav_filename, "/home/jrod/wavs/IMP%d-%dy%d.wav", r03_session_num, num, num2++);
    else
      sprintf(wav_filename, "/home/jrod/wavs/IMP%d-%dn%d.wav", r03_session_num, num, num2++);
  }
  else {
    num2 = 0;
    sprintf(wav_filename, "/home/jrod/wavs/IMP%d-%d.wav", r03_session_num, num++);
  }

  if (!carmen_file_exists(wav_filename)) {
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

    theta_set_outfile(vox, wav_filename, "riff", 0, 2);
    theta_tts(s, vox);
    theta_unload_voice(vox);
  }

  speech_pid = fork();
  if (speech_pid == 0)
    {
      execlp("play", "play", wav_filename, NULL);
      carmen_die_syserror("Could not exec play");
    }
  else if (speech_pid == -1)
    carmen_die_syserror("Could not fork to start play");
  else
    waitpid(speech_pid, &status, 0);
}

void do_intro() {

  static int ignore = 0;
  static int question_block = 0;
  static char line[2000], token[100];
  static char say_text[500], print_text[500];
  char *mark, *pos;
  int n = 0;

  fgets(line, 2000, fp);
  while (!feof(fp)) {
    if (sscanf(line, "%s", token) == 1) {
      if (!strcmp(token, "ENDIF")) {
	ignore = 0;
	question_block = 0;
      }
      else if (!ignore) {
	if (!strcmp(token, "SAY")) {
	  mark = strstr(line, token) + strlen(token);
	  mark += strspn(mark, " \t");
	  fork_speech(mark, question_block, answer);
	}
	else if (!strcmp(token, "PRINT")) {
	  mark = strstr(line, token) + strlen(token);
	  mark += strspn(mark, " \t");
	  carmen_imp_gui_publish_text_message(format_gui_text(carmen_new_string(mark)));
	}
	else if (!strcmp(token, "QUESTION")) {
	  say_text[0] = print_text[0] = '\0';
	  mark = strstr(line, token) + strlen(token);
	  if (sscanf(mark, "%s", token) == 1) {
	    if (!strcmp(token, "SAY")) {
	      mark = strstr(mark, token) + strlen(token);
	      mark += strspn(mark, " \t");
	      pos = strstr(mark, "PRINT");
	      if (pos)
		n = pos-mark;
	      else
		n = strlen(mark);
	      strncpy(say_text, mark, n);
	      say_text[n] = '\0';
	      if (pos) {
		mark = pos + strlen("PRINT");
		mark += strspn(mark, " \t");
		strcpy(print_text, mark);
	      }
	      else {
		print_text[0] = ' ';
		print_text[1] = '\0';
	      }
	    }
	    else if (!strcmp(token, "PRINT")) {
	      mark = strstr(mark, token) + strlen(token);
	      mark += strspn(mark, " \t");
	      pos = strstr(mark, "SAY");
	      if (pos)
		n = pos-mark;
	      else
		n = strlen(mark);
	      strncpy(print_text, mark, n);
	      print_text[n] = '\0';
	      if (pos) {
		mark = pos + strlen("SAY");
		mark += strspn(mark, " \t");
		strcpy(say_text, mark);
	      }
	    }
	  }
	  carmen_imp_gui_publish_question_message(format_gui_text(carmen_new_string(print_text)));
	  if (strlen(say_text) > 0)
	    fork_speech(say_text, question_block, answer);
	  waiting_for_answer = 1;
	  while (waiting_for_answer)
	    sleep_ipc(0.01);
	}
	else if (!strcmp(token, "SHOWARROW")) {
	  carmen_imp_gui_publish_arrow_message(CARMEN_IMP_GUI_ARROW_STRAIGHT, -0.3);
	  sleep_ipc(0.1);
	  carmen_imp_gui_publish_arrow_message(CARMEN_IMP_GUI_ARROW_STRAIGHT, -0.25);
	  sleep_ipc(0.1);
	  carmen_imp_gui_publish_arrow_message(CARMEN_IMP_GUI_ARROW_STRAIGHT, -0.2);
	  sleep_ipc(0.1);
	  carmen_imp_gui_publish_arrow_message(CARMEN_IMP_GUI_ARROW_STRAIGHT, -0.15);
	  sleep_ipc(0.1);
	  carmen_imp_gui_publish_arrow_message(CARMEN_IMP_GUI_ARROW_STRAIGHT, -0.1);
	  sleep_ipc(0.1);
	  carmen_imp_gui_publish_arrow_message(CARMEN_IMP_GUI_ARROW_STRAIGHT, -0.05);
	  sleep_ipc(0.1);
	  carmen_imp_gui_publish_arrow_message(CARMEN_IMP_GUI_ARROW_STRAIGHT, 0.0);
	  sleep_ipc(0.1);
	  carmen_imp_gui_publish_arrow_message(CARMEN_IMP_GUI_ARROW_STRAIGHT, 0.05);
	  sleep_ipc(0.1);
	  carmen_imp_gui_publish_arrow_message(CARMEN_IMP_GUI_ARROW_STRAIGHT, 0.1);
	  sleep_ipc(0.1);
	  carmen_imp_gui_publish_arrow_message(CARMEN_IMP_GUI_ARROW_STRAIGHT, 0.15);
	  sleep_ipc(0.1);
	  carmen_imp_gui_publish_arrow_message(CARMEN_IMP_GUI_ARROW_STRAIGHT, 0.2);
	  sleep_ipc(0.1);
	  carmen_imp_gui_publish_arrow_message(CARMEN_IMP_GUI_ARROW_STRAIGHT, 0.25);
	  sleep_ipc(0.1);
	  carmen_imp_gui_publish_arrow_message(CARMEN_IMP_GUI_ARROW_STRAIGHT, 0.3);
	  sleep_ipc(0.1);
	  carmen_imp_gui_publish_arrow_message(CARMEN_IMP_GUI_ARROW_STRAIGHT, 0.25);
	  sleep_ipc(0.1);
	  carmen_imp_gui_publish_arrow_message(CARMEN_IMP_GUI_ARROW_STRAIGHT, 0.2);
	  sleep_ipc(0.1);
	  carmen_imp_gui_publish_arrow_message(CARMEN_IMP_GUI_ARROW_STRAIGHT, 0.15);
	  sleep_ipc(0.1);
	  carmen_imp_gui_publish_arrow_message(CARMEN_IMP_GUI_ARROW_STRAIGHT, 0.1);
	  sleep_ipc(0.1);
	  carmen_imp_gui_publish_arrow_message(CARMEN_IMP_GUI_ARROW_STRAIGHT, 0.05);
	  sleep_ipc(0.1);
	  carmen_imp_gui_publish_arrow_message(CARMEN_IMP_GUI_ARROW_STRAIGHT, 0.0);
	  sleep_ipc(0.5);
	  carmen_imp_gui_publish_arrow_message(CARMEN_IMP_GUI_ARROW_LEFT, 0.0);
	  sleep_ipc(1.0);
	  carmen_imp_gui_publish_arrow_message(CARMEN_IMP_GUI_ARROW_RIGHT, 0.0);
	  sleep_ipc(1.0);
	}
	else if (!strcmp(token, "PAUSE")) {
	  mark = strstr(line, token) + strlen(token);
	  if (sscanf(mark, "%d", &n) == 1) {
	    printf("pausing for %d seconds\n", n);
	    sleep_ipc(n);
	  }
	  else
	    printf("pausing...sscanf error!\n");
	}
	else if (!strcmp(token, "WAIT")) {
	  waiting_for_answer = 1;
	  while (waiting_for_answer)
	    sleep_ipc(0.01);
	  //printf("press return to continue: ");
	  //fflush(0);
	  //getchar();
	}
	else if (!strcmp(token, "IFNO")) {
	  if (answer)
	    ignore = 1;
	  question_block = 1;
	}
	else if (!strcmp(token, "IFYES")) {
	  if (!answer)
	    ignore = 1;
	  question_block = 1;
	}
	else
	  carmen_warn("Warning: Unknown command: %s\n", token);
      }
    }
    fgets(line, 2000, fp);
  }

  fclose(fp);
}

int main(int argc, char **argv) {

  char *filename;

  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);

  if (argc > 1) {
    if (!strcmp(argv[1], "--daemon") || !strcmp(argv[1], "-d")) {
      daemon(0,0);
      if (argc > 2) {
	filename = argv[2];
	if (strchr(filename, '1'))
	  r03_session_num = 1;
	else if (strchr(filename, '2'))
	  r03_session_num = 2;
      }
      else
	filename = carmen_new_string("imp_intro.in");
    }
    else {
      filename = argv[1];
      if (strchr(filename, '1'))
	r03_session_num = 1;
      else if (strchr(filename, '2'))
	r03_session_num = 2;
    }
  }
  else
    filename = carmen_new_string("imp_intro.in");

  if (!carmen_file_exists(filename))
    carmen_die("Could not find file %s\n", filename);  

  fp = fopen(filename, "r");
  
  ipc_init();
  //signal(SIGCHLD, sigchld_handler);
  signal(SIGINT, shutdown_module); 
  
  theta_init(NULL);
  
  do_intro();

  //IPC_dispatch();
  
  return 0;
}
