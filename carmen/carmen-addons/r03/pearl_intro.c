#include <carmen/carmen.h>
#include <sys/wait.h>
#include <sys/types.h>
//#include <theta.h>
#include "neck_interface.h"


static FILE *fp;
int answer;


static void ipc_init() {

  return;
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

void sigchld_handler() {

  int status;

  waitpid(-1, &status, WNOHANG);
}

#if 0
static void fork_speech(char *s) {

  static cst_voice *vox;
  static theta_voice_desc *vlist;
  //static theta_async_t theta_proc;
  //static int theta_proc_used = 0;
  static int first = 1;
  //int cnt = 0;

  if (first) {
    vlist = theta_enum_voices(NULL, NULL);
    first = 0;
  }

  //while (1) {
    vox = theta_load_voice(vlist);
    while (vox == NULL) {
      fprintf(stderr, "ERROR: vox == NULL!\n");
      vox = theta_load_voice(vlist);
      usleep(100000);
    }

    //if (theta_proc_used)
    //  theta_close_async(theta_proc);
    //theta_proc = theta_tts_async(s, vox);
    //theta_proc_used = 1;
    //while (theta_query_async(theta_proc) && cnt < 100) {
    //usleep(100000);
    //cnt++;
    //}
    //theta_close_async(theta_proc);
    //theta_unload_voice(vox);
    //if (cnt < 100)
    //break;
    //}

  theta_tts(s, vox);
  theta_unload_voice(vox);

  /*
  int speech_pid;
  
  speech_pid = fork();
  if (speech_pid == 0)
    {
      execlp("/opt/theta/bin/theta", "theta", s, NULL);
      carmen_die_syserror("Could not exec theta");
    }
  if (speech_pid == -1)
    carmen_die_syserror("Could not fork to start theta");
  */
}
#endif

void do_intro() {

  static int ignore = 0;
  static char line[2000], token[100];
  static char say_text[500], print_text[500];
  char *mark, *pos;
  int n = 0;
  int c;

  fgets(line, 2000, fp);
  while (!feof(fp)) {
    if (sscanf(line, "%s", token) == 1) {
      if (!strcmp(token, "ENDIF")) {
	ignore = 0;
      }
      else if (!ignore) {
	if (!strcmp(token, "SAY")) {
	  mark = strstr(line, token) + strlen(token);
	  mark += strspn(mark, " \t");
	  //fork_speech(mark);
	}
	else if (!strcmp(token, "PRINT")) {
	  mark = strstr(line, token) + strlen(token);
	  mark += strspn(mark, " \t");
	  //carmen_imp_gui_publish_text_message(format_gui_text(carmen_new_string(mark)));
	  printf("%s", mark);
	  fflush(0);
	}
	else if (!strcmp(token, "HEAD")) {
	  mark = strstr(line, token) + strlen(token);
	  mark += strspn(mark, " \t");
	  carmen_neck_command(carmen_new_string(mark));
	  //printf("%s", mark);
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
	  //carmen_imp_gui_publish_question_message(format_gui_text(carmen_new_string(print_text)));
	  printf("%s", print_text);
	  fflush(0);
	  //if (strlen(say_text) > 0)
	  //  fork_speech(say_text);
	  //waiting_for_answer = 1;
	  //while (waiting_for_answer)
	  //  sleep_ipc(0.01);
	  c = '0';
	  while (toupper(c) != 'Y' && toupper(c) != 'N') {
	    fprintf(stderr, "Enter 'y' or 'n' to continue: ");
	    fflush(stderr);
	    c = getchar();
	  }
	  if (toupper(c) == 'Y')
	    answer = 1;
	  else
	    answer = 0;
	}
	else if (!strcmp(token, "PAUSE")) {
	  mark = strstr(line, token) + strlen(token);
	  if (sscanf(mark, "%d", &n) == 1) {
	    fprintf(stderr, "pausing for %d seconds\n", n);
	    sleep_ipc(n);
	  }
	  else
	    fprintf(stderr, "pausing...sscanf error!\n");
	}
	else if (!strcmp(token, "WAIT")) {
	  //carmen_imp_gui_publish_question_message(carmen_new_string(print_text)));
	  fprintf(stderr, "press return to continue: ");
	  fflush(stderr);
	  getchar();
	}
	else if (!strcmp(token, "IFNO")) {
	  if (answer)
	    ignore = 1;
	}
	else if (!strcmp(token, "IFYES")) {
	  if (!answer)
	    ignore = 1;
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

  if (argc > 1)
    filename = argv[1];
  else
    filename = carmen_new_string("imp_intro.in");

  if (!carmen_file_exists(filename))
    carmen_die("Could not find file %s\n", filename);  

  fp = fopen(filename, "r");
  
  ipc_init();
  signal(SIGCHLD, sigchld_handler);
  signal(SIGINT, shutdown_module); 
  
  //theta_init(NULL);
  
  do_intro();

  //IPC_dispatch();
  
  return 0;
}
