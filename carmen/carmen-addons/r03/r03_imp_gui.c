#include <carmen/carmen_graphics.h>
#include <carmen/roomnav_guidance_messages.h>
#include <carmen/roomnav_guidance.h>
#include <carmen/sound_server_interface.h>
#include "imp_gui_interface.h"
#include "r03_imp_server_interface.h"


static carmen_roomnav_guidance_msg guidance_msg;
static int user_id = 1;
static int walk_num = 1;
static int audio = 1;
static int text = 1;
static int arrow = 1;
static int woz = 1;

#define DEFAULT_VOLUME 80


#define WOZ_NUM_GUIDANCE 25
static const int woz_bbox[WOZ_NUM_GUIDANCE][4] = {{250, 385, 300, 450},  // forward
						  {190, 450, 255, 500},  // left
						  {250, 500, 300, 570},  // back
						  {295, 450, 360, 500},  // right
						  {28, 145, 95, 290},  // through doorway
						  {225, 20, 322, 135},  // enter hall
						  {430, 145, 555, 286},  // enter room
						  {24, 325, 122, 421},  // left corner
						  {429, 325, 525, 421},  // right corner
						  {144, 129, 212, 226},  // end of hall
						  {24, 460, 150, 555},  // left intersection
						  {400, 460, 525, 555},  // right intersection
						  {158, 355, 225, 420},  // next left
						  {322, 356, 390, 418},  // next right
						  {-1, -1, -1, -1},  // right at door
						  {-1, -1, -1, -1},  // left at door
						  {400, 10, 526, 90},  // enter hall turn right
						  {40, 10, 165, 90},  // enter hall turn left
						  {250, 295, 300, 360},  // keep going
						  {339, 131, 405, 225},  // continue down hall
						  {-1, -1, -1, -1},  // take left hall
						  {-1, -1, -1, -1},  // take right hall
						  {-1, -1, -1, -1},  // enter left room
						  {-1, -1, -1, -1},  // enter right room
						  {232, 175, 315, 255}};  // destination reached


static int woz_get_guidance(int x, int y) {

  int i;

  for (i = 0; i < WOZ_NUM_GUIDANCE; i++)
    if (x >= woz_bbox[i][0] && x <= woz_bbox[i][2] && y >= woz_bbox[i][1] && y <= woz_bbox[i][3])
      return i;

  return -1;
}

// assumes guidance_msg has already been filled in
static void publish_guidance_msg() {

  static int first = 1;
  IPC_RETURN_TYPE err;
  
  if (first) {
    strcpy(guidance_msg.host, carmen_get_tenchar_host_name());
    first = 0;
  }

  guidance_msg.timestamp = carmen_get_time_ms();

  err = IPC_defineMsg(CARMEN_ROOMNAV_GUIDANCE_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_ROOMNAV_GUIDANCE_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_ROOMNAV_GUIDANCE_MSG_NAME);

  err = IPC_publishData(CARMEN_ROOMNAV_GUIDANCE_MSG_NAME, &guidance_msg);
  carmen_test_ipc_exit(err, "Could not publish", CARMEN_ROOMNAV_GUIDANCE_MSG_NAME);  
}

static void publish_audio_msg(char *s __attribute__ ((unused)), int command, int alt, int please) {

  char wav_filename[100];
  
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

  carmen_sound_play_file(carmen_new_string(wav_filename));
}

static void woz_button_press_handler(GtkWidget *w __attribute__ ((unused)),
				     GdkEventMotion *event) {

  int x, y, dir, please, altdir;
  char pls[10];
  char pls2[10];
  char guidance[200];

  x = (int)event->x;
  y = (int)event->y;

  dir = woz_get_guidance(x,y);

  if (dir < 0)
    return;

  //printf("button press: %d %d --> %d\n", x, y, dir);

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
    publish_audio_msg(guidance, dir, altdir, please);

  publish_guidance_msg();

  fflush(0);
}

static void volume_handler(GtkAdjustment *adj) {

  carmen_sound_set_volume(100 - (GTK_ADJUSTMENT(adj))->value);
}

static void play_intro1() {

  carmen_r03_imp_play_intro_file(carmen_new_string("/home/jrod/IMP1.txt"));
}

