/*****************************************************************************
 * PROJECT: Xavier
 *
 * (c) Copyright 1993 Richard Goodwin & Joseph O'Sullivan. All rights reserved.
 * (c) Copyright 1993 Hank Wan. All rights reserved.
 *
 * FILE: sonar_interface.h
 *
 * ABSTRACT:
 *
 * Header file to be included to use Xavier sonar functions
 * Most of the comments are from the DoubleTalk documentation,
 * adapted as felt fit. Goes with the sonar library.
 *
 *****************************************************************************/

#ifndef SONAR_INTERFACE_LOADED
#define SONAR_INTERFACE_LOADED

#include "devUtils.h"
#include "handlers.h"

#define CMPERFOOT 30.48

#define SONAR_OFFSET 7.5

#define SONAR_READING_TO_CM(sonarReading) \
   (((CMS)(sonarReading))/10.0*CMPERFOOT + robot_radius)

/************************************************************************
 *  Sonar basic constants
 ************************************************************************/

#define SONAR_READINGS_PER_SWEEP 24
#define DEGREES_PER_SONAR_READING (360/SONAR_READINGS_PER_SWEEP)
#define SONAR_READINGS_PER_SEC    4
#define SONAR_MAX_RANGE         255

/* Number of consecutive out-of-range sonar readings needed before
   concluding that the sonar bank is bad (and then stopping) */
#define BAD_SONAR_BANK_THRESHOLD  4

/* The following macros are used to convert the index into an array of
 * sonar values into an angle.  The first macro gives the angle in degrees, 
 * relative to the robot.  Zero is straight ahead and the value increases 
 * in the clockwise direction.  The second macro gives the same value, but
 * in radians and in the "map" frame of reference which is changed by 90
 * degrees and increases in the counter-clockwise direction.
 */

#define SONAR_INDEX_TO_DEG(index)\
((double)360.0*(index)/SONAR_READINGS_PER_SWEEP + SONAR_OFFSET)

#define SONAR_INDEX_TO_MAP_RAD(index)\
((double)PI/2-(double)DEG_TO_RAD(SONAR_INDEX_TO_DEG(index)))

/************************************************************************
 *  Sonar device type.
 ************************************************************************/

typedef struct {
  DEV_PTR dev;
}  SONAR_TYPE, *SONAR_PTR;

/************************************************************************
 *  Sonar sweep data type
 ************************************************************************/
typedef struct {
    unsigned short frame;
    unsigned char *data;	/* 24 bytes */
}  SONAR_DATA, *SONAR_DATA_PTR;

typedef struct sonarType {
  int value;                    /* the distance */
  int mostRecent;               /* was this one in most recent return? */
  struct timeval time;          /* time of capture.  not yet precise   */
} sonarType;

typedef long sensor_set;	/* 2 bytes */

typedef struct tactileType {
  char value;
  struct timeval time;          /* time of capture.  not yet precise */
} tactileType;

typedef struct irType {
  int value;
  int mostRecent;
  struct timeval time;          /* time of capture.  not yet precise */
} irType;

/************************************************************************
 *  Simplest sonar commands
 *  NOTE : SONAR_init must be called prior to any other SONAR command.
 ************************************************************************/

BOOLEAN SONAR_init(void);
void SONAR_Debug(BOOLEAN flag);
void SONAR_outputHnd(int fd, long chars_available);
void SONAR_terminate(void);

/* 
 * We don't want to sonars running continuously. So, provide a pause/restart
 * capability
 */
void SONAR_pause(void);
void SONAR_continue(void);
BOOLEAN SONAR_pausedp(void);

/************************************************************************
 *  Low level sonar commands
 ************************************************************************/

/* Use the following to set the firing mask so as to select which
 * sonars get used.
 */
#define SENSOR_ZERO(set) ((set) = (long)0)
#define SENSOR_ALL(set) ((set) = 0xffffff)
#define SENSOR_SET(n,set) ((set) |= (1 << ((n)%SONAR_READINGS_PER_SWEEP)))
#define SENSOR_CLR(n,set) ((set) &= ~(1 << ((n)%SONAR_READINGS_PER_SWEEP)))
#define SENSOR_ISSET(n,set) ((set) & (1 << ((n)%SONAR_READINGS_PER_SWEEP)))

