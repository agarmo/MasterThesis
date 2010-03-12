#include <carmen/carmen.h>
#include "pearlguide_interface.h"


int main(int argc, char **argv) {

  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);
  
  if (argc < 2)
    carmen_die("usage: pearlguide_set_neck_motion <\"on\"|\"off\">\n");

  if (!strcmp(argv[1], "on"))
    carmen_pearlguide_set_neck_motion(1);
  else if (!strcmp(argv[1], "off"))
    carmen_pearlguide_set_neck_motion(0);
  else
    carmen_die("usage: pearlguide_set_neck_motion <\"on\"|\"off\">\n");
  
  return 0;
}
