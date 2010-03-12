/*****************************************************************************
 * PROJECT: Xavier
 *
 * (c) Copyright 1993 Richard Goodwin & Joseph O'Sullivan. All rights reserved.
 * (c) Copyright 1993 Hank Wan. All rights reserved.
 *
 * MODULE: sonar
 * FILE: sonar_interface.c
 *
 * ABSTRACT:
 * Wrapper package to allow easy access to Xavier's sonar unit.
 *
 *****************************************************************************/

#include <carmen/carmen.h>
#include "Common.h"
#include "b21a_base.h"
#include "abus_sensors.h"

#include "B21/Sonar.h"
#include "B21/Ir.h"
#include "B21/Tactile.h"
#include "B21/Base.h"
#include "B21/libmsp.h"
#include "B21/Rai.h"

#define SONAR_BUFFER_SIZE DEFAULT_LINE_LENGTH

SONAR_TYPE sonar_device;

static SONAR_DATA sonarData;
static unsigned char sonarReadings[SONAR_READINGS_PER_SWEEP];
static unsigned short sonarFrame = 0;
HandlerList sonar_handlers = NULL;
static BOOLEAN sonar_outstanding_request = FALSE;
Handler main_comm_handler;

/* variables that keep track of the current sonar settings */

static BOOLEAN sonar_paused = FALSE;
static int nSonarPauseTimer = 0;
static int nSonarPauseTimeout;
static int nAckPending = 0;
static double robot_radius = 26.7;

BOOLEAN workingSonar[NUM_SONARS];
SONAR_DATA rwiSonarData;
int irThreshold = 0;     /* Disabled */
int tactilesInRow[NUM_TACTILE_ROWS];
int irThreshold;
int irsInRow[NUM_IR_ROWS];

/*****************************************************************************
 * Forward procedure declarations
 *****************************************************************************/

static void ProcessSweepData(unsigned char *buf);
static void ProcessAck(unsigned char flag);
static BOOLEAN check_data(unsigned char *data, int ndata);
static unsigned int check_sum(unsigned char *data,int ndata);
static void write_with_check_sum(unsigned char *data,int ndata);
static void disconnectSONAR(DEV_PTR ignore);
static void reconnectSONAR(DEV_PTR ignore);

/*****************************************************************************
 *
 * FUNCTION: void SONAR_pause();
 * DESCRIPTION: Stops the sonar for a wee while.
 *
 * FUNCTION: void SONAR_continue();
 * DESCRIPTION: Starts sonars up following a pause
 *
 * FUNCTION: BOOLEAN SONAR_pausep();
 * DESCRIPTION: returns true if the sonars are paused.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 *****************************************************************************/

void SONAR_continue(void)
{
  if(sonar_paused) {
    sonar_paused = FALSE;
    SONAR_reset_timer();
  }
}

BOOLEAN SONAR_pausedp(void)
{
  return (sonar_paused);
}

void SONAR_reset_timer(void)
{
  nSonarPauseTimer = 0;
}

void SONAR_set_timeout(int n)
{
  nSonarPauseTimeout = n;
}

/*****************************************************************************
 *
 * FUNCTION: BOOLEAN SONAR_init(void);
 *
 * DESCRIPTION: Open the sonar device and initialize it.
 *
 * INPUTS:
 * SONAR_PTR init - structure describing sonar device.
 *
 * OUTPUTS: Returns true if device opened successfully, false otherwise.
 *
 *****************************************************************************/

BOOLEAN SONAR_init(void)
{
  sonar_device.dev = devCreateDev("sonar", NULL);
  sonar_handlers = CreateHandlerList(SONAR_NUMBER_EVENTS);
  return TRUE;
}

/* SONAR_Debug -- sets sonar debug flag */

void SONAR_Debug(BOOLEAN flag)
{
  devSetDebug(sonar_device.dev, flag);
}

/*****************************************************************************
 *
 * FUNCTION: BOOLEAN SONAR_outputHnd(int fd, long chars_available);
 *
 * DESCRIPTION: Handles character output from the sonar device.
 *
 *****************************************************************************/

