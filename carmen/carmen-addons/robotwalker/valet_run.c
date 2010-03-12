#include <carmen/carmen.h>
#include "valet_interface.h"
#include "irman.h"

extern int errno;

unsigned char BUTTON_1[6]={218,158,128,0,0,0};
unsigned char BUTTON_1b[6]={216,68,244,0,0,0};
unsigned char BUTTON_2[6]={220, 68, 116,0,0,0};
unsigned char BUTTON_3[6] = {218, 68, 180,0,0,0};
unsigned char BUTTON_3b[6]={21,85,85,0,0,0};
unsigned char BUTTON_4[6]={222,68,52,0,0,0};
unsigned char BUTTON_5[6]={217,68,212,0,0,0};
unsigned char BUTTON_6[6] = {221,68,84,0,0,0};
unsigned char BUTTON_7[6]={219, 68, 148,0,0,0};
unsigned char BUTTON_8[6]={223,68,20,0,0,0};
unsigned char BUTTON_9[6] = {216,196,228,0,0,0};
unsigned char BUTTON_0[6]={220,196,100,0,0,0};

unsigned char BUTTON_PLUS[6] = {221,196,68,0,0,0};
unsigned char BUTTON_MINUS[6] = {219,196,132,0,0,0};

int BUTTON_PARK = 10;
int BUTTON_RETURN = 11;


int open_serialport( char *dev )
{
  int fd;
  struct termios newtio;

  fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
  //fd = open(dev, O_WRONLY | O_NOCTTY | O_NDELAY);
  if(fd == -1) {
    fprintf(stderr, "Couldn't open serial port %s\n", dev);
    return -1;
  }
  
  memset(&newtio, 0, sizeof(newtio));
  newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR | ICRNL;
  newtio.c_oflag = 0;
  newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);  //= ICANON;
  tcflush(fd, TCIFLUSH);
  tcsetattr(fd, TCSANOW, &newtio);

  fcntl(fd, F_SETFL, FNDELAY);

  return fd;
}

void close_serialport( int fd )
{
  close(fd);
}

void init_irman( int fd )
{
  char cmd[2] = "IR";

  write(fd, cmd, 2);
}

int arrays_equal( char *b1, char *b2, int len )
{
  int i;
  for (i = 0; i < len; i++) {
    if (b1[i] != b2[i]) return 0;
  }
  return 1;
}

int get_button_number( char *buf )
{
  if(arrays_equal(buf,BUTTON_1,6))return 1;
  else if(arrays_equal(buf, BUTTON_1b,6))return 1;
  else if(arrays_equal(buf, BUTTON_2,6))return 2;
  else if (arrays_equal(buf, BUTTON_3, 6)) return 3;
  else if(arrays_equal(buf, BUTTON_3b,6))return 3;
  else if (arrays_equal(buf, BUTTON_4, 6)) return 4;
  else if (arrays_equal(buf, BUTTON_5, 6)) return 5;
  else if (arrays_equal(buf, BUTTON_6, 6)) return 6;
  else if (arrays_equal(buf, BUTTON_7, 6)) return 7;
  else if (arrays_equal(buf, BUTTON_8, 6)) return 8;
  else if (arrays_equal(buf, BUTTON_9, 6)) return 9;
  else if (arrays_equal(buf, BUTTON_0, 6)) return 0;
  else if (arrays_equal(buf, BUTTON_PLUS, 6)) return BUTTON_PARK;
  else if (arrays_equal(buf, BUTTON_MINUS, 6)) return BUTTON_RETURN;
  else return -1;
}

int filedescriptor(char *dev)
{

  int filed;
  
  filed = open_serialport(dev);

  sleep(1);

  init_irman(filed);
  return filed;
}

int getbutton(int fd) {

  int button_number = -1;
  int result;
  unsigned char buf[7];

  result = read(fd, buf, 6);

  if (result == 6) {
    printf("%d %d %d -> ", buf[0], buf[1], buf[2]);
    button_number = get_button_number(buf);
    printf("%d\n", button_number);
  }

  return button_number;
}

int main(int argc, char *argv[]) {

  int button_number;
  int fd;
  char c;

  button_number = -1;

  /*
  if (argc < 2)
    carmen_die("usage: valet_run <device>\n");
  */

  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);

  fd = (argc < 2 ? -1 : filedescriptor(argv[1]));
 
  while (1) {
    if (fd < 0) {
      c = getchar();
      if (c == 'p')
	carmen_valet_park();
      else if (c == 'r')
	carmen_valet_return();
    }
    else {
      button_number = getbutton(fd);
      if (button_number >= 0) {
	if (button_number == BUTTON_PARK)
	  carmen_valet_park();
	else if (button_number == BUTTON_RETURN)
	  carmen_valet_return();
      }
    }
  }

  if (fd >= 0)
    close_serialport(fd);

  return 0;
}
