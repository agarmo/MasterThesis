#include <qpe/qpeapplication.h>
#include <carmen/carmen.h>
#include "imp_gui.h"


QPEApplication *app;
ImpGUI *imp_gui;
char *text;
double arrow_theta;
int arrow_type;

void gui_msg_handler(carmen_imp_gui_msg *msg) {

  //printf("received message (%d): %s\n", msg->cmd, msg->text);

  if (msg->cmd == CARMEN_IMP_GUI_ARROW) {
    imp_gui->setDisplay(DISPLAY_GOING_TO);
    arrow_theta = msg->arrow_theta;
    arrow_type = msg->arrow_type;
  }
  else if (msg->cmd == CARMEN_IMP_GUI_QUESTION) {
    if (text)
      free(text);
    text = carmen_new_string(msg->text);
    printf("text = %s\n", text);
    imp_gui->setDisplay(DISPLAY_MAIN_MENU);
  }
  else if (msg->cmd == CARMEN_IMP_GUI_TEXT) {
    if (text)
      free(text);
    text = carmen_new_string(msg->text);
    printf("text = %s\n", text);
    imp_gui->setDisplay(DISPLAY_ARRIVAL);
  }
}

void init_ipc() {

  carmen_imp_gui_subscribe_gui_message(NULL, gui_msg_handler, CARMEN_SUBSCRIBE_LATEST);
}

void shutdown_ipc() {

  IPC_disconnect();
}

void update_ipc() {

  IPC_listen(0);
}

void shutdown(int sig) {

  shutdown_ipc();
  exit(sig);
  app->quit();
}

int main(int argc, char **argv) {

  app = new QPEApplication(argc, argv);  

  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);

  signal(SIGINT, shutdown);

  init_ipc();

  imp_gui = new ImpGUI();
  text = NULL;

  app->showMainWidget(imp_gui);
  imp_gui->showFullScreen();

  while (1) {
    app->processEvents();
    update_ipc();
    usleep(10000);
  }
  
  return 0;
}
