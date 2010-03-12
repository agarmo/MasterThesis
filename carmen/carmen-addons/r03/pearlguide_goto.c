#include <carmen/carmen.h>
#include "pearlguide_interface.h"


int main(int argc, char **argv) {

  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);
  
  if (argc < 2)
    carmen_die("usage: pearlguide_goto <goal_name>\n");

  carmen_pearlguide_set_goal(argv[1]);
  
  return 0;
}
