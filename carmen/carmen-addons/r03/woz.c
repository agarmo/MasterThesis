#include <carmen/carmen_graphics.h>
#include <carmen/roomnav_guidance_messages.h>
#include <carmen/roomnav_guidance.h>
#include <carmen/walker_interface.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <theta.h>

static carmen_roomnav_guidance_msg guidance_msg;
static int audio = 1;


// assumes guidance_msg has already been filled in
static void publish_guidance_msg() {

  static int first = 1;
  IPC_RETURN_TYPE err;
  
  if (first) {
    strcpy(guidance_msg.host, carmen_get_tenchar_host_name());
    first = 0;
  }

  guidance_msg.timestamp = carmen_get_time_ms();

  err = IPC_publishData(CARMEN_ROOMNAV_GUIDANCE_MSG_NAME, &guidance_msg);
  carmen_test_ipc_exit(err, "Could not publish", CARMEN_ROOMNAV_GUIDANCE_MSG_NAME);  
}

void shutdown_module(int sig) {

  if (sig == SIGINT) {
    fprintf(stderr, "\nDisconnecting.\n");
    close_ipc();
    exit(0);
  }
}

void sigchld_handler() {

  int status;

  waitpid(-1, &status, WNOHANG);
}

static void fork_speech(char *s, int command, int alt, int please) {

  static cst_voice *vox;
  static theta_voice_desc *vlist;
  static int first = 1;
  char wav_filename[100];
  int speech_pid;
  int status;
  
  //s = NULL;  //dbug

  
  if (alt) {
    if (please)
      sprintf(wav_filename, "/home/jrod/wavs/GUIDANCE%dpa.wav", command);
    else
      sprintf(wav_filename, "/home/jrod/wavs/GUIDANCE%da.wav", command);
  }
  else {
    if (please)
      sprintf(wav_filename, "/home/jrod/wavs/GUIDANCE%dp.wav", command);
    else
      sprintf(wav_filename, "/home/jrod/wavs/GUIDANCE%d.wav", command);
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

static void button_press(GtkWidget *widget __attribute__ ((unused)), 
			 gpointer data) {

  char pls[10];
  char pls2[10];
  char guidance[200];
  int dir = (int) data;
  int please, altdir;

  please = altdir = 0;

  guidance_msg.cmd = dir;

  if (guidance_msg.text)
    free(guidance_msg.text);
  guidance_msg.text = NULL;
  
  guidance_msg.cmd = dir;

  if(rand() % 2) {
    please = 1;
    strcpy(pls, "Please ");
    strcpy(pls2, "Please\n");
  } else {
    strcpy(pls, "");
    strcpy(pls2, "");
  }

  if(dir == CARMEN_ROOMNAV_GUIDANCE_FORWARD) {
    if(rand() % 2) {
      sprintf(guidance, "\"%sGO FORWARD\"", pls);
      guidance_msg.text = carmen_new_string("%sgo forward.", pls2);
    } else {
      altdir = 1;
      sprintf(guidance, "\"%sGO STRAIGHT\"", pls);
      guidance_msg.text = carmen_new_string("%sgo straight.", pls2);	
    }
  } else if(dir == CARMEN_ROOMNAV_GUIDANCE_LEFT) {
    sprintf(guidance, "\"%sTURN LEFT\"", pls);
    guidance_msg.text = carmen_new_string("%sturn left.", pls2);
  } else if(dir == CARMEN_ROOMNAV_GUIDANCE_BACKWARD) {
    sprintf(guidance, "\"%sTURN AROUND\"", pls);
    guidance_msg.text = carmen_new_string("%sturn around.", pls2);
  } else if(dir == CARMEN_ROOMNAV_GUIDANCE_RIGHT) {
    sprintf(guidance, "\"%sTURN RIGHT\"", pls);
    guidance_msg.text = carmen_new_string("%sturn right.", pls2);
  } else if(dir == CARMEN_ROOMNAV_GUIDANCE_GO_THROUGH_DOORWAY) {
    sprintf(guidance, "\"%sGO THROUGH THE DOORWAY\"", pls);
    guidance_msg.text = carmen_new_string("%sgo through\n the doorway.", pls2);
  } else if(dir == CARMEN_ROOMNAV_GUIDANCE_ENTER_HALLWAY) {
    sprintf(guidance, "\"%sENTER THE HALLWAY\"", pls);
    guidance_msg.text = carmen_new_string("%senter\n the\n hallway.", pls2);
  } else if(dir == CARMEN_ROOMNAV_GUIDANCE_ENTER_ROOM_IN_FRONT) {
    sprintf(guidance, "\"%sENTER THE ROOM IN FRONT OF YOU\"", pls);
    guidance_msg.text = carmen_new_string("%senter room\n in front\n of you.", pls2);
  } else if(dir == CARMEN_ROOMNAV_GUIDANCE_LEFT_CORNER) {
    sprintf(guidance, "\"%sTURN LEFT AT THE CORNER\"", pls);
    guidance_msg.text = carmen_new_string("%sturn left\n at the\n corner.", pls2);
  } else if (dir == CARMEN_ROOMNAV_GUIDANCE_RIGHT_CORNER) {
    sprintf(guidance, "\"%sTURN RIGHT AT THE CORNER\"", pls);
    guidance_msg.text = carmen_new_string("%sturn right\n at the\n corner.", pls2);
  } else if(dir == CARMEN_ROOMNAV_GUIDANCE_GO_TO_END_OF_HALLWAY) {
    sprintf(guidance, "\"%sGO TO THE END OF THE HALLWAY\"", pls);
    guidance_msg.text = carmen_new_string("%sgo to the\n end of\n the hallway.", pls2);
  } else if(dir == CARMEN_ROOMNAV_GUIDANCE_TURN_LEFT_AT_INTERSECTION) {
    sprintf(guidance, "\"%sTURN LEFT AT THE INTERSECTION\"", pls);
    guidance_msg.text = carmen_new_string("%sturn left\n at the\n intersection.", pls2);
  } else if(dir == CARMEN_ROOMNAV_GUIDANCE_TURN_RIGHT_AT_INTERSECTION) {
    sprintf(guidance, "\"%sTURN RIGHT AT THE INTERSECTION\"", pls);
    guidance_msg.text = carmen_new_string("%sturn right\n at the\n intersection.", pls2);
  } else if(dir == CARMEN_ROOMNAV_GUIDANCE_TURN_NEXT_LEFT) {
    if(rand() % 2) {
      sprintf(guidance, "\"%sTURN AT THE NEXT LEFT\"", pls);
      guidance_msg.text = carmen_new_string("%sturn\n at the\n next left.", pls2);
    } else {
      altdir = 1;
      sprintf(guidance, "\"%sTAKE THE NEXT LEFT\"", pls);
      guidance_msg.text = carmen_new_string("%stake\n the\n next left.", pls2);
    }
  } else if(dir == CARMEN_ROOMNAV_GUIDANCE_TURN_NEXT_RIGHT) {
    if(rand() % 2) {
      sprintf(guidance, "\"%sTURN AT THE NEXT RIGHT\"", pls);
      guidance_msg.text = carmen_new_string("%sturn\n at the\n next right.", pls2);
    } else {
      altdir = 1;
      sprintf(guidance, "\"%sTAKE THE NEXT RIGHT\"", pls);
      guidance_msg.text = carmen_new_string("%stake\n the\n next right.", pls2);
    }
  } else if(dir == CARMEN_ROOMNAV_GUIDANCE_TURN_RIGHT_AT_DOORWAY) {
    sprintf(guidance, "\"%sTURN RIGHT AT THE DOORWAY\"", pls);
    guidance_msg.text = carmen_new_string("%sturn right\n at the\n doorway.", pls2);
  } else if(dir == CARMEN_ROOMNAV_GUIDANCE_TURN_LEFT_AT_DOORWAY) {
    sprintf(guidance, "\"%sTURN LEFT AT THE DOORWAY\"", pls);
    guidance_msg.text = carmen_new_string("%sturn left\n at the\n doorway.", pls2);
  } else if(dir == CARMEN_ROOMNAV_GUIDANCE_ENTER_HALLWAY_TURN_RIGHT) {
    sprintf(guidance, "\"%sENTER THE HALLWAY AND TURN RIGHT\"", pls);
    guidance_msg.text = carmen_new_string("%senter hallway\n and\n turn right.", pls2);
  } else if(dir == CARMEN_ROOMNAV_GUIDANCE_ENTER_HALLWAY_TURN_LEFT) {
    sprintf(guidance, "\"%sENTER THE HALLWAY AND TURN LEFT\"", pls);
    guidance_msg.text = carmen_new_string("%senter hallway\n and\n turn left.", pls2);
  } else if(dir == CARMEN_ROOMNAV_GUIDANCE_CONTINUE_DOWN_HALLWAY) {
    sprintf(guidance, "\"%sCONTINUE DOWN THE HALLWAY\"", pls);
    guidance_msg.text = carmen_new_string("%scontinue\n down \nthe hallway.", pls2);
  } else if(dir == CARMEN_ROOMNAV_GUIDANCE_TAKE_LEFT_HALLWAY) {
    sprintf(guidance, "\"%sTAKE THE LEFT HALLWAY\"", pls);
    guidance_msg.text = carmen_new_string("%stake the\n hallway on\n your left.", pls2);      
  } else if(dir == CARMEN_ROOMNAV_GUIDANCE_TAKE_RIGHT_HALLWAY) {
    sprintf(guidance, "\"%sTAKE THE RIGHT HALLWAY\"", pls);
    guidance_msg.text = carmen_new_string("%stake the\n hallway on\n your right.", pls2);
  } else if(dir == CARMEN_ROOMNAV_GUIDANCE_KEEP_GOING) {
    sprintf(guidance, "\"KEEP GOING\"");
    guidance_msg.text = carmen_new_string("Keep\n going.");
  } else if(dir == CARMEN_ROOMNAV_GUIDANCE_DESTINATION_REACHED) {
    sprintf(guidance, "\"YOU HAVE ARRIVED\"");
    guidance_msg.text = carmen_new_string("You\n have\n arrived");
  }

  if (strlen(pls) == 0) {
    guidance[0] = toupper(guidance[0]);
    guidance_msg.text[0] = toupper(guidance_msg.text[0]);
  }

  printf("%s\n", guidance);
  
  if(audio)
    fork_speech(guidance, dir, altdir, please);

  publish_guidance_msg();

  fflush(0);
}

static void main_window_destroy(GtkWidget *widget __attribute__ ((unused)),
				gpointer data __attribute__ ((unused))) {

  gtk_main_quit();
}

static void graphics_init() {

  GtkWidget *main_window, *vbox;
  GtkWidget *hbox1, *hbox2, *hbox3, *hbox4, *hbox5, *hbox6;  //, *hbox7;

  GtkWidget *forward_button, *left_button, *right_button, *turn_around_button;
  GtkWidget *keep_going_button, *destination_reached_button;

  GtkWidget *enter_hallway_button, *enter_room_button, *through_doorway_button;
  GtkWidget *enter_hallway_turn_left_button, *enter_hallway_turn_right_button;
  GtkWidget *end_of_hallway_button, *continue_down_hallway_button;

  GtkWidget *left_corner_button, *right_corner_button;
  GtkWidget *left_intersection_button, *right_intersection_button;

  GtkWidget *next_left_button, *next_right_button;
  GtkWidget *left_at_doorway_button, *right_at_doorway_button;
  GtkWidget *take_left_hallway_button, *take_right_hallway_button;
  //GtkWidget *enter_left_room_button, *enter_right_room_button;

  main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_policy(GTK_WINDOW(main_window), FALSE, TRUE, FALSE);
  
  gtk_signal_connect(GTK_OBJECT(main_window), "destroy",
		     GTK_SIGNAL_FUNC(main_window_destroy), NULL);

  vbox = gtk_vbox_new(FALSE, 5);
  hbox1 = gtk_hbox_new(FALSE, 0);
  hbox2 = gtk_hbox_new(FALSE, 0);
  hbox3 = gtk_hbox_new(FALSE, 0);
  hbox4 = gtk_hbox_new(FALSE, 0);
  hbox5 = gtk_hbox_new(FALSE, 0);
  hbox6 = gtk_hbox_new(FALSE, 0);
  //hbox7 = gtk_hbox_new(FALSE, 0);

  forward_button = gtk_button_new_with_label ("\n    Forward    \n");
  gtk_signal_connect (GTK_OBJECT(forward_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_FORWARD);
  gtk_box_pack_start(GTK_BOX(hbox1), forward_button, FALSE, FALSE, 5);

  left_button = gtk_button_new_with_label ("\n    Left    \n");
  gtk_signal_connect (GTK_OBJECT(left_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_LEFT);
  gtk_box_pack_start(GTK_BOX(hbox1), left_button, FALSE, FALSE, 5);

  right_button = gtk_button_new_with_label ("\n    Right    \n");
  gtk_signal_connect (GTK_OBJECT(right_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_RIGHT);
  gtk_box_pack_start(GTK_BOX(hbox1), right_button, FALSE, FALSE, 5);

  turn_around_button = gtk_button_new_with_label ("\n    Turn around    \n");
  gtk_signal_connect (GTK_OBJECT(turn_around_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_BACKWARD);
  gtk_box_pack_start(GTK_BOX(hbox1), turn_around_button, FALSE, FALSE, 5);

  keep_going_button = gtk_button_new_with_label ("\n    Keep going    \n");
  gtk_signal_connect (GTK_OBJECT(keep_going_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_KEEP_GOING);
  gtk_box_pack_start(GTK_BOX(hbox1), keep_going_button, FALSE, FALSE, 5);

  destination_reached_button = gtk_button_new_with_label ("\n    Destination reached    \n");
  gtk_signal_connect (GTK_OBJECT(destination_reached_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_DESTINATION_REACHED);
  gtk_box_pack_start(GTK_BOX(hbox1), destination_reached_button, FALSE, FALSE, 5);


  enter_hallway_button = gtk_button_new_with_label ("\n    Enter hallway    \n");
  gtk_signal_connect (GTK_OBJECT(enter_hallway_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_ENTER_HALLWAY);
  gtk_box_pack_start(GTK_BOX(hbox2), enter_hallway_button, FALSE, FALSE, 5);

  enter_hallway_turn_left_button = gtk_button_new_with_label ("\n    Enter hallway & turn left    \n");
  gtk_signal_connect (GTK_OBJECT(enter_hallway_turn_left_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_ENTER_HALLWAY_TURN_LEFT);
  gtk_box_pack_start(GTK_BOX(hbox2), enter_hallway_turn_left_button, FALSE, FALSE, 5);

  enter_hallway_turn_right_button = gtk_button_new_with_label ("\n    Enter hallway & turn right    \n");
  gtk_signal_connect (GTK_OBJECT(enter_hallway_turn_right_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_ENTER_HALLWAY_TURN_RIGHT);
  gtk_box_pack_start(GTK_BOX(hbox2), enter_hallway_turn_right_button, FALSE, FALSE, 5);

  end_of_hallway_button = gtk_button_new_with_label ("\n    Go to end of hallway    \n");
  gtk_signal_connect (GTK_OBJECT(end_of_hallway_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_GO_TO_END_OF_HALLWAY);
  gtk_box_pack_start(GTK_BOX(hbox3), end_of_hallway_button, FALSE, FALSE, 5);

  continue_down_hallway_button = gtk_button_new_with_label ("\n    Continue down hallway    \n");
  gtk_signal_connect (GTK_OBJECT(continue_down_hallway_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_CONTINUE_DOWN_HALLWAY);
  gtk_box_pack_start(GTK_BOX(hbox3), continue_down_hallway_button, FALSE, FALSE, 5);

  take_left_hallway_button = gtk_button_new_with_label ("\n    Take left hallway    \n");
  gtk_signal_connect (GTK_OBJECT(take_left_hallway_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_TAKE_LEFT_HALLWAY);
  gtk_box_pack_start(GTK_BOX(hbox4), take_left_hallway_button, FALSE, FALSE, 5);

  take_right_hallway_button = gtk_button_new_with_label ("\n    Take right hallway    \n");
  gtk_signal_connect (GTK_OBJECT(take_right_hallway_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_TAKE_RIGHT_HALLWAY);
  gtk_box_pack_start(GTK_BOX(hbox4), take_right_hallway_button, FALSE, FALSE, 5);


  enter_room_button = gtk_button_new_with_label ("\n    Enter room    \n");
  gtk_signal_connect (GTK_OBJECT(enter_room_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_ENTER_ROOM_IN_FRONT);
  gtk_box_pack_start(GTK_BOX(hbox5), enter_room_button, FALSE, FALSE, 5);

  /*
  enter_left_room_button = gtk_button_new_with_label ("\n    Enter left room    \n");
  gtk_signal_connect (GTK_OBJECT(enter_left_room_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_ENTER_LEFT_ROOM);
  gtk_box_pack_start(GTK_BOX(hbox5), enter_left_room_button, FALSE, FALSE, 5);

  enter_right_room_button = gtk_button_new_with_label ("\n    Enter right room    \n");
  gtk_signal_connect (GTK_OBJECT(enter_right_room_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_ENTER_RIGHT_ROOM);
  gtk_box_pack_start(GTK_BOX(hbox5), enter_right_room_button, FALSE, FALSE, 5);
  */

  through_doorway_button = gtk_button_new_with_label ("\n    Through doorway    \n");
  gtk_signal_connect (GTK_OBJECT(through_doorway_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_GO_THROUGH_DOORWAY);
  gtk_box_pack_start(GTK_BOX(hbox5), through_doorway_button, FALSE, FALSE, 5);

  left_at_doorway_button = gtk_button_new_with_label ("\n    Left at doorway    \n");
  gtk_signal_connect (GTK_OBJECT(left_at_doorway_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_TURN_LEFT_AT_DOORWAY);
  gtk_box_pack_start(GTK_BOX(hbox5), left_at_doorway_button, FALSE, FALSE, 5);

  right_at_doorway_button = gtk_button_new_with_label ("\n    Right at doorway    \n");
  gtk_signal_connect (GTK_OBJECT(right_at_doorway_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_TURN_RIGHT_AT_DOORWAY);
  gtk_box_pack_start(GTK_BOX(hbox5), right_at_doorway_button, FALSE, FALSE, 5);


  next_left_button = gtk_button_new_with_label ("\n    Next left    \n");
  gtk_signal_connect (GTK_OBJECT(next_left_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_TURN_NEXT_LEFT);
  gtk_box_pack_start(GTK_BOX(hbox6), next_left_button, FALSE, FALSE, 5);

  next_right_button = gtk_button_new_with_label ("\n    Next right    \n");
  gtk_signal_connect (GTK_OBJECT(next_right_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_TURN_NEXT_RIGHT);
  gtk_box_pack_start(GTK_BOX(hbox6), next_right_button, FALSE, FALSE, 5);

  left_corner_button = gtk_button_new_with_label ("\n    Left corner    \n");
  gtk_signal_connect (GTK_OBJECT(left_corner_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_LEFT_CORNER);
  gtk_box_pack_start(GTK_BOX(hbox6), left_corner_button, FALSE, FALSE, 5);

  right_corner_button = gtk_button_new_with_label ("\n    Right corner    \n");
  gtk_signal_connect (GTK_OBJECT(right_corner_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_RIGHT_CORNER);
  gtk_box_pack_start(GTK_BOX(hbox6), right_corner_button, FALSE, FALSE, 5);

  left_intersection_button = gtk_button_new_with_label ("\n    Left intersection    \n");
  gtk_signal_connect (GTK_OBJECT(left_intersection_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_TURN_LEFT_AT_INTERSECTION);
  gtk_box_pack_start(GTK_BOX(hbox6), left_intersection_button, FALSE, FALSE, 5);

  right_intersection_button = gtk_button_new_with_label ("\n    Right intersection    \n");
  gtk_signal_connect (GTK_OBJECT(right_intersection_button), "clicked",
		      GTK_SIGNAL_FUNC(button_press), (gpointer)CARMEN_ROOMNAV_GUIDANCE_TURN_RIGHT_AT_INTERSECTION);
  gtk_box_pack_start(GTK_BOX(hbox6), right_intersection_button, FALSE, FALSE, 5);


  gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox3, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox4, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox5, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox6, FALSE, FALSE, 0);
  //gtk_box_pack_start(GTK_BOX(vbox), hbox7, FALSE, FALSE, 0);

  gtk_container_add(GTK_CONTAINER(main_window), vbox);

  gtk_widget_show_all(main_window);
}

static void ipc_init() {

  IPC_RETURN_TYPE err;

  guidance_msg.text = NULL;
  
  err = IPC_defineMsg(CARMEN_ROOMNAV_GUIDANCE_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_ROOMNAV_GUIDANCE_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_ROOMNAV_GUIDANCE_MSG_NAME);
}

static gint updateIPC(gpointer *data __attribute__ ((unused))) {

  sleep_ipc(0.01);
  carmen_graphics_update_ipc_callbacks((GdkInputFunction) updateIPC);

  return 1;
}

int main(int argc, char *argv[]) {

  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);

  carmen_randomize();

  gtk_init(&argc, &argv);

  if (argc > 1 && !strcmp(argv[1], "-notheta"))
    audio = 0;
  else
    audio = 1;

  theta_init(NULL);
  graphics_init();
  ipc_init();

  signal(SIGCHLD, sigchld_handler);
  signal(SIGINT, shutdown_module); 

  carmen_graphics_update_ipc_callbacks((GdkInputFunction) updateIPC);
  gtk_main();

  return 0;
}