void SONAR_set_param(unsigned char delay,
		     sensor_set mask,
		     unsigned char range);

void SONAR_set_sequence(unsigned char spacing,
			unsigned char interleave,
			unsigned char first,
			unsigned char nfire);

/************************************************************************
 *  Higher level sonar commands
 ************************************************************************/

void SONAR_set_spacing(int spacing);
/* At any one time, fire every nth sonar where n=spacing (default 6).
 * Eg., spacing=24, fire one at a time.  spacing=3, fire every 3rd at any
 *      given time.  1<=spacing<=SONAR_READINGS_PER_SWEEP
 */

void SONAR_set_interleave(int interleave);
/* Set interleave pattern.  After firing ith sonar, fire (i+n)'th sonar
 * (mod spacing).  n=interleave.  (default 3).  1<=n<=255
 * Eg., with spacing=4, interleave=1: fire 1,2,3,4
 *      interleave=2: fire 1,3,2,4
 *      interleave=3: fire 1,4,3,2
 */

void SONAR_set_first(int first);
void SONAR_set_nfire(int nfire);
/* Set first and nfire.  Fire nfire sonars, starting at first
 * This is a mask on top of mask below.
 * 0<=first<=23, 1<=nfire<=SONAR_READINGS_PER_SWEEP
 */

void SONAR_set_delay(int delay);
/* specifies interfire delay (1-255msec [default 18])
 * Side effect: stops firing.
 */

void SONAR_set_mask(sensor_set mask);
/* Set 24 bit mask. (default 0xffffff)  bit 0=first sonar, bit 23=last sonar
 * Side effect: stops firing.
 */

void SONAR_set_range(int range);
/* Set range.
 * maximum range required from the ring (1-255 tenths of feet [default 255]).
 */

/************************************************************************
 *  Streaming sonar commands
 ************************************************************************/

/* Start/stop firing */
void SONAR_start_fire(void);
void SONAR_stop_fire(void);

/* Get one sweep */
void SONAR_req_data(Pointer call_back, Pointer client_data);

void SONAR_stream(struct timeval *interval,
		  Handler handler,
		  Pointer client_data);

BOOLEAN SONAR_is_streaming(void);

void SONAR_missed(Handler handler, Pointer client_data);

/*****************************************************************************
 * Timer related stuff - pauses the sonars, cause it to continue, checks to 
 * see if the sonars are paused, reset a master timer to 0, set the timeout 
 * before pauseing, and update (checking to see if the timeout has passed).
 *****************************************************************************/
void SONAR_pause(void);
void SONAR_continue(void);
BOOLEAN SONAR_pausedp(void);
void SONAR_reset_timer(void);
void SONAR_set_timeout(int);
void SONAR_update_timer(void);

void start_abus_sensors(Handler comm_handler);
void stop_abus_sensors(void);

/*****************************************************************************
 * EVENTS
 *
 *   To install an event handler, call the function SONAR_InstallHandler
 *
 *   void HandlerEvent1(<type> callback_data, Pointer client_data)
 *   {
 *   ...
 *   }
 *
 *   SONAR_InstallHandler(HandlerEvent1, EVENT1, client_data)
 *
 *****************************************************************************/

void SONAR_InstallHandler(Handler handler, int event, Pointer client_data,
			  BOOLEAN remove);
void  SONAR_RemoveHandler(Handler handler, int event);
/*
 *      EVENT                       Type of callback data
 *      *****                       *********************                 
 */

#define SONAR_SWEEP       0             /* SONAR_DATA_PTR data */
#define SONAR_MISSED      1             /* void *data */
#define SONAR_DISCONNECT  2             /* void *data */
#define SONAR_RECONNECT   3             /* void *data */

#define SONAR_NUMBER_EVENTS       4

extern SONAR_TYPE   sonar_device;

#endif