static void play_intro2() {

  carmen_r03_imp_play_intro_file(carmen_new_string("/home/jrod/IMP2.txt"));
}

static void intro_yes() {

  carmen_imp_gui_answer(1);
}

static void intro_no() {

  carmen_imp_gui_answer(0);
}

static void intro_continue() {

  carmen_imp_gui_answer(1);
}

static void session_id_handler(GtkAdjustment *adj) {

  user_id = (GTK_ADJUSTMENT(adj))->value;
}

static void session_walk_handler(GtkAdjustment *adj) {

  walk_num = (GTK_ADJUSTMENT(adj))->value;
}

static void audio_on() {

  audio = 1;
}

static void audio_off() {

  audio = 0;
}

static void text_on() {

  text = 1;
}

static void text_off() {

  text = 0;
}

static void arrow_on() {

  arrow = 1;
}

static void arrow_off() {

  arrow = 0;
}

static void woz_on() {

  woz = 1;
}

static void woz_off() {

  woz = 0;
}

static void start_session() {

  int config = 0;

  if (audio)
    config |= AUDIO_GUIDANCE;
  if (text)
    config |= TEXT_GUIDANCE;
  if (arrow)
    config |= ARROW_GUIDANCE;
  if (woz)
    config |= WOZ_GUIDANCE;

  carmen_r03_imp_start_session(user_id, walk_num, config);
}

static void finish_session() {

  carmen_r03_imp_finish_session();
}

static void window_destroy() {

  gtk_main_quit();
}

