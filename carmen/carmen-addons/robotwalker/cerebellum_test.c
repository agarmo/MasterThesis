#include <carmen/carmen.h>
#include <limits.h>

#include "cerebellum_com.h"

#define METRES_PER_CEREBELLUM .1
#define ROT_VEL_FACT_RAD 100

static double max_t_vel = 1.0;

static void 
set_wheel_velocities(double vl, double vr)
{
  if(vl > max_t_vel)
    vl = max_t_vel;
  else if(vl < -max_t_vel)
    vl = -max_t_vel;
  if(vr > max_t_vel)
    vr = max_t_vel;
  else if(vr < -max_t_vel)
    vr = -max_t_vel;

  vl /= METRES_PER_CEREBELLUM;
  vr /= METRES_PER_CEREBELLUM;

  carmen_cerebellum_set_velocity((int)vl, (int)vr);
}

void 
shutdown_cerebellum(int signo __attribute__ ((unused))) 
{
  fprintf(stderr, "\nShutting down robot...");
  sleep(1);
  carmen_cerebellum_disconnect_robot();
  fprintf(stderr, "done.\n");
  carmen_terminal_restore();
  exit(0);
}

int
main(int argc __attribute__ ((unused)), 
     char **argv __attribute__ ((unused))) 
{
  double wheel_diameter;
  char dev_name[100];
  double vl, vr;
  int k;
  double tv, rv;
  int a,b,c,d;
  int error;

  vl = vr = tv = rv = a = b = c = d = error = 0;

  signal(SIGINT, shutdown_cerebellum);
  signal(SIGTERM, shutdown_cerebellum);

  carmen_terminal_cbreak(0);

  strcpy(dev_name, "/dev/ttyUSB0");
  wheel_diameter = 0.165;

  if (carmen_cerebellum_connect_robot(dev_name) < 0) 
    return -1;

  while(1) {
    //fprintf(stderr, ".");

    k = getchar();

    if (k != EOF) {
      switch(k) {
      case 's': 
	error = carmen_cerebellum_get_state(&a, &b, &c, &d);

	fprintf(stderr, "%8d %8d %8d %8d %d\n",a,b,c,d,error);

	vl = vr = 0.0; 

	break;
      case 'c': carmen_cerebellum_engage_clutch(); vl = vr = 0.0; break;
      case 'd': carmen_cerebellum_disengage_clutch(); vl = vr = 0.0; break;
      case 'l': carmen_cerebellum_limp(); vl = vr = 0.0; break;
      case 'e': carmen_cerebellum_engage(); vl = vr = 0.0; break;
      case '1': vl = 0.0; vr = -0.1; break;
      case '2': vl = -0.1; vr = -0.1; break;
      case '3': vl = -0.1; vr = 0.0; break;
      case '4': vl = -0.1; vr = 0.1; break;
      case '5': vl = 0.0; vr = 0.0; break;
      case '6': vl = 0.1; vr = -0.1; break;
      case '7': vl = 0.0; vr = 0.1; break;
      case '8': vl = 0.1; vr = 0.1; break;
      case '9': vl = 0.1; vr = 0.0; break;
      default:  vl = 0.0; vr = 0; break;
      }

      if (k >= '1' && k <= '9') {
	vl /= METRES_PER_CEREBELLUM;
	vr /= METRES_PER_CEREBELLUM;
	/*
	  vl -= 0.5 * rv * ROT_VEL_FACT_RAD*3;
	  vr += 0.5 * rv * ROT_VEL_FACT_RAD*3;
	*/
	fprintf(stderr, " (%d %d)\n",(int)vl,(int)vr);
	carmen_cerebellum_set_velocity((int)vl, (int)vr);
      }
    }
  }
  return 0;
}
