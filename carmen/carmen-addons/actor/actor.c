#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <carmen/carmen.h>

carmen_navigator_autonomous_stopped_message sm;

static void 
actor_shutdown(int signo __attribute__ ((unused)) )
{
  static int done = 0;
  
  if(!done) {
    close_ipc();
    printf("Disconnected from IPC.\n");

    done = 1;
  }
  exit(0);
}

/* A static variable for holding the line. */
static char *line_read = (char *)NULL;

/* this code coppied strait out of the readline info page*/
char *
rl_gets ()
{
  /* If the buffer has already been allocated, return the memory
     to the free pool. */
  if (line_read)
    {
      free (line_read);
           line_read = (char *)NULL;
    }
  
  /* Get a line from the user. */
  line_read = readline (">");
     
  /* If the line has any text in it, save it on the history. */
  if (line_read && *line_read)
    add_history (line_read);
  
  return (line_read);
}

static void
handler(){
  int done = 0;
  
  while( !done ){
    char* command = rl_gets();

    if( command == NULL )
      actor_shutdown(0);
    
    if( strncmp("pause",command,strlen("pause")) == 0 ){
      double seconds;
      if( sscanf(&command[strlen("pause")],"%lf",&seconds) == 1 ){
	IPC_listenWait((int)(seconds*1000.0));
      }
    }else{
      double x,y;
      if( sscanf(command,"%lf%lf",&x,&y) == 2 ){
	carmen_navigator_set_goal(x,y);
	carmen_navigator_go();
	done = 1;
      }else{
	actor_shutdown(0);
      }
    }
  }
}


int
main(int argc __attribute__ ((unused)), 
     char **argv ){

  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);
  
  signal(SIGINT,actor_shutdown);
  
  carmen_navigator_subscribe_autonomous_stopped_message
    (&sm,handler,CARMEN_SUBSCRIBE_ALL);
  
  //start the first command
  handler();

  IPC_dispatch();
  
  return 0;
}
							
  