static void gui_init() {

  GtkWidget *window, *hbox, *woz_vbox, *event_box, *woz_frame, *woz_pixmap;
  GtkWidget *vsep, *control_vbox, *control_hbox1, *control_frame, *carmen_frame, *force_frame;
  GtkWidget *carmen_vbox, *carmen_label, *carmen_hbox, *carmen_proc_label, *carmen_status_label;
  GtkWidget *control_hbox2, *intro_label, *intro1_button, *intro2_button, *intro_yes_button;
  GtkWidget *intro_no_button, *intro_continue_button, *intro_vbox, *intro_hbox;
  GtkWidget *control_vsep, *session_label, *session_id_label, *session_walk_label;
  GtkWidget *session_audio_label, *session_woz_label, *session_text_label;
  GtkWidget *session_hsep, *session_origin_label, *session_destination_label;
  GtkWidget *session_config_hbox, *session_button_hbox, *session_vbox;
  GtkWidget *session_config_vbox1, *session_config_vbox2, *session_id_spin_button;
  GtkWidget *session_walk_spin_button, *session_audio_hbox, *session_text_hbox;
  GtkWidget *session_arrow_label, *session_arrow_hbox, *session_arrow_yes_button, *session_arrow_no_button;
  GtkWidget *session_woz_hbox, *session_audio_yes_button, *session_audio_no_button;
  GtkWidget *session_text_yes_button, *session_text_no_button, *session_woz_yes_button;
  GtkWidget *session_woz_no_button, *session_start_button, *session_finish_button;
  GtkWidget *volume_vbox, *volume_label, *volume_scale;
  GdkPixmap *gdk_pixmap;
  GtkObject *volume_adj, *session_walk_adj, *session_id_adj;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_policy(GTK_WINDOW(window), FALSE, TRUE, FALSE);
  gtk_signal_connect(GTK_OBJECT(window), "destroy",
		     GTK_SIGNAL_FUNC(window_destroy), NULL);
  gtk_widget_show(window);

  hbox = gtk_hbox_new(FALSE, 10);
  gtk_container_add(GTK_CONTAINER(window), hbox);
  gtk_widget_show(hbox);

  control_vbox = gtk_vbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(hbox), control_vbox, TRUE, TRUE, 10);
  gtk_widget_show(control_vbox);

  control_frame = gtk_frame_new(NULL);
  gtk_box_pack_start(GTK_BOX(control_vbox), control_frame, TRUE, TRUE, 10);  
  gtk_widget_show(control_frame);

  // introductions
  control_hbox1 = gtk_hbox_new(FALSE, 10);
  gtk_container_add(GTK_CONTAINER(control_frame), control_hbox1);
  gtk_widget_show(control_hbox1);

  intro_vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(control_hbox1), intro_vbox, TRUE, TRUE, 10);
  gtk_widget_show(intro_vbox);

  intro_label = gtk_label_new("Introductions");
  gtk_box_pack_start(GTK_BOX(intro_vbox), intro_label, FALSE, FALSE, 20);  
  gtk_widget_show(intro_label);

  intro1_button = gtk_button_new_with_label("\n           Play Intro 1           \n");
  gtk_signal_connect(GTK_OBJECT(intro1_button), "clicked",
		     GTK_SIGNAL_FUNC(play_intro1), NULL);
  gtk_box_pack_start(GTK_BOX(intro_vbox), intro1_button, FALSE, FALSE, 5);  
  gtk_widget_show(intro1_button);
  
  intro2_button = gtk_button_new_with_label("\n           Play Intro 2           \n");
  gtk_signal_connect(GTK_OBJECT(intro2_button), "clicked",
		     GTK_SIGNAL_FUNC(play_intro2), NULL);
  gtk_box_pack_start(GTK_BOX(intro_vbox), intro2_button, FALSE, FALSE, 5);  
  gtk_widget_show(intro2_button);
  
  intro_hbox = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(intro_vbox), intro_hbox, FALSE, FALSE, 10);
  gtk_widget_show(intro_hbox);

  intro_yes_button = gtk_button_new_with_label("\n    Yes    \n");
  gtk_signal_connect(GTK_OBJECT(intro_yes_button), "clicked",
		     GTK_SIGNAL_FUNC(intro_yes), NULL);
  gtk_box_pack_start(GTK_BOX(intro_hbox), intro_yes_button, TRUE, TRUE, 5);  
  gtk_widget_show(intro_yes_button);
  
  intro_no_button = gtk_button_new_with_label("\n    No    \n");
  gtk_signal_connect(GTK_OBJECT(intro_no_button), "clicked",
		     GTK_SIGNAL_FUNC(intro_no), NULL);
  gtk_box_pack_start(GTK_BOX(intro_hbox), intro_no_button, TRUE, TRUE, 5);  
  gtk_widget_show(intro_no_button);
  
  intro_continue_button = gtk_button_new_with_label("\n           Continue            \n");
  gtk_signal_connect(GTK_OBJECT(intro_continue_button), "clicked",
		     GTK_SIGNAL_FUNC(intro_continue), NULL);
  gtk_box_pack_start(GTK_BOX(intro_vbox), intro_continue_button, FALSE, FALSE, 5);  
  gtk_widget_show(intro_continue_button);
  
  control_vsep = gtk_vseparator_new();
  gtk_box_pack_start(GTK_BOX(control_hbox1), control_vsep, TRUE, TRUE, 10);
  gtk_widget_show(control_vsep);

  session_vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(control_hbox1), session_vbox, TRUE, TRUE, 10);
  gtk_widget_show(session_vbox);

  session_label = gtk_label_new(" Walking Session ");
  gtk_box_pack_start(GTK_BOX(session_vbox), session_label, FALSE, FALSE, 5);  
  gtk_widget_show(session_label);

  session_config_hbox = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(session_vbox), session_config_hbox, FALSE, FALSE, 10);
  gtk_widget_show(session_config_hbox);

  session_config_vbox1 = gtk_vbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(session_config_hbox), session_config_vbox1, FALSE, FALSE, 10);
  gtk_widget_show(session_config_vbox1);
  
  session_id_label = gtk_label_new(" User ID ");
  gtk_box_pack_start(GTK_BOX(session_config_vbox1), session_id_label, FALSE, FALSE, 8);  
  gtk_widget_show(session_id_label);

  session_walk_label = gtk_label_new(" Walk Number ");
  gtk_box_pack_start(GTK_BOX(session_config_vbox1), session_walk_label, FALSE, FALSE, 7);  
  gtk_widget_show(session_walk_label);

  session_audio_label = gtk_label_new(" Audio ");
  gtk_box_pack_start(GTK_BOX(session_config_vbox1), session_audio_label, FALSE, FALSE, 6);  
  gtk_widget_show(session_audio_label);

  session_text_label = gtk_label_new(" Text ");
  gtk_box_pack_start(GTK_BOX(session_config_vbox1), session_text_label, FALSE, FALSE, 3);  
  gtk_widget_show(session_text_label);

  session_arrow_label = gtk_label_new(" Arrow ");
  gtk_box_pack_start(GTK_BOX(session_config_vbox1), session_arrow_label, FALSE, FALSE, 5);  
  gtk_widget_show(session_arrow_label);

  session_woz_label = gtk_label_new(" Wizard of Oz ");
  gtk_box_pack_start(GTK_BOX(session_config_vbox1), session_woz_label, FALSE, FALSE, 3);  
  gtk_widget_show(session_woz_label);

  session_config_vbox2 = gtk_vbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(session_config_hbox), session_config_vbox2, FALSE, FALSE, 10);
  gtk_widget_show(session_config_vbox2);

  session_id_adj = gtk_adjustment_new(1, 0, 100, 1, 1, 0);
  gtk_signal_connect(GTK_OBJECT(session_id_adj), "value_changed",
		     GTK_SIGNAL_FUNC(session_id_handler), NULL);  
  session_id_spin_button = gtk_spin_button_new(GTK_ADJUSTMENT(session_id_adj), 1, 0);
  gtk_box_pack_start(GTK_BOX(session_config_vbox2), session_id_spin_button, FALSE, FALSE, 5);
  gtk_widget_show(session_id_spin_button);

  session_walk_adj = gtk_adjustment_new(1, 1, 8, 1, 1, 0);
  gtk_signal_connect(GTK_OBJECT(session_walk_adj), "value_changed",
		     GTK_SIGNAL_FUNC(session_walk_handler), NULL);  
  session_walk_spin_button = gtk_spin_button_new(GTK_ADJUSTMENT(session_walk_adj), 1, 0);
  gtk_box_pack_start(GTK_BOX(session_config_vbox2), session_walk_spin_button, FALSE, FALSE, 5);
  gtk_widget_show(session_walk_spin_button);

  session_audio_hbox = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(session_config_vbox2), session_audio_hbox, FALSE, FALSE, 0);
  gtk_widget_show(session_audio_hbox);

  session_audio_yes_button = gtk_radio_button_new_with_label(NULL, "on");
  gtk_signal_connect(GTK_OBJECT(session_audio_yes_button), "clicked",
		     GTK_SIGNAL_FUNC(audio_on), NULL);
  gtk_box_pack_start(GTK_BOX(session_audio_hbox), session_audio_yes_button, TRUE, TRUE, 10);
  gtk_widget_show(session_audio_yes_button);

  session_audio_no_button = gtk_radio_button_new_with_label
    (gtk_radio_button_group(GTK_RADIO_BUTTON(session_audio_yes_button)), "off");
  gtk_signal_connect(GTK_OBJECT(session_audio_no_button), "clicked",
		     GTK_SIGNAL_FUNC(audio_off), NULL);
  gtk_box_pack_start(GTK_BOX(session_audio_hbox), session_audio_no_button, TRUE, TRUE, 10);
  gtk_widget_show(session_audio_no_button);

  session_text_hbox = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(session_config_vbox2), session_text_hbox, FALSE, FALSE, 0);
  gtk_widget_show(session_text_hbox);

  session_text_yes_button = gtk_radio_button_new_with_label(NULL, "on");
  gtk_signal_connect(GTK_OBJECT(session_text_yes_button), "clicked",
		     GTK_SIGNAL_FUNC(text_on), NULL);
  gtk_box_pack_start(GTK_BOX(session_text_hbox), session_text_yes_button, TRUE, TRUE, 10);
  gtk_widget_show(session_text_yes_button);

  session_text_no_button = gtk_radio_button_new_with_label
    (gtk_radio_button_group(GTK_RADIO_BUTTON(session_text_yes_button)), "off");
  gtk_signal_connect(GTK_OBJECT(session_text_no_button), "clicked",
		     GTK_SIGNAL_FUNC(text_off), NULL);
  gtk_box_pack_start(GTK_BOX(session_text_hbox), session_text_no_button, TRUE, TRUE, 10);
  gtk_widget_show(session_text_no_button);

  session_arrow_hbox = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(session_config_vbox2), session_arrow_hbox, FALSE, FALSE, 0);
  gtk_widget_show(session_arrow_hbox);

  session_arrow_yes_button = gtk_radio_button_new_with_label(NULL, "on");
  gtk_signal_connect(GTK_OBJECT(session_arrow_yes_button), "clicked",
		     GTK_SIGNAL_FUNC(arrow_on), NULL);
  gtk_box_pack_start(GTK_BOX(session_arrow_hbox), session_arrow_yes_button, TRUE, TRUE, 10);
  gtk_widget_show(session_arrow_yes_button);

  session_arrow_no_button = gtk_radio_button_new_with_label
    (gtk_radio_button_group(GTK_RADIO_BUTTON(session_arrow_yes_button)), "off");
  gtk_signal_connect(GTK_OBJECT(session_arrow_no_button), "clicked",
		     GTK_SIGNAL_FUNC(arrow_off), NULL);
  gtk_box_pack_start(GTK_BOX(session_arrow_hbox), session_arrow_no_button, TRUE, TRUE, 10);
  gtk_widget_show(session_arrow_no_button);

  session_woz_hbox = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(session_config_vbox2), session_woz_hbox, FALSE, FALSE, 0);
  gtk_widget_show(session_woz_hbox);
  
  session_woz_yes_button = gtk_radio_button_new_with_label(NULL, "on");
  gtk_signal_connect(GTK_OBJECT(session_woz_yes_button), "clicked",
		     GTK_SIGNAL_FUNC(woz_on), NULL);
  gtk_box_pack_start(GTK_BOX(session_woz_hbox), session_woz_yes_button, TRUE, TRUE, 10);
  gtk_widget_show(session_woz_yes_button);

  session_woz_no_button = gtk_radio_button_new_with_label
    (gtk_radio_button_group(GTK_RADIO_BUTTON(session_woz_yes_button)), "off");
  gtk_signal_connect(GTK_OBJECT(session_woz_no_button), "clicked",
		     GTK_SIGNAL_FUNC(woz_off), NULL);
  gtk_box_pack_start(GTK_BOX(session_woz_hbox), session_woz_no_button, TRUE, TRUE, 10);
  gtk_widget_show(session_woz_no_button);

  session_hsep = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(session_vbox), session_hsep, TRUE, TRUE, 10);
  gtk_widget_show(session_hsep);

  session_origin_label = gtk_label_new(" Origin:   Bar Lounge ");
  gtk_box_pack_start(GTK_BOX(session_vbox), session_origin_label, FALSE, FALSE, 5);  
  gtk_widget_show(session_origin_label);

  session_destination_label = gtk_label_new(" Destination:   Woodlands Storage Room ");
  gtk_box_pack_start(GTK_BOX(session_vbox), session_destination_label, FALSE, FALSE, 5);  
  gtk_widget_show(session_destination_label);

  session_button_hbox = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(session_vbox), session_button_hbox, TRUE, TRUE, 10);
  gtk_widget_show(session_button_hbox);

  session_start_button = gtk_button_new_with_label("\n Start \n");
  gtk_signal_connect(GTK_OBJECT(session_start_button), "clicked",
		     GTK_SIGNAL_FUNC(start_session), NULL);
  gtk_box_pack_start(GTK_BOX(session_button_hbox), session_start_button, TRUE, TRUE, 5);  
  gtk_widget_show(session_start_button);
  
  session_finish_button = gtk_button_new_with_label("\n Finish \n");
  gtk_signal_connect(GTK_OBJECT(session_finish_button), "clicked",
		     GTK_SIGNAL_FUNC(finish_session), NULL);
  gtk_box_pack_start(GTK_BOX(session_button_hbox), session_finish_button, TRUE, TRUE, 5);  
  gtk_widget_show(session_finish_button);

  control_hbox2 = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(control_vbox), control_hbox2, TRUE, TRUE, 10);
  gtk_widget_show(control_hbox2);

  // carmen status
  carmen_vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(control_hbox2), carmen_vbox, TRUE, TRUE, 10);
  gtk_widget_show(carmen_vbox);

  carmen_label = gtk_label_new("Carmen Status");
  gtk_box_pack_start(GTK_BOX(carmen_vbox), carmen_label, FALSE, FALSE, 5);  
  gtk_widget_show(carmen_label);

  carmen_frame = gtk_frame_new(NULL);
  gtk_box_pack_start(GTK_BOX(carmen_vbox), carmen_frame, TRUE, TRUE, 10);
  gtk_widget_show(carmen_frame);

  carmen_hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(carmen_frame), carmen_hbox);
  gtk_widget_show(carmen_hbox);

  carmen_proc_label = gtk_label_new("central ...\n"
				"param_server ...\n"
				"walker_cerebellum ...\n"
				"walker ...\n"
				"robot ...\n"
				"roomnav ...\n"
				"roomnav_guidance ...");
  gtk_label_set_justify(GTK_LABEL(carmen_proc_label), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start(GTK_BOX(carmen_hbox), carmen_proc_label, TRUE, TRUE, 10);
  gtk_widget_show(carmen_proc_label);

  carmen_status_label = gtk_label_new("OK\nOK\nOK\nOK\nOK\nOK\nOK");
  gtk_box_pack_start(GTK_BOX(carmen_hbox), carmen_status_label, TRUE, TRUE, 10);
  gtk_widget_show(carmen_status_label);


  // force panel
  force_frame = gtk_frame_new(NULL);
  gtk_box_pack_start(GTK_BOX(control_hbox2), force_frame, TRUE, TRUE, 10);  
  gtk_widget_show(force_frame);

  // volume control
  volume_vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(control_hbox2), volume_vbox, TRUE, TRUE, 10);
  gtk_widget_show(volume_vbox);

  volume_label = gtk_label_new("Volume");
  gtk_box_pack_start(GTK_BOX(volume_vbox), volume_label, FALSE, FALSE, 5);  
  gtk_widget_show(volume_label);

  volume_adj = gtk_adjustment_new(100 - DEFAULT_VOLUME, 0, 100, 1, 10, 0);
  gtk_signal_connect(GTK_OBJECT(volume_adj), "value_changed",
		     GTK_SIGNAL_FUNC(volume_handler), NULL);
  volume_scale = gtk_vscale_new(GTK_ADJUSTMENT(volume_adj));
  gtk_scale_set_digits(GTK_SCALE(volume_scale), 0);
  gtk_scale_set_draw_value(GTK_SCALE(volume_scale), 0);
  gtk_box_pack_start(GTK_BOX(volume_vbox), volume_scale, TRUE, TRUE, 5);  
  gtk_widget_show(volume_scale);

  vsep = gtk_vseparator_new();
  gtk_box_pack_start(GTK_BOX(hbox), vsep, TRUE, TRUE, 10);
  gtk_widget_show(vsep);

  // woz panel
  woz_vbox = gtk_vbox_new(TRUE, 10);
  gtk_box_pack_start(GTK_BOX(hbox), woz_vbox, TRUE, TRUE, 10);
  gtk_widget_show(woz_vbox);

  event_box = gtk_event_box_new();
  gtk_box_pack_start(GTK_BOX(woz_vbox), event_box, TRUE, TRUE, 10);
  gtk_signal_connect(GTK_OBJECT(event_box), "button_press_event",
  		     GTK_SIGNAL_FUNC(woz_button_press_handler), NULL);
  gtk_widget_add_events(event_box, GDK_BUTTON_PRESS_MASK);
  gtk_widget_show(event_box);

  woz_frame = gtk_frame_new(NULL);
  gtk_container_add(GTK_CONTAINER(event_box), woz_frame);
  gtk_widget_show(woz_frame);

  gdk_pixmap = gdk_pixmap_create_from_xpm(window->window, NULL, NULL, "woz.xpm");
  woz_pixmap = gtk_pixmap_new(gdk_pixmap, NULL);
  gtk_container_add(GTK_CONTAINER(woz_frame), woz_pixmap);
  gtk_widget_show(woz_pixmap);
}

static void ipc_init() {

  return;
}

static gint updateIPC(gpointer *data __attribute__ ((unused))) {

  sleep_ipc(0.01);
  carmen_graphics_update_ipc_callbacks((GdkInputFunction) updateIPC);

  return 1;
}

int main(int argc, char *argv[]) {

  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);

  gtk_init(&argc, &argv);

  gui_init();
  ipc_init();

  carmen_graphics_update_ipc_callbacks((GdkInputFunction) updateIPC);

  gtk_main();

  return 0;
}
