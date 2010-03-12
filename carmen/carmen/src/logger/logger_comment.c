#include <carmen/carmen.h>

int main(int argc, char** argv)
{
  if (argc != 2) {
    carmen_die("Usage: %s \"Text to be send to the logger module\"", argv[0]);
  }

  char comment[4096];
  strncpy((char*)comment, argv[1], 4095);

  carmen_ipc_initialize(argc, argv);

  carmen_logger_comment(comment);

  carmen_ipc_disconnect();
  
  return 0;
}