void SONAR_outputHnd(int fd, long chars_available)
{
  static unsigned char buffer[SONAR_BUFFER_SIZE + 1];
  static unsigned char *startPos = buffer; /* position to start parsing from */
  static unsigned char *endPos = buffer; /* position to add more characters */
  int numRead = 0;

  while(chars_available > 0) {
    if(startPos == endPos) {
      startPos = endPos = buffer;
      bzero(buffer, SONAR_BUFFER_SIZE+1);
    }
    
    /* read in the output. */
    numRead = devReadN(fd, (char *)endPos,
		       MIN(chars_available,(SONAR_BUFFER_SIZE -
					    (endPos - startPos))));
    endPos += numRead;
    if(numRead == 0) { /* handle error here. The port is already closed. */
    }
    else while(startPos < endPos) {
      if(startPos[0] != 1) {
	fprintf(stderr, "sonar_interface: panic: bad station address\n");
	startPos++;
      } else if( endPos-startPos < 6 )
	break;
      else {
	int len = startPos[2]*256+startPos[3];
	if(len > 26)
	  fprintf(stderr,
		  "sonar_interface: panic: unusually long packet len: %d!\n",
		  len);
	if(endPos-startPos < len + 6)
	  break;
	else {
	  /* the whole packet has been read */
	  if(!check_data(startPos,len+6)) {
	    fprintf(stderr, "sonar_interface: panic: bad check sum\n");
	    startPos += len+6;
	  } else {
	    switch( startPos[1] ) {
	    case 2:	/* an ack */
	      if( len != 1 ) {
		fprintf( stderr, "sonar_interface: panic: unknown ack len\n" );
		startPos += len+6;
		break;
	      }
	      ProcessAck(startPos[4]);
	      if( --nAckPending < 0 ) {
		fprintf( stderr,
			"sonar_interface: warning: more acks than expected\n");
		nAckPending = 0;
	      }
	      startPos += len+6;
	      break;
	    case 6:	/* sonar sweep data */
	      if( nAckPending != 0 ) {
		fprintf( stderr,
			"sonar_interface: warning: number of Ack's out of sync\n" );
		nAckPending = 0;
	      }
	      if( len != 26 ) {
		fprintf( stderr,
			"sonar_interface: panic: unknown sweep len\n" );
		startPos += len+6;
		break;
	      }
	      ProcessSweepData(startPos);
	      startPos += len+6;
	      break;
	    default:
	      fprintf( stderr,
		      "sonar_interface: panic: unknown packet command: %d\n",
		      startPos[1] );
	      startPos += len+6;
	      break;
	    }
	  }
	}
      }
      /* Fix up the buffer. Throw out any consumed lines.*/
      if( startPos >= endPos ) {
	/* parsed the whole thing, just clean it all up */
	bzero(buffer, SONAR_BUFFER_SIZE+1);
	startPos = endPos = buffer;
      } else if (startPos != buffer) {
	/* slide it back and wait for more characters */
	bcopy(startPos, buffer, (endPos - startPos));
	endPos = buffer + (endPos - startPos);
	startPos = buffer;
      }
    }
    chars_available = devCharsAvailable(fd);
  }
}

static void ProcessSweepData(unsigned char *buf)
{
  int i;
  static int lastFrame = -1;

  sonarFrame = buf[4]*256+buf[5];
  if(sonarFrame != lastFrame) {
    lastFrame = sonarFrame;
#ifdef SONAR_CABLE_BROKEN
    for(i = 6; i < 14 ; i++)
      sonarReadings[i-6] = (buf[i] == 0 ? SONAR_MAX_RANGE : buf[i]);
    for(i = 14; i < 22 ; i++)
      sonarReadings[i-6+8] = (buf[i] == 0 ? SONAR_MAX_RANGE : buf[i]);
    for(i = 22; i < 30 ; i++)
      sonarReadings[i-6-8] = (buf[i] == 0 ? SONAR_MAX_RANGE : buf[i]);
#else
    for(i = 6; i < 30 ; i++)
      sonarReadings[i-6] = (buf[i] == 0 ? SONAR_MAX_RANGE : buf[i]);
#endif /* SONAR_CABLE_BROKEN */
  
    /* call the call back routine if there is one. */
    sonarData.frame = sonarFrame;
    sonarData.data = sonarReadings;
    FireHandler(sonar_handlers, SONAR_SWEEP, &sonarData);
  }
}

