#include <carmen/carmen.h>
#include <limits.h>

#define DEBUG 1

#include "cerebellum_com.h"


#define        CEREBELLUM_TIMEOUT          (2.0)

#define        METRES_PER_INCH        (0.0254)
#define        INCH_PER_METRE         (39.370)

#define        CEREBELLUM_LOOP_FREQUENCY  (300.0)
#define        MACH5_WHEEL_CIRCUMFERENCE  (3.5 * METRES_PER_INCH * 3.1415926535)
#define        MACH5_TICKS_REV       (11600.0)
#define        METRES_PER_CEREBELLUM (1.0/(MACH5_TICKS_REV / MACH5_WHEEL_CIRCUMFERENCE))
#define        METRES_PER_CEREBELLUM_VELOCITY (1.0/(METRES_PER_CEREBELLUM * CEREBELLUM_LOOP_FREQUENCY))

#define        WHEELBASE        (9.875 * METRES_PER_INCH)

#define        ROT_VEL_FACT_RAD (WHEELBASE*METRES_PER_CEREBELLUM_VELOCITY/2)

#define        MAXV             (1.0*METRES_PER_CEREBELLUM_VELOCITY)


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
stop_cerebellum(int signo __attribute__ ((unused))) 
{
  fprintf(stderr, "\r\nStopping robot...\r\n");
  carmen_cerebellum_set_velocity(0,0);
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

void drive(float distance)
{
  int speed;
  int l_enc, r_enc, l_vel, r_vel;
  int tick_distance = distance / METRES_PER_CEREBELLUM;
  int delta;

  carmen_cerebellum_get_state(&l_enc, &r_enc, &l_vel, &r_vel);

  delta = l_enc - tick_distance;

  speed = (int)(0.2 / METRES_PER_CEREBELLUM);

  fprintf(stderr,"At %d, Going to %d at speed %d\n",l_enc,delta, speed);

  if(tick_distance > 0)
    {
      carmen_cerebellum_set_velocity(speed, speed);

      while(delta < l_enc)
	{
	  carmen_cerebellum_get_state(&l_enc, &r_enc, &l_vel, &r_vel);
	  fprintf(stderr,"%x\n",l_enc);
	  usleep(10000);
	}
    }
  fprintf(stderr,"done\n");

  carmen_cerebellum_set_velocity(0,0);
}

int 
main(int argc, 
     char **argv) 
{
  double wheel_diameter;
  //char dev_name[100];
  double vl, vr;
  int k;
  double tv, rv;
  int a,b,c,d;
  int error;
  double voltage;
  int fault,ltemp,rtemp;
  //double start_time;


  fprintf(stderr,"starting cerebellum test\n");
  
  tv = rv = a = b = c = d = error = 0;

  signal(SIGINT, stop_cerebellum);
  signal(SIGTERM, shutdown_cerebellum);

  carmen_terminal_cbreak(0);

  wheel_diameter = 0.165;

  if(argc < 2)
    {
      
      fprintf(stderr,"usage: cerebellum_test cerebellum_serial_port_device\n");
    
      return -1;
    }
  carmen_cerebellum_connect_robot(argv[1]);

  /*
  printf("Constants: \r\nCEREBELLUM_LOOP_FREQUENCY  %f\r\nMACH5_WHEEL_CIRCUMFERENCE  %f\r\nMACH5_TICKS_REV %f\r\nMETRES_PER_CEREBELLUM %f\r\nMETRES_PER_CEREBELLUM_VELOCITY %f\r\nWHEELBASE %f\r\nROT_VEL_FACT_RAD %f\r\nMAXV %f\r\n",CEREBELLUM_LOOP_FREQUENCY,MACH5_WHEEL_CIRCUMFERENCE,MACH5_TICKS_REV,METRES_PER_CEREBELLUM,METRES_PER_CEREBELLUM_VELOCITY,WHEELBASE,ROT_VEL_FACT_RAD,MAXV);
  */
  carmen_cerebellum_ac(25);

  tv = rv = 0;


  while(1) {

    //    start_time = carmen_get_time_ms();
    //error = carmen_cerebellum_get_state(&a, &b, &c, &d);
    //fprintf(stderr, "Total delay: %f\r\n",(carmen_get_time_ms() - start_time)*1000);

    //fprintf(stderr, ".");
    
    carmen_cerebellum_get_state(&a, &b, &c, &d);

    //printf("getting char\r\n");
    k = getchar();
    //printf("got char\r\n");

    if (k != EOF) {
      switch(k) {
      case 'e': 
	error = carmen_cerebellum_get_state(&a, &b, &c, &d);

	fprintf(stderr, "Encoder readings: %8x %8x %8x %8x %d\r\n",a,b,c,d,error);

	break;
      case 'b': 
	error = carmen_cerebellum_get_voltage(&voltage);
	fprintf(stderr, "Voltage: %lf  %d\r\n",voltage,error);
	break;
      case 'a': error = carmen_cerebellum_reconnect_robot(); 
	printf("got: %d\r\n",error); 
	break;
      case 'd': carmen_cerebellum_limp(); break;
      case 'r': carmen_cerebellum_engage(); break;
      case 'q': shutdown_cerebellum(5);
      case '8': tv += 0.9; break;
      case 'i': tv += 0.1; break;
      case 'j': rv -= 3; break;
      case 'k': tv = 0.0; rv = 0; break;
      case 'l': rv += 3; break;
      case ',': tv -= 0.1; break;
      case 'f': error = carmen_cerebellum_fire(); printf("got: %d\r\n",error); break;
      case 'c':
	error = carmen_cerebellum_get_temperatures(&fault,&ltemp,&rtemp);
	fprintf(stderr, "temps: err: %d fault: %d ltemp: %d rtemp: %d\r\n",error,fault,ltemp,rtemp);
	break;
      default:  tv = 0.0; rv = 0; break;
      }

      if (tv == 0.0 && rv == 0.0) {
	carmen_cerebellum_set_velocity(0, 0);
      } else {
	vl = tv * METRES_PER_CEREBELLUM_VELOCITY;
	vr = tv * METRES_PER_CEREBELLUM_VELOCITY;
	vl -= 0.5 * rv * ROT_VEL_FACT_RAD;
	vr += 0.5 * rv * ROT_VEL_FACT_RAD;
	fprintf(stderr, "tv:%f rv:%f Velocities: %f %f\n",tv,rv,vl,vr);
	error = carmen_cerebellum_set_velocity((int)vl, (int)vr);
      }
    }
  }
  return 0;
}