/*****************************************************************************
 *
 * FUNCTION: static void ProcessAck(unsigned char flag)
 *
 * DESCRIPTION: used to process ack
 *
 *****************************************************************************/

#define ACK_LENGTH 7

static void ProcessAck(unsigned char flag)
{
  switch(flag) {
  case 0:
    /* cancel the timeout. */
    if (devGetDebug(sonar_device.dev))
      fprintf(stderr, "Ack Received\n" );
    devCancelTimeout(sonar_device.dev);
    break;
  case 0x7f:
    fprintf(stderr, "read_ack: illegal parameter signaled\n");
    /* what to do if there is a bad ack? */
    break;
  case 0x7e:
    fprintf(stderr, "read_ack: bad check sum signaled\n");
    /* what to do if there is a bad ack? */
    break;
  default:
    fprintf(stderr, "read_ack: error: response code %d\n", (int)flag);
    /* what to do if there is a bad ack? */
    break;
  }
}

/*****************************************************************************
 *
 * FUNCTION: static unsigned int check_sum(data,ndata)
 *
 * DESCRIPTION: Calculate the check sum.
 *
 * OUTPUTS: Returns checksum.
 *
 *****************************************************************************/

static unsigned int check_sum(unsigned char *data,int ndata)
{
  int sum = 0;
  while((ndata--) > 0) {
    sum += *data++;
    sum %= 256*256;
  }
  return sum;
}

/*****************************************************************************
 *
 * FUNCTION: static void write_with_check_sum(unsigned char *data,
 *                                            int ndata)
 *
 * DESCRIPTION: Add a checksum to the buffer and write it.
 *
 * NOTES : ndata is size of data, including checksum word
 *
 *****************************************************************************/

static void write_with_check_sum(unsigned char *data, int ndata)
{
  unsigned int sum = check_sum(data, ndata-2);
  data[ndata-2] = sum / 256;
  data[ndata-1] = sum % 256;
  devWriteN(devGetFd(sonar_device.dev), (char *)data, ndata);
}

/*****************************************************************************
 *
 * FUNCTION: static BOOLEAN check_data(unsigned char *data, int ndata)
 *
 * DESCRIPTION: Check the checksum.
 *
 * OUTPUTS: Returns true if it is a valid checksum.
 *
 *****************************************************************************/

static BOOLEAN check_data(unsigned char *data, int ndata)
     /* ndata includes check sum bytes */
{
  unsigned int sum = check_sum(data, ndata - 2);

  if(sum != data[ndata - 2] * 256 + data[ndata - 1])
    return FALSE;
  return TRUE;
}


void SONAR_InstallHandler(Handler handler, int event, Pointer client_data,
			  BOOLEAN remove)
{
  InstallHandler(sonar_handlers, handler, event, client_data, remove);
}

void SONAR_RemoveHandler(Handler handler, int event)
{
  RemoveHandler(sonar_handlers, handler, event);
}

/*****************************************************************************
 *
 * FUNCTION: void SONAR_stream(struct timeval *interval,
 *                             Handler handler,
 *                             Pointer client_data)
 *
 * DESCRIPTION: start streaming of sonar data to the handler.
 *
 *****************************************************************************/

void SONAR_stream(struct timeval *interval, Handler handler,
		  Pointer client_data)
{
  SONAR_InstallHandler(handler, SONAR_SWEEP, client_data, FALSE);
}

/*****************************************************************************
 *
 * FUNCTION: BOOLEAN SONAR_is_streaming(void)
 *
 * DESCRIPTION: returns true if the sonar is streaming.
 *
 *****************************************************************************/

BOOLEAN SONAR_is_streaming(void)
{
  return devIsPolling(sonar_device.dev);
}

/*****************************************************************************
 *
 * FUNCTION: void SONAR_missed(Handler handler,
 *                             Pointer client_data)
 *
 * DESCRIPTION: register a function to handle cases where a sonar sweep
 * never arrives before the next sweep is requested.
 *
 *****************************************************************************/

void SONAR_missed(Handler handler, Pointer client_data)
{
  SONAR_InstallHandler(handler, SONAR_MISSED, client_data, FALSE);
}

/*****************************************************************************
 *
 * FUNCTION: void disconnectSONAR(void)
 *
 *
 *****************************************************************************/

static void disconnectSONAR(DEV_PTR ignore)
{
  /* called when the sonar disconnects. */
  devStopPolling(sonar_device.dev);
  FireHandler(sonar_handlers, SONAR_DISCONNECT, (Pointer) NULL);
}


/*****************************************************************************
 *
 * FUNCTION: void reconnectSONAR(void)
 *
 *****************************************************************************/

static void reconnectSONAR(DEV_PTR ignore)
{
  fprintf(stderr,"Reconnecting to the sonar:\n");
  FireHandler(sonar_handlers, SONAR_RECONNECT, (Pointer) NULL);
}

static void baseTCACallback(RaiModule *mod)
{
  struct timeval TCA_waiting_time = {0, 10};
  devProcessDevices(&TCA_waiting_time); 
  main_comm_handler(NULL, NULL);
}

static int sonarCallback(sonarType *sonars)
{
  int     index, newindex;
  static  int sonarFrame = 0;
  static  unsigned char sonarReadings[NUM_SONARS];
  static  unsigned char reportSonarReadings[NUM_SONARS];
  static  BOOLEAN fired[NUM_SONARS];
  static  int allFired = 0;

  if(allFired == 0)
    for(index = 0; index < NUM_SONARS; index++) {
      fired[index] = FALSE;
      sonarReadings[index] = 0; /*SONAR_MAX_RANGE*/
    }
  
  for(index = 0; index < NUM_SONARS; index++)
    if(sonars[index].mostRecent) {
#ifndef MSPMODULEBROKEN
      newindex = (index + 12) % NUM_SONARS;
#else
      /* don't laugh, I've been up late.
       * Anyway, thats what compilers are for 
       */
      newindex = (index + 12) % NUM_SONARS;
      if(newindex < 16)
	newindex = (newindex - 8) % NUM_SONARS;
      else
	newindex = (newindex) % NUM_SONARS;
#endif
      if(sonars[index].value == 0x7FFF)
	sonarReadings[newindex] = SONAR_MAX_RANGE; 
      else {
	if(sonars[index].value < robot_radius * 10.0)
	  sonarReadings[newindex] = 0; 
	else 
	  sonarReadings[newindex] = 
	    ((sonars[index].value - robot_radius * 10.0) * 12)/(10*CMPERFOOT);
      }
      if(sonarReadings[newindex] > SONAR_MAX_RANGE)
	sonarReadings[newindex] = SONAR_MAX_RANGE;
      if(!fired[newindex]) {
        fired[newindex] = TRUE; 
	/*        if (!((newindex>=8) && (newindex<16)))*/
	allFired++;
      }
      
#ifndef MSPMODULEBROKEN
      if(allFired == 24) {
#else
      if(allFired == 16) {
#endif
        /* call the call back routine if there is one. */
        rwiSonarData.frame = sonarFrame++;
        for(index = 0; index < NUM_SONARS; index++)
          if(workingSonar[index])
	    reportSonarReadings[index] = sonarReadings[index];
	  else
	    reportSonarReadings[index] = SONAR_MAX_RANGE;
        rwiSonarData.data  = reportSonarReadings;
        FireHandler(sonar_handlers, SONAR_SWEEP, &rwiSonarData);
        allFired = 0;
      }
    }
  return 0;
}

/*
 * Convert a tactile position to a BumpSwitch identifier - 
 * Amelia counts from the reset plate, Xavier from the charger.
 */
static unsigned int tactileIndexToBumpSwitch(int index)
{
  return (~(1 << ((9 + index) % 16)));
}

static int tactileCallback(tactileType **tactileMatrix)
{
  int index, row;
  int touched = FALSE;
  unsigned int switches = 0xFFFF;
 
  for(row = 0; row < NUM_TACTILE_ROWS; row++)
    for(index = 0; index < tactilesInRow[row]; index++)
      if(tactileMatrix[row][index].value) {
	touched = TRUE;
	/* OK, a bump has happened. Lets convert it into Xavier
	 * coordinartes so that we can don't have to change processBumps
	 * If its from row 1, then we have to convert to base coordinates, 
	 * Otherwise, we simply convert to the correct orientation
	 */	
	if ((row == 2) || (row == 3)) 
	  switches = switches & tactileIndexToBumpSwitch(index);
      }
  
  if(touched) {
    SET_ROBOT_STATE(bumpers, switches);
  }
  else {
    SET_ROBOT_STATE(bumpers, 0xFFFF);
  }
  if(rwi_base.bumpers != 0xFFFF && previous_state.bumpers == 0xFFFF) {
    rwi_base.bump = TRUE;
    if (rwi_base.stopWhenBump) {
      BASE_Halt();
      BASE_Kill();
      ProcessBump(rwi_base.bumpers);
    }
  }
  if(rwi_base.bumpers == 0xFFFF && previous_state.bumpers != 0xFFFF) {
    rwi_base.bump = FALSE;
    if(rwi_base.stopWhenBump)
      rwi_base.emergency = FALSE;
  }
  return 0;
}

static int irCallback(irType **irMatrix)
{
  int index, row;
  int targetLevel;
  int close = FALSE;
  unsigned int switches = 0xFFFF;
 
  if(!irThreshold)       /* No way to actually switch the callback off?? */
    return 0;

  row = 1; /* Just look at the row at the base */
  for(index = 0; index < irsInRow[row]; index++)
    if(irMatrix[row][index].value > irThreshold) {
      close = TRUE;
      targetLevel = row;
      printf("Ir near at %d, %d\n",row,index);
      /* OK, a bump has happened. Lets convert it into Xavier
       * coordinartes so that we can don't have to change processBumps
       * If its from row 1, then we have to convert to base coordinates, 
       * Otherwise, we simply convert to the correct orientation
       */	
      if((row == 2) || (row == 3)) 
	switches = switches & tactileIndexToBumpSwitch(index);
    }
  
  if(close)
    rwi_base.bumpers = switches;
  else
    rwi_base.bumpers = 0xFFFF;
  
  if(rwi_base.bumpers != 0xFFFF && previous_state.bumpers == 0xFFFF) {
    rwi_base.bump = TRUE;
    if(rwi_base.stopWhenBump) {
      BASE_Halt();
      BASE_Kill();
      ProcessBump(rwi_base.bumpers);
    }
  }
  if(rwi_base.bumpers == 0xFFFF && previous_state.bumpers != 0xFFFF) {
    rwi_base.bump = FALSE;
    if(rwi_base.stopWhenBump)
      rwi_base.emergency = FALSE;
  }
  return 0;
}

void start_abus_sensors(Handler comm_handler)
{
  RaiModule *base_module;
  int i;

  main_comm_handler = comm_handler;
  for(i = 0; i < 24; i++)
    workingSonar[i] = TRUE;

  RaiInit();
  base_module = makeModule("rwi", NULL);
  irInit();
  tactileInit();
  sonarInit();
  sonarStop();        /* stop-start to include new MSP */
  sonarStart();
  
  addPolling(base_module, baseTCACallback, 15);
  registerTactileCallback(tactileCallback);
  registerIrCallback(irCallback);
  registerSonarCallback(sonarCallback);
  RaiStart();  
}

void stop_abus_sensors(void)
{
  sonarStop();
  RaiShutdown();  
}
