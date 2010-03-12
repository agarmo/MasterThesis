/*****************************************************************************
 * PROJECT: TCA
 *
 * (c) Copyright Richard Goodwin, 1995. All rights reserved.
 *
 * FILE: devUtils.c
 *
 * ABSTRACT:
 * 
 * This file provides a set for routines for use with device interfaces.
 * A device interface is a set of routines that provides high level access
 * to a device through its device driver.  All device interfaces are required
 * to be able to connect to a socket rather than the device driver.  The 
 * routine provided in the file are to help with connecting to a simulator.
 *
 * ADAPTED FROM XAVIER SOFTWARE TO FACILITATE ADDING I/O TO MODULES.
 *
 *****************************************************************************/

#include <carmen/carmen.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <netdb.h>
#include "Common.h"
#include "handlers.h"
#include "timeUtils.h"
#include "devUtils.h"
#include <stdarg.h>

/*****************************************************************************
 * Global constants
 *****************************************************************************/

#define RECONNECT_INTERVAL  10

/*****************************************************************************
 * Global types
 *****************************************************************************/

typedef struct _dev_line_buffer_type
{ char *buffer;
  char *string;
  int length;
  int nextChar;
  char *delimChar;
  int lineLimit;
  void (*processRoutine)(char *, void *);
  void *clientData;
  BOOLEAN replaceDelimit;
  BOOLEAN partialLines;
} LINE_BUFFER_TYPE;

#ifdef DEV_FUNCTIONAL_ONLY
typedef struct _dev {
  DEV_STATUS_TYPE status;
  BOOLEAN use_simulator;
  struct {
    const char *machine;
    int portNumber;
  } sim_dev;
  struct {
    const char *ttyPort;
    int baudCode;   /* one of the codes B0 .. B9600 in ttydev.h */
    TTY_OPEN_TYPE lineDiscipline;
  } ttydev;
  const char *devName;
  fd_set acceptFds;
  fd_set listenFds;
  fd_set talkFds;
  fd_set fds;
  int fd;
  LISTENING_TYPE listen;
  BOOLEAN debug;
  FILE *debug_file;
  fd_set *readMask;
  DEVICE_OUTPUT_HND outputHnd;
  _HandlerEntry timeoutHnd;
  struct timeval timeOutTime;
  _HandlerEntry pollHnd;
  struct timeval pollInterval;
  struct timeval pollTime;
  void (* sigHnd)(DEV_PTR);
  void (* disconnectHnd)(DEV_PTR);
  void (* reconnectHnd)(DEV_PTR);
  void *ClientData;
}  DEV_TYPE;
#endif

/*****************************************************************************
 * Global variables
 *****************************************************************************/

/* An array of devices. */
static DEV_PTR devices[FD_SETSIZE];
static fd_set devConnections;       /* list of open file descriptors */
static fd_set devListenConnections; /* list of file descriptors to listen to.*/
static int num_devices = 0;         /* highest index of any device. */

/* the open device array indexes devises by their file id. */
static DEV_PTR open_devices[FD_SETSIZE]; 
static int maxDevFd = 0; /* highest fid of device actually connected */

static struct timeval MAX_CYCLE_TIME = {10,0};   /* ten seconds. */
static struct timeval ZERO_TIMEOUT = {0,0};      /* zero timeout. */
static struct timeval Now = {0,0};  /* the current time. */

static int waiting = 0; /* the number of handlers waiting for returns */

static BOOLEAN PipeBroken = FALSE;  /* set when a sigpipe is received. */

static BOOLEAN inBackground=FALSE;  /* indicates process is in background */

/* Devices marked for closing. Prevents infinite looping when there is an 
 * error closing a device and devUtils tries to close it again. 
 */
static fd_set devices_closing;

/* Devices marked for freeing.  Have to be careful not to free a device while
 * in a signal handler.
 */
static DEV_PTR devices_freeing[FD_SETSIZE];
static int devicesToFree=0;  /* indicates devices need to be freed. */

static DEV_TYPE devDefaultDevValues =
{
  NOT_CONNECTED,
  FALSE,
  { "", DEFAULT_PORT},
  { "", DEFAULT_BAUD,TTY_OPEN_RAW},
  "",
  NO_FDS,
  NO_FDS,
  NO_FDS,
  NO_FDS,
  NO_FD,
  LISTENING | TALKING,
  FALSE,
  TRUE,
  (FILE *) NULL,
  &devConnections,
  (DEVICE_OUTPUT_HND) NULL,
  Null_Handler,
  {TIME_MAX, 0},
  Null_Handler,  
  {0, 0},
  {TIME_MAX, 0},
  (void (*)(DEV_PTR)) NULL,
  (void (*)(DEV_PTR)) NULL,
  (void (*)(DEV_PTR)) NULL,
};

/*****************************************************************************
 * Forward Declarations.
 *****************************************************************************/
void devSetParametersArgs(DEV_PTR device, va_list args);
int devConnectTottySimple(DEV_PTR dev);
int devConnectToNonttySimple(DEV_PTR dev);
#undef devIsConnected
BOOLEAN devIsConnected(DEV_PTR dev);

/* Functions to make the conversion from TCA to IPC easier */
BOOLEAN connectAt(const char *machine, int port, int *readSd, int *writeSd);
BOOLEAN x_ipc_connectAt(const char *machine, int port,
                        int *readSd, int *writeSd);
BOOLEAN connectAt(const char *machine, int port, int *readSd, int *writeSd)
{ return x_ipc_connectAt(machine, port, readSd, writeSd); }

BOOLEAN listenAtPort(int *port, int *sd);
BOOLEAN x_ipc_listenAtPort(int *port, int *sd);
BOOLEAN listenAtPort(int *port, int *sd)
{ return x_ipc_listenAtPort(port, sd); }

BOOLEAN listenAtSocket(int port, int *sd);
BOOLEAN x_ipc_listenAtSocket(int port, int *sd);
BOOLEAN listenAtSocket(int port, int *sd)
{ return x_ipc_listenAtSocket(port, sd); }


/*****************************************************************************
 *
 * FUNCTION: BOOLEAN fd_isZero(fd_set *fds);
 *
 * DESCRIPTION: Is anything in the fd set set?
 *
 * INPUTS:
 *
 * OUTPUTS: 
 *
 * HISTORY:
 *
 *****************************************************************************/

BOOLEAN fd_isZero(fd_set *fds)
{
  unsigned int i;
  for (i=0; i<howmany(FD_SETSIZE,NFDBITS); i++) {
    if (fds->fds_bits[i] != 0)
      return FALSE;
  }
  return TRUE;
}


/*****************************************************************************
 *
 * FUNCTION: void fd_copy(fd_set *src, fd_set *dst)
 *
 * DESCRIPTION: Copy the fd set.
 *
 * INPUTS:
 *
 * OUTPUTS: 
 *
 * HISTORY:
 *
 *****************************************************************************/

void fd_copy(fd_set *src, fd_set *dst)
{
  unsigned int i;
  for (i=0; i<howmany(FD_SETSIZE,NFDBITS); i++) {
    dst->fds_bits[i] = src->fds_bits[i];
  }
}


/*****************************************************************************
 *
 * FUNCTION: void fd_copy_or(fd_set *src, fd_set *dst)
 *
 * DESCRIPTION: Copy the fd set oring it with the destination.
 *
 * INPUTS:
 *
 * OUTPUTS: 
 *
 * HISTORY:
 *
 *****************************************************************************/

void fd_copy_or(fd_set *src, fd_set *dst)
{
  unsigned int i;
  for (i=0; i<howmany(FD_SETSIZE,NFDBITS); i++) {
    dst->fds_bits[i] |= src->fds_bits[i];
  }
}


/*****************************************************************************
 *
 * FUNCTION: void fd_copy_xor(fd_set *src, fd_set *dst)
 *
 * DESCRIPTION: Copy the fd set oring it with the destination.
 *
 * INPUTS:
 *
 * OUTPUTS: 
 *
 * HISTORY:
 *
 *****************************************************************************/

void fd_copy_xor(fd_set *src, fd_set *dst)
{
  unsigned int i;
  for (i=0; i<howmany(FD_SETSIZE,NFDBITS); i++) {
    dst->fds_bits[i] ^= src->fds_bits[i];
  }
}


/*****************************************************************************
 *
 * FUNCTION: BOOLEAN fd_equal(fd_set *set1, fd_set *set2)
 *
 * DESCRIPTION: Compare fd sets.
 *
 * INPUTS:
 *
 * OUTPUTS: 
 *
 * HISTORY:
 *
 *****************************************************************************/

BOOLEAN fd_equal(fd_set *set1, fd_set *set2)
{
  unsigned int i;
  for (i=0; i<howmany(FD_SETSIZE,NFDBITS); i++) {
    if (set1->fds_bits[i] != set2->fds_bits[i]) 
      return FALSE;
  }
  return TRUE;
}


/*****************************************************************************
 *
 * FUNCTION: void fd_copy_clear(fd_set *src, fd_set *dst)
 *
 * DESCRIPTION: Copy the fd set, clearing set bits.
 *
 * INPUTS:
 *
 * OUTPUTS: 
 *
 * HISTORY:
 *
 *****************************************************************************/

#if 0
static void fd_copy_clear(fd_set *src, fd_set *dst)
{
  int i;
  for (i=0; i<howmany(FD_SETSIZE,NFDBITS); i++) {
    dst->fds_bits[i] = dst->fds_bits[i] & (~(src->fds_bits[i]));
  }
}
#endif 


/*****************************************************************************
 *
 * FUNCTION: void devShutdown(void);
 *
 * DESCRIPTION: Shuts all the devices.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 * HISTORY:
 *
 *****************************************************************************/

void devShutdown(void)
{
  int i;
  
  fprintf(stderr,"\n Shutting down\n");
  fflush(stderr);
  
  for(i=0; i<=num_devices; i++){
    if (devices[i] != NULL) {
      devDisconnectDev(devices[i],FALSE,FALSE);
    }
  }
}

/*****************************************************************************
 *
 * FUNCTION: void devSignal(void);
 *
 * DESCRIPTION: Handles various signals.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 * HISTORY:
 *
 *****************************************************************************/

#if defined(sun) && !defined(__svr4__)
static void devSignal(int s, int c, struct sigcontext *sp)
#else
     static void devSignal(int s)
#endif
{
  /* the default thing to do is call the exit routine for each device and
   * then disconnect the device and exit.
   * The only exception is a write to a broke pipe.
   * Just close device and continue.
   *
   * Make sure the signal handler is not re-entered.
   */
  
  static BOOLEAN in_handler = FALSE;
  
  if ((in_handler) && (s != SIGTTIN)){
#if defined(sun) && !defined(__svr4__)
    fprintf(stderr,"\n Recursive Abort: signal %d %d; cleaning up.\n",s,c);
#else
    fprintf(stderr,"\n Recursive Abort: signal %d ; cleaning up.\n",s);
#endif
  } else {
    in_handler = TRUE;
    if (s == SIGPIPE) {
      /* A pipe closed during a write.  Just let the write handle it */
#if defined(sun) && !defined(__svr4__)
      fprintf(stderr,"\n Abort: caught a broken pipe signal %d %d\n",s,c);
      fprintf(stderr,"cleaning up closed connection.\n");
#else
      fprintf(stderr,"\n Abort: caught a broken pipe signal %d\n",s);
      fprintf(stderr,"cleaning up closed connection.\n");
#endif
      PipeBroken = TRUE;
    } else if (s == SIGTTIN) {
      signal(SIGTTIN, SIG_IGN);
      inBackground=TRUE;
      /* disconnectDev(&stdin_device, FALSE, FALSE);*/
    } else {
      /* only thing to do for other signals is to shut down gracefully */
#if defined(sun) && !defined(__svr4__)
      fprintf(stderr,"\n Abort: caught a signal %d %d; cleaning up.\n",s,c);
#else
      fprintf(stderr,"\n Abort: caught a signal %d; cleaning up.\n",s);
#endif
      devShutdown();
      fflush(stderr);
      exit(-1);
    }
  }
  fflush(stderr);
  in_handler = FALSE;
}

/*****************************************************************************
 *
 * FUNCTION: void devInit(void);
 *
 * DESCRIPTION: Initialze the data structures used by devUtils.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 * HISTORY:
 *
 *****************************************************************************/

void devInit(void)
{
  static BOOLEAN firstTime = TRUE;
  if (firstTime) {
    firstTime = FALSE;
    FD_ZERO(&devConnections);
    FD_ZERO(&devListenConnections);
    bzero(devices, sizeof(devices));
    bzero(devices_freeing, sizeof(devices));
  }
  /* set up signal handlers */
  signal(SIGINT,  devSignal);
  /* signal(SIGQUIT, devSignal); */
  signal(SIGILL,  devSignal);
  signal(SIGFPE,  devSignal);
  signal(SIGBUS,  devSignal);
  signal(SIGSEGV, devSignal);
  signal(SIGPIPE, devSignal);
  
  signal(SIGTTIN, devSignal);
  /*    signal(SIGTERM, devSignal);*/
  /*    signal(SIGURG, devSignal);*/
  /*    signal(SIGSTOP, devSignal);*/
  /*    signal(SIGTSTP, devSignal);*/
  /*    signal(SIGCONT, devSignal);*/
  /*    signal(SIGCHLD, devSignal);*/
  /*    signal(SIGTTOU, devSignal);*/
  /*    signal(SIGIO, devSignal);*/
  /*    signal(SIGWINCH, devSignal);*/
  /*    signal(SIGLOST, devSignal);*/
  /*    signal(SIGUSR1, devSignal);*/
  /*    signal(SIGUSR2, devSignal);*/
  
  /* signal(SIGSYS,  devSignal); */
  /* signal(SIGTERM, devSignal); */
}


/*****************************************************************************
 *
 * FUNCTION: DEV_PTR devCreateDev(const char *name, ...)
 *
 * DESCRIPTION:
 * devCreateDev: Create a device data structure and initialize it.
 * The first parameter is a string that gives the name of the device.
 * This is followed by a variable length list of parameter name, 
 * parameter value pairs.  The list is terminated with a NULL parameter.
 * For example myDev = devCreateDev("myDev" DEV_OUTPUTHND, myOutputHnd, NULL);
 * Unset parameters are the to their defaults, usually NULL.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 * HISTORY:
 *
 *****************************************************************************/

DEV_PTR devCreateDev(const char *name, ...)
{
  DEV_PTR dev;
  va_list args;
  
  devInit();
  dev = (DEV_PTR)malloc(sizeof(DEV_TYPE));
  carmen_test_alloc(dev);
  *dev = devDefaultDevValues;
  dev->devName = name;
  va_start(args, name);
  devSetParametersArgs(dev,args);
  va_end(args);
  return dev;
}

/*****************************************************************************
 *
 * FUNCTION: DEV_PTR devCreateTTYDev(const char *name, ...)
 *
 * DESCRIPTION:
 * devCreateTTYDev: Same as devCreateDev, except that the defaults are set
 * for a device that opens a tty port for I/O.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 * HISTORY:
 *
 *****************************************************************************/

DEV_PTR devCreateTTYDev(const char *name, ...)
{
  DEV_PTR device;
  va_list args;
  
  va_start(args, name);
  devInit();
  device = (DEV_PTR)malloc(sizeof(DEV_TYPE));
  carmen_test_alloc(device);
  *device = devDefaultDevValues;
  device->devName = name;
  devSetListening(device, LISTENING | TALKING);
  devSetReconnectHnd(device, devConnect);
  devSetParametersArgs(device,args);
  va_end(args);
  return device;
}


/*****************************************************************************
 *
 * FUNCTION: void devFreeDev(DEV_PTR dev)
 *
 * DESCRIPTION:
 * devFreeDev: Disconnects the device and frees the memory associated with a 
 * device.  It can be called from the disconnectHnd.  The pointer is invalid
 * after the call.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 * HISTORY:
 *
 *****************************************************************************/

static void devReallyFreeDevs(void)
{
  int i,j;
  DEV_PTR dev;
  
  for(i=0;i < FD_SETSIZE; i++) {
    if (devices_freeing[i]) {
      dev = devices_freeing[i];
      for (j=0; j<FD_SETSIZE; j++) {
	if (devices[j] == dev) {
	  devices[j] = NULL;
	}
      }
      free(dev);
      for (j=0; j<FD_SETSIZE; j++) {
	if (devices_freeing[j] == dev) {
	  devices_freeing[j] = NULL;
	}
      }
    }
  }
  devicesToFree=0;
}


/*****************************************************************************
 *
 * FUNCTION: void devFreeDev(DEV_PTR dev)
 *
 * DESCRIPTION:
 * devFreeDev: Disconnects the device and frees the memory associated with a 
 * device.  It can be called from the disconnectHnd.  The pointer is invalid
 * after the call.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 * HISTORY:
 *
 *****************************************************************************/

void devFreeDev(DEV_PTR dev)
{
  int i;

  if (dev != NULL) {
    /* Don't disconnect if already in the free list */
    for (i=0; i<FD_SETSIZE; i++) {
      if (devices_freeing[i] == dev)
	return;
    }

    devDisconnectDev(dev,FALSE,FALSE);
    
    devices_freeing[devicesToFree++] = dev;
  }
}

/*****************************************************************************
 *
 * FUNCTION: DEV_PTR devConnect(DEV_PTR device)
 *
 * DESCRIPTION:
 *
 * devConnect: connect the device according the the parameter settings.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 * HISTORY:
 *
 *****************************************************************************/

void devConnect(DEV_PTR device)
{
  if (device->use_simulator) {
    /* set up to use the simulator */
    devConnectDev(device,devConnectToSimulator(device));
  } else if ((device->listen & (ACCEPTING)) == (ACCEPTING)){
    /* code to open the real device here. */
    devServerInitialize(device);
  } else if ((device->listen & (LISTENING | TALKING)) == (LISTENING| TALKING)){
    /* code to open the real device here. */
    devConnectTotty(device);
  } else if ((device->listen & LISTENING) == LISTENING) {
    /* code to open the real device here. */
    devConnectTotty(device);
  }
  
}

/*****************************************************************************
 *
 * FUNCTION: void devSetParameters(DEV_PTR device, ...)
 *
 * DESCRIPTION:
 * devSetParameters: Set parameters for a device.
 * The first parameter is the device data structure.
 * This is followed by a variable length list of parameter name, 
 * parameter value pairs.  The list is terminated with a NULL parameter.
 * For example devSetParameters(myDev DEV_OUTPUTHND, myOutputHnd, NULL);
 * Unset parameters are left unchanged.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 * HISTORY:
 *
 *****************************************************************************/

void devSetParametersArgs(DEV_PTR device, va_list args)
{
  DEV_PARAM_TYPE parameter;
  const char *strValue;
  int intValue;
  FILE *fileValue;
  DEVICE_OUTPUT_HND outputHnd;
  void (* hndValue)(DEV_PTR);
  
  parameter = (DEV_PARAM_TYPE) va_arg(args, int);
  while (parameter != DEV_END_LIST) {
    switch (parameter) {
      
    case DEV_END_LIST:
      break;
      
    case DEV_USE_SIMULATOR:
      intValue = va_arg(args, int);
      device->use_simulator = intValue;
      break;
      
    case DEV_SIM_MACHINE:
      strValue = va_arg(args, const char *);
      device->sim_dev.machine = strValue;
      break;
      
    case DEV_PORT_NUMBER:
      intValue = va_arg(args, int);
      device->sim_dev.portNumber = intValue;
      break;
      
    case DEV_TTY_PORT:
      strValue = va_arg(args, const char *);
      device->ttydev.ttyPort = strValue;
      break;
      
    case DEV_BAUD_CODE:
      intValue = va_arg(args, int);
      device->ttydev.baudCode = intValue;
      break;
      
    case DEV_LISTENING:
      intValue = va_arg(args, int);
      device->listen = intValue;
      break;

    case DEV_LINE_DISCIPLINE:
      intValue = va_arg(args, int);
      device->ttydev.lineDiscipline = (TTY_OPEN_TYPE)intValue;
      break;
      
    case DEV_DEBUG:
      intValue = va_arg(args, int);
      device->debug = intValue;
      break;
      
    case DEV_DEBUG_FILE:
      fileValue = va_arg(args, FILE *);
      device->debug_file = fileValue;
      break;
      
    case DEV_OUTPUTHND:
      outputHnd = va_arg(args, DEVICE_OUTPUT_HND);
      device->outputHnd = outputHnd;
      break;
      
    case DEV_SIGNALHND:
      hndValue = (void (*)(DEV_PTR)) va_arg(args, void *);
      device->sigHnd = hndValue;
      break;
      
    case DEV_DISCONNECTHND:
      hndValue = (void (*)(DEV_PTR)) va_arg(args, void *);
      device->disconnectHnd = hndValue;
      break;
      
    case DEV_RECONNECTHND:
      hndValue = (void (*)(DEV_PTR)) va_arg(args, void *);
      device->reconnectHnd = hndValue;
      break;
      
    case DEV_CLIENT_DATA:
      device->clientData = (void *) va_arg(args, void *);
      break;
      
    case DEV_CLOSE_ON_ZERO:
      device->closeOnZero = (BOOLEAN) va_arg(args, BOOLEAN);
      break;
      
#ifndef TEST_CASE_COVERAGE
    default:
      fprintf(stderr, "Invalid device parameter %d", parameter);
      fprintf(stderr, 
	      "Probably missing a NULL at the end of a devCreateDev or devSetParameters call.\n");
      return;
      break;
#endif      
    }
    parameter = (DEV_PARAM_TYPE) va_arg(args, int);
  }
  va_end(args);
}

void devSetParameters(DEV_PTR device, ...)
{
  va_list args;
  
  va_start(args, device);
  devSetParametersArgs( device, args);
  va_end(args);
}

/*****************************************************************************
 *
 * FUNCTION: void devSetParameter(DEV_PTR device, DEV_PARAM_TYPE param, 
 *                                void *value)
 *
 * DESCRIPTION:
 * devSetParameter: Set a parameter for a device.
 * The first parameter is the device data structure.
 * This is followed by a parameter name, 
 * parameter value pair.
 * For example devSetParameter(myDev DEV_OUTPUTHND, myOutputHnd);
 * Unset parameters are left unchanged.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 * HISTORY:
 *
 *****************************************************************************/

void devSetParameter(DEV_PTR device, DEV_PARAM_TYPE param, void *value)
{
}

void devSetUseSimulator(DEV_PTR device, BOOLEAN useSim)
{ device->use_simulator = useSim;}

void devSetSimMachine(DEV_PTR device, const char *machine)
{ device->sim_dev.machine = machine;}

void devSetPort(DEV_PTR device, int port)
{ device->sim_dev.portNumber = port;}

void devSetBaudRate(DEV_PTR device, int baudRate)
{ device->ttydev.baudCode = baudRate;}

void devSetListening(DEV_PTR device, unsigned int listen)
{ device->listen = listen;}

void devSetDebug(DEV_PTR device, BOOLEAN debug)
{ device->debug = debug;}

void devSetDebugFile(DEV_PTR device, FILE *file)
{ device->debug_file = file;}

void devSetOutputHnd(DEV_PTR device, DEVICE_OUTPUT_HND hnd)
{ device->outputHnd = hnd;}

void devSetSignalHnd(DEV_PTR device, void (* hnd)(DEV_PTR))
{ device->sigHnd = hnd;}

void devSetDisconnectHnd(DEV_PTR device, void (* hnd)(DEV_PTR))
{ device->disconnectHnd = hnd;}

void devSetReconnectHnd(DEV_PTR device, void (* hnd)(DEV_PTR))
{ device->reconnectHnd = hnd;}

void devSetClientData(DEV_PTR device, void * data)
{ device->clientData = data;}

void devSetCloseOnZero(DEV_PTR device, BOOLEAN closeOnZero)
{ device->closeOnZero = closeOnZero;}


/* 
 * functions for getting the parameter values one at at time.
 */

const char *devGetName(DEV_PTR device)
{ return device->devName;}

fd_set *devGetFds(DEV_PTR device)
{ return &device->listenFds;}

BOOLEAN devGetUseSimulator(DEV_PTR device)
{ return device->use_simulator;}

const char *devGetSimMachine(DEV_PTR device)
{ return device->sim_dev.machine;}

int devGetPort(DEV_PTR device)
{ return device->sim_dev.portNumber;}

int devGetBaudRate(DEV_PTR device)
{ return device->ttydev.baudCode;}

unsigned int devGetListening(DEV_PTR device)
{ return device->listen;}

BOOLEAN devGetDebug(DEV_PTR device)
{ return device->debug;}

FILE *devGetDebugFile(DEV_PTR device)
{ return device->debug_file;}

DEVICE_OUTPUT_HND devGetOutputHnd(DEV_PTR device)
{ return device->outputHnd;}

DEVICE_HANDLER devGetSignalHnd(DEV_PTR device)
{ return device->sigHnd;}

DEVICE_HANDLER devGetDisconnectHnd(DEV_PTR device)
{ return device->disconnectHnd;}

DEVICE_HANDLER devGetReconnectHnd(DEV_PTR device)
{ return device->reconnectHnd;}

void *devGetClientData(DEV_PTR device)
{ return device->clientData;}

BOOLEAN devGetCloseOnZero(DEV_PTR device)
{ return device->closeOnZero;}

struct timeval *devGetPollInterval(DEV_PTR device)
{ return &device->pollInterval;}


/*****************************************************************************
 *
 * FUNCTION: void connectDev(DEV_PTR dev,int fd);
 *
 * DESCRIPTION:
 *
 * INPUTS:
 * DEV_PTR dev - structure for initializing connection.
 *
 * OUTPUTS:
 *
 * HISTORY:
 *
 *****************************************************************************/

void devConnectDev(DEV_PTR dev, int fd)
{
  int i, space;
  BOOLEAN found=FALSE;
  
  /* set the status and put into the open device array if it is open. */
  if (dev->listen & (LISTENING | TALKING)) {
    if (fd == NO_FD) {
      dev->status = RECONNECTING;
    } else {
      FD_SET(fd, &devConnections);
      FD_SET(fd, &devListenConnections);
      FD_SET(fd, &(dev->listenFds));
      FD_SET(fd, &(dev->talkFds));
      FD_SET(fd, &(dev->fds));
      open_devices[fd] = dev;
      dev->fd = fd;
      dev->status = CONNECTED;
    }
  } else {
    dev->status = CONNECTED;
  }
  
  /* make sure the device is in the array of devices */
  space = num_devices;
  for (i=0;i<=num_devices;i++) {
    if (devices[i] == dev)
      found = TRUE;
    if (devices[i] == NULL)
      space = i;
  }
  if (!found) {
    devices[space] = dev;
    num_devices = MAX(num_devices, space+1);
  }
  if(fd+1 > maxDevFd) {
    maxDevFd = fd+1;
  }
}

/*****************************************************************************
 *
 * FUNCTION: void listenAtDev(DEV_PTR dev,int fd);
 *
 * DESCRIPTION:
 *
 * INPUTS:
 * DEV_PTR dev - structure for initializing connection.
 *
 * OUTPUTS: 
 *
 * HISTORY:
 *
 *****************************************************************************/

static void listenAtDev(DEV_PTR dev, int fd)
{
  devConnectDev(dev,fd);
  FD_SET(fd,&(dev->acceptFds));
}

/*****************************************************************************
 *
 * FUNCTION: void updateConnections(DEV_PTR dev,fd_set *fds);
 *
 * DESCRIPTION:
 *
 * INPUTS:
 * DEV_PTR dev - structure for initializing connection.
 *
 * OUTPUTS: 
 *
 * HISTORY:
 *
 *****************************************************************************/

void devUpdateConnections(DEV_PTR dev,fd_set *fds, int maxFD)
{
  int i;
  
  maxFD = MAX(0,MIN(maxFD, FD_SETSIZE));
  num_devices = MAX(num_devices,maxFD);
  
  for (i=0; i<=num_devices; i++) {
    if (FD_ISSET(i,fds)) {
      if (!FD_ISSET(i,&(dev->listenFds))) {
	devConnectDev(dev,i);
      }
    } else if (FD_ISSET(i,&(dev->listenFds))) {
      open_devices[i] = NULL;
      FD_CLR(i,&(dev->listenFds));
      FD_CLR(i,&(dev->talkFds));
      FD_CLR(i,&(dev->fds));
      FD_CLR(i,&devConnections);
      FD_CLR(i,&devListenConnections);
    }
  }
}

/*****************************************************************************
 *
 * FUNCTION: BOOLEAN isDisconnected(DEV_PTR dev);
 *
 * DESCRIPTION: returns true if the device is not currently connected or 
 *              trying to reconnect.
 *
 * INPUTS:
 * DEV_PTR dev - structure for initializing connection.
 *
 * OUTPUTS:
 *
 * HISTORY:
 *
 *****************************************************************************/

BOOLEAN devIsDisconnected(DEV_PTR dev)
{
  return ((dev == NULL) || (dev->status == NOT_CONNECTED));
}

/*****************************************************************************
 *
 * FUNCTION: BOOLEAN devHasFd(DEV_PTR dev);
 *
 * DESCRIPTION: returns true if the device is currently connected.
 *
 * INPUTS:
 * DEV_PTR dev - structure for initializing connection.
 *
 * OUTPUTS:
 *
 * HISTORY:
 *
 *****************************************************************************/

BOOLEAN devHasFd(DEV_PTR dev)
{
  return ((dev == NULL) || !fd_isZero(&(dev->talkFds)));
}

/* The following is to maintain compatibility. */
BOOLEAN devIsConnected(DEV_PTR dev)
{
  return devHasFd(dev);
}

/*****************************************************************************
 *
 * FUNCTION: void disconnectDev(DEV_PTR dev,BOOLEAN isDisconnected,
 *                              BOOLEAN reconnect);
 *
 * DESCRIPTION:
 *
 * INPUTS:
 * DEV_PTR dev - structure for initializing connection.
 *
 * OUTPUTS:
 *
 * HISTORY:
 *
 *****************************************************************************/

void devDisconnectDev(DEV_PTR dev,BOOLEAN isDisconnected, BOOLEAN reconnect)
{
  int i, index;
  
  for(index=0;(devices[index] != dev) && (index < num_devices); index++);
  if (devices[index] != dev) {
    return;
  }
  
  if (!FD_ISSET(index,&devices_closing)) {
    FD_SET(index,&devices_closing);
    
    for (i=0; i<maxDevFd; i++) {
      if (dev->listen & LISTENING) {
	if (FD_ISSET(i,&(dev->listenFds))) {
	  printf("Connection closed %s %d\n", dev->devName,i);
	  /*	  if (!isDisconnected)*/
	  close(i);
	  FD_CLR(i, &(dev->listenFds));
	  FD_CLR(i, &(dev->talkFds));
	  FD_CLR(i, &(dev->fds));
	  FD_CLR(i, &devConnections);
	  FD_CLR(i, &devListenConnections);
	}
      }
      if (open_devices[i] == dev) {
	open_devices[i] = NULL;
      }
    }
    
    if(dev->disconnectHnd != NULL) {
      (* dev->disconnectHnd)(dev);
    }
    
    if (reconnect)
      dev->status = RECONNECTING;
    else
      dev->status = NOT_CONNECTED;
    
    FD_CLR(index,&devices_closing);
  }
}


/*****************************************************************************
 *
 * FUNCTION: int devConnectToSocket(DEV_PTR dev)
 *
 * DESCRIPTION:
 *
 * INPUTS:
 * DEV_PTR dev - structure for initializing connection.
 *
 * OUTPUTS: Return socket descriptor number if successful, else -1.
 *
 * HISTORY:
 *
 *****************************************************************************/
int devConnectToSocket(DEV_PTR dev)
{
  int fd;
  
  if (connectAt(dev->sim_dev.machine,dev->sim_dev.portNumber,&fd,&fd)) {
    devConnectDev(dev,fd);
    devSetCloseOnZero(dev, FALSE);
  } else {
    fd = NO_FD;
  }
  
  return fd;
}

/******************************************************************************
 *
 * FUNCTION: int startListening(port, sd)
 *
 * DESCRIPTION:
 * Opens the server port and sets it up to listen for new connections.
 *
 * INPUTS
 *  int port : the port number to listen to;
 *  int *sd : the integer to store the fd in.
 *
 * OUTPUTS:
 * Returns FALSE if there was an error detected, TRUE otherwise.
 *
 *****************************************************************************/

BOOLEAN devStartListening(int port, DEV_PTR dev)
{
  int port_id=0;
  int socket_id=0;
  
  if (listenAtPort(&port, &port_id)) {
    listenAtDev(dev,port_id);
#ifndef NO_UNIX_SOCKETS
    if(listenAtSocket(port, &socket_id)) {
      listenAtDev(dev,socket_id);
      fprintf(stderr,
	      "Opened UNIX & TCP/IP sockets to accept connections.\n");
    } else {
      fprintf(stderr,
	      "Cannot open UNIX socket, accepting TCP connection only.\n");
    }
#endif
  } else {
    devDisconnectDev(dev,TRUE,TRUE);
    return FALSE;
  }
  return TRUE;
}

/******************************************************************************
 *
 * FUNCTION: BOOLEAN serverInitialize(void)
 *
 * DESCRIPTION:
 * Initialize the server. Creates a socket, looks up the server port
 * number and binds the socket to that port number, and listens to the
 * socket. The machine name for the simulation server is assumed to be the
 * machine on which it is running and so it needs not be specified.
 *
 * OUTPUTS:
 * Returns FALSE if there was an error detected, TRUE otherwise.
 *
 *****************************************************************************/

BOOLEAN devServerInitialize(DEV_PTR dev)
{
  struct servent *sp;
  
  if (dev->sim_dev.portNumber == -1) {
    if ((sp = getservbyname(dev->devName, NULL)) == NULL) {
      fprintf(stderr,"Unable to find port number for the server %s\n",
	      dev->devName);
      return FALSE;
    }
    dev->sim_dev.portNumber = sp->s_port;
  }
  
  if (!devStartListening(dev->sim_dev.portNumber, dev)) {
    return FALSE;
  } else {
    /* now waiting for connections */
    printf("Server %s waiting for connections from remotes \n",
	   dev->devName);
  }
  return TRUE;
}


/**********************************************************/
/*                                                        */
/*  openRaw						  */	
/*                                                        */
/*  Opens passed filename and sets its device to raw mode.*/
/*  Returns file descriptor.                              */
/*                                                        */
/*  raw mode is defined as:                               */
/*  Off: echo, canonical input, extended processing,      */
/*       signals, break key, parity, 8th bit strip,       */
/*       flow control, output post processing             */
/*   On: 8 bit size                                       */
/*                                                        */
/**********************************************************/

#ifdef linux
int openRaw(const char *filename, mode_t io_flags)
{
  struct termios term_info;
  int fd;
  
  fd = open(filename,io_flags);
  if (fd == -1)
    {
      fprintf(stderr,"can't open port at 'open' in openRaw\n");
      fflush(stderr);
      /* maybe complain if TRACE is on */
      return NO_FD;
    }
  
  if(tcgetattr(fd,&term_info) <0)
    {
      fprintf(stderr,"fd is not a terminal\n");
      fflush(stderr);
      /* complain - fd is not a terminal */
      return NO_FD;
    }
  
#ifndef REDHAT_52
  /* turn off echo, canonical mode, extended processing, signals */
  term_info.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  
  /* turn off break sig, cr->nl, parity off, 8 bit strip, flow control */
  term_info.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  
  /* clear size, turn off parity bit */
  term_info.c_cflag &= ~(CSIZE | PARENB);
  
  /* set size to 8 bits */
  term_info.c_cflag |= CS8;
  
  /* turn output processing off */
  term_info.c_oflag &= ~(OPOST);
#else
  cfmakeraw(&term_info);
#endif
  
  /* Set time and bytes to read at once */
  term_info.c_cc[VTIME] = 0;
  term_info.c_cc[VMIN] = 0;
  
  if(tcsetattr(fd,TCSAFLUSH,&term_info) <0)
    {
      fprintf(stderr,"cannot set attributes for %d\n", fd);
      fflush(stderr);
      return NO_FD;
    }
  
  return fd;
}
#endif

/*****************************************************************************
 *
 * FUNCTION: int devConnectTottyRaw(DEV_PTR dev)
 *
 * DESCRIPTION:
 *
 * INPUTS:
 * DEV_PTR dev - structure for initializing connection.
 *
 * OUTPUTS: Return tty descriptor number if successful, else -1.
 *
 * HISTORY:
 *
 *****************************************************************************/

#ifdef linux
static int devConnectTottyRaw(DEV_PTR dev)
{
  struct termios terminfo;
  int fd;
  
  /*.. Open the port for both ReaDing and WRiting ..*/
  if ((fd = openRaw(dev->ttydev.ttyPort, O_RDWR)) < 0) {
    fprintf(stderr,"could not open port \n");
    fflush(stderr);
    fd = NO_FD;
    return fd;
  } 
  if (tcgetattr(fd,&terminfo) < 0) {
    fprintf(stderr,"could not get attr for port. \n");
    fflush(stderr);
    fd = NO_FD;
    return fd;
  }
  if (cfsetispeed(&terminfo,dev->ttydev.baudCode)) {
    fprintf(stderr,"could not get attr for port. \n");
    fflush(stderr);
    fd = NO_FD;
    return fd;
  }
  if (cfsetospeed(&terminfo,dev->ttydev.baudCode)) {
    fprintf(stderr,"could not get attr for port. \n");
    fflush(stderr);
    fd = NO_FD;
    return fd;
  }
  if (tcsetattr(fd,TCSANOW,&terminfo)) {
    fprintf(stderr,"could not get attr for port. \n");
    fflush(stderr);
    fd = NO_FD;
    return fd;
  }
  
  devConnectDev(dev,fd);
  devSetCloseOnZero(dev, FALSE);
  return(fd);
}
#else
static int devConnectTottyRaw(DEV_PTR dev)
{
  devSetCloseOnZero(dev, FALSE);
  return devConnectTottySimple(dev);
}
#endif 

/*****************************************************************************
 *
 * FUNCTION: int devConnectTottySimple(DEV_PTR dev)
 *
 * DESCRIPTION:
 *
 * INPUTS:
 * DEV_PTR dev - structure for initializing connection.
 *
 * OUTPUTS: Return tty descriptor number if successful, else -1.
 *
 * HISTORY:
 *
 *****************************************************************************/

int devConnectTottySimple(DEV_PTR dev)
{
  int fd;
  
  /*.. Open the port for both ReaDing and WRiting ..*/
  if ((fd = open(dev->ttydev.ttyPort, O_RDWR, 0)) < 0) {
    fprintf(stderr,"could not open port\n");
    fprintf(stderr,"in connect to simple \n");
    fflush(stderr);
    fd = NO_FD;
  } else if (ioctl(fd, TIOCNXCL, NULL) < 0) {
    fprintf(stderr, "Warning, can't set exclusive access for %s (%s) %d\n",
	    dev->devName,dev->ttydev.ttyPort,TIOCNXCL);
    fflush(stderr);
    fd = NO_FD;
  } else {
#ifndef REDHAT_52
    struct sgttyb mode;
 
    ioctl(fd, TIOCGETP, &mode);
    mode.sg_ispeed=dev->ttydev.baudCode;
    mode.sg_ospeed=dev->ttydev.baudCode;
    mode.sg_flags |= O_RAW;
    mode.sg_flags &= ~O_ECHO;
    
    if (ioctl(fd, TIOCSETP, &mode) < 0) {
      fprintf(stderr, "Error, can't set baud rate and mode for %s (%s) %ld\n",
	      dev->devName,dev->ttydev.ttyPort,(long) TIOCSETP);
    } else {
      devFlushChars(dev);
    }
#else
    struct termios term_info;

    tcgetattr(fd, &term_info);
    /* turn off echo */
    term_info.c_lflag &= ~ECHO;
    if (cfsetospeed(&term_info, dev->ttydev.baudCode) < 0 ||
	cfsetispeed(&term_info, dev->ttydev.baudCode) < 0 ||
	tcsetattr(fd, TCSANOW, &term_info) < 0) {
      fprintf(stderr, "Error, can't set baud rate and mode for %s (%s)\n",
	      dev->devName, dev->ttydev.ttyPort);
    } else {
    devFlushChars(dev);
    }
#endif
    devConnectDev(dev,fd);
    devSetCloseOnZero(dev, FALSE);
  }
  return(fd);
}

/*****************************************************************************
 *
 * FUNCTION: int devConnectToNonttySimple(DEV_PTR dev)
 *
 * DESCRIPTION: connect to non tty device using open and setting no
 * 				parameters
 *
 * INPUTS:
 * DEV_PTR dev - structure for initializing connection.
 *
 * OUTPUTS: Return tty descriptor number if successful, else -1.
 *
 * HISTORY:
 *
 *****************************************************************************/

int devConnectToNonttySimple(DEV_PTR dev)
{
  int fd;
  
  /*.. Open the port for both ReaDing and WRiting ..*/
  if ((fd = open(dev->ttydev.ttyPort, O_RDWR, 0)) < 0) {
    fprintf(stderr,"could not open port\n");
    fprintf(stderr,"in connect to simple \n");
    fflush(stderr);
    fd = NO_FD;
    } 
  else {
    devFlushChars(dev);
    devConnectDev(dev,fd);
    /*  devSetCloseOnZero(dev, FALSE);*/
    }
  return(fd);
}

/*****************************************************************************
 *
 * FUNCTION: int devConnectTotty(DEV_PTR dev)
 *
 * DESCRIPTION:
 *
 * INPUTS:
 * DEV_PTR dev - structure for initializing connection.
 *
 * OUTPUTS: Return tty descriptor number if successful, else -1.
 *
 * HISTORY:
 *
 *****************************************************************************/

int devConnectTotty(DEV_PTR dev)
{
  if (dev->ttydev.lineDiscipline == TTY_OPEN_SIMPLE) {
    fprintf(stderr,"trying to open simple\n");
	fflush(stderr);
    return(devConnectTottySimple(dev));
	}
  else if (dev->ttydev.lineDiscipline == TTY_OPEN_RAW) {
    fprintf(stderr,"trying to open raw\n");
	fflush(stderr);
    return(devConnectTottyRaw(dev));
	}
  else if (dev->ttydev.lineDiscipline == NON_TTY_OPEN_SIMPLE)
	return(devConnectToNonttySimple(dev));

  else {
    fprintf(stderr,
	    "devConnectTotty: Unknown line discipline (%d) for device %s\n",
	    dev->ttydev.lineDiscipline,
	    dev->devName);
    return(NO_FD);
  }
}


/*****************************************************************************
 *
 * FUNCTION: void printBuffer(int length, unsigned char *buf);
 *
 * DESCRIPTION:
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 * HISTORY:
 *
 *****************************************************************************/
static void printBuffer(FILE *outfile, int length, const char *buf)
{
  int i;
  
  fprintf(outfile,"[");
  for(i=0;i<length; i++) {
    switch (buf[i]) {
    case '\n':
      fprintf(outfile, "\\n");
      break;
    case '\0':
      fprintf(outfile, "\\0");
      break;
    default:
      fprintf(outfile, "%c",buf[i]);
    }
  }
  fprintf(outfile,"]\n");
  fflush(outfile);
}

/******************************************************************************
 *
 * FUNCTION: BOOLEAN writeN(DEV_PTR, buf, nChars)
 *
 * DESCRIPTION: This routine primarily calls the system function write.
 *
 * INPUTS:
 * DEV_PTR dev;
 * char *buf;
 * int nChars;
 *
 * OUTPUTS: BOOLEAN
 *
 *****************************************************************************/

BOOLEAN devWriteN(int fd, const char *buf, int nChars)
{
  int amountWritten = 0;
  DEV_PTR dev;
  
  dev = devGetDev(fd);
  if (dev == NULL) {
    fprintf(stderr, "devWriteN: Try to write to a non-devUtils fd \n");
    return 0;
  }
  
  if (dev->debug) {
    /* make sure the valid data in the buffer ends with zero */
    if (buf[nChars] == '\0') {
      if (dev->debug_file) {
	fprintf(dev->debug_file, "Dev %s (%d) sending: ",
		dev->devName, fd);
	printBuffer(dev->debug_file,nChars,buf);
      } else {
	fprintf(stderr,"Dev %s (%d) sending: ",
		dev->devName, fd);
	printBuffer(stderr,nChars,buf);
      }
    }
  }
  
  if (fd == NO_FD) {
    devDisconnectDev(dev,TRUE,TRUE);
    return FALSE;
  }
  
  while (nChars > 0) {
    PipeBroken = FALSE;
    amountWritten = write(fd, buf, nChars);
    if (PipeBroken) {
      /* the pipe was broken, just close the socket and continue */
      devDisconnectDev(dev,TRUE,TRUE);
      return FALSE;
    } else if (amountWritten < 0) {
      if (errno == EWOULDBLOCK) {
	fprintf(stderr,"\nWARNING: writeN: EWOULDBLOCK: trying again!\n");
	PAUSE_MIN_DELAY();
      } else {
	return FALSE;
      }
    } else {
      nChars -= amountWritten;
      buf += amountWritten;
    }
  }
  return TRUE;
}

/******************************************************************************
 *
 * FUNCTION: long numChars(sd)
 *
 * DESCRIPTION:
 * Find out how many characters are available to be read.
 *
 * INPUTS:
 * int sd;
 *
 * OUTPUTS:
 *
 * NOTES:
 *
 *****************************************************************************/

long devCharsAvailable(int sd)
{
  long available=0;
   
  if (ioctl(sd, FIONREAD, &available) == 0)
    return available;
  else
    return -1;
}

/******************************************************************************
 *
 * FUNCTION: int readN(DEV_PTR dev, int fd, char *buf, int nchars)
 *
 * DESCRIPTION:
 * Read nchars of data from sd into buf. Continue to read until
 * nchars have been read.  Return 0 if end of file reached and -1 if
 * there was an error.
 *
 * INPUTS:
 * DEV_PTR dev;
 * char *buf;
 * int nchars;
 *
 * OUTPUTS: int numRead.
 *
 * NOTES:
 *
 * buf is a preallocated pointer to storage equal to nchars.
 * Propagation of low level errors - failures on read/writes still an issue.
 *
 *****************************************************************************/

int devReadN(int fd, char *buf, int nchars)
{
  int amountRead=0, amountToRead;
  DEV_PTR dev;
  
  dev = devGetDev(fd);
  if (dev == NULL) {
    fprintf(stderr, "devReadN: Try to read from a non-devUtils fd \n");
    return 0;
  }
  
  amountToRead = nchars;
  for(;;){
    if (amountToRead <= 0)
      return(nchars - amountToRead);
    amountRead = read(fd, buf, amountToRead);
    if ((amountRead == 0) && (amountToRead > 0))
      { /* just got an end of file indication.
	 * close the socket and remove it from the list of active
	 * connections
	 */
	/* remove the connection from the read mask */
	devDisconnectDev(dev,TRUE,TRUE);
	return 0;
    } else if (amountRead < 0)
      /* some other problem.  Try to continue?
       */
      return amountRead;
    amountToRead -= amountRead;
    buf += amountRead;
  }
}

/******************************************************************************
 *
 * FUNCTION: void flushChars(DEV_PTR dev)
 *
 * DESCRIPTION:
 * Flush any characters from the input of the file descriptor.
 *
 * INPUTS:
 * DEV_PTR dev;
 *
 * OUTPUTS:
 *
 * NOTES:
 *
 * This could be done using FIONFLUSH for files and tty devices, but not for
 * sockets.
 *
 *****************************************************************************/

void devFlushChars(DEV_PTR dev)
{
  long available=0;
  char inbuf[DEFAULT_LINE_LENGTH];
  int i;
  
  for (i=0; i<FD_SETSIZE; i++) {
    if (FD_ISSET(i,&(dev->listenFds))) {
      while ((available=devCharsAvailable(i)) > 0)
	devReadN(i, inbuf, MIN(available,DEFAULT_LINE_LENGTH));
    }
  }
}

/*****************************************************************************
 *
 * FUNCTION: int devConnectToSimulator (DEV_PTR dev);
 *
 * DESCRIPTION:
 *
 * INPUTS:
 *
 * OUTPUTS: Return socket descriptor number if successful, else -1.
 *
 * HISTORY:
 *
 *****************************************************************************/

int devConnectToSimulator(DEV_PTR dev)
{
  int sd;
  char inbuf[DEFAULT_LINE_LENGTH+1];
  long chars_available=0;
  fd_set readSet;
  int result=0;
  int stat;
  
  fprintf(stderr, "Trying to open simulated device %s\n",dev->devName);
  dev->listen = LISTENING | TALKING;
  sd = devConnectToSocket(dev);
  
  if (sd != -1) {
    /* write the device name to the socket to allow the simulator
     * to know what device we wanted to open.
     */
    
    fprintf(stderr, "Opened socket to simulated device %s\n",dev->devName);
    
    do {
      /* wait for the name of the simulator */
      FD_ZERO(&readSet);
      FD_SET(sd,&readSet);
      do {
	stat = select(FD_SETSIZE, &readSet, NULL, NULL, &MAX_CYCLE_TIME);
	if (stat == 0) devProcessDevices(NULL);
      } while (stat < 0 && errno == EINTR);
      
      if (stat < 0 ) {
	fprintf(stderr,"devConnectToSimulator: Select failed %d\n",errno);
	return -1;
      }
      
      chars_available = devCharsAvailable(sd);
      if (chars_available == -1)
	fprintf(stderr,"POLL ERROR on socket\n");
    } while (chars_available <= 0);
    
    bzero(inbuf,DEFAULT_LINE_LENGTH+1);
    result = devReadN(sd, inbuf,MIN(chars_available,DEFAULT_LINE_LENGTH));
    
    if (result <= 0)
      { /* cound not read the socket */
	return -1;
      }
    
    fprintf(stderr, "Connected to server %s\n",inbuf);
    
    devSetCloseOnZero(dev, FALSE);
    devWriteN(sd,dev->devName,strlen(dev->devName));
    fprintf(stderr, "Opened simulated device %s\n",dev->devName);
    bzero(inbuf,DEFAULT_LINE_LENGTH+1);
    devReadN(sd,inbuf,strlen(dev->devName));
  }
  
  return sd;
}

/******************************************************************************
 *
 * FUNCTION: void setTimeout (DEV_PTR dev, int seconds)
 *
 *
 * DESCRIPTION:
 * Set a timeout for some device interface.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 *****************************************************************************/

void devSetAlarm(DEV_PTR dev, struct timeval *timeoutTime,
		 Handler timeoutHnd, void *data)
{
  /* only make sense if there is a handler */
  if (timeoutHnd != NULL) {
    dev->timeOutTime = *timeoutTime;
    dev->timeoutHnd.handler = timeoutHnd;
    dev->timeoutHnd.client_data = data;
    dev->timeoutHnd.remove = FALSE;
  }
}

/******************************************************************************
 *
 * FUNCTION: void setTimer(DEV_PTR dev, int seconds)
 * 
 *
 * DESCRIPTION: 
 * Set a Alarm for some device interface.
 *
 * INPUTS:
 *
 * OUTPUTS: 
 *
 *****************************************************************************/

void devSetTimer(DEV_PTR dev, struct timeval *delay,
		 Handler timeoutHnd, void *data)
{
  /* only make sense if there is a handler */
  if ((dev != NULL) && (timeoutHnd != NULL)) {
    gettimeofday (&(dev->timeOutTime), NULL);
    addTime(&Now, delay, &(dev->timeOutTime));
    dev->timeoutHnd.handler = timeoutHnd;
    dev->timeoutHnd.client_data = data;
    dev->timeoutHnd.remove = FALSE;
  }
}

/******************************************************************************
 *
 * FUNCTION: void cancelTimeout (DEV_PTR dev)
 *
 *
 * DESCRIPTION:
 * Cancel a timeout for some device interface.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 *****************************************************************************/

void devCancelTimeout (DEV_PTR dev)
{
  dev->timeOutTime.tv_sec = TIME_MAX;
  dev->timeOutTime.tv_usec = 0;
}

/******************************************************************************
 *
 * FUNCTION: void setMaxCycleTime(secs, usecs)
 *
 * DESCRIPTION:
 * Set the maximum timeout for the select within the function ProcessDevices
 *
 * INPUTS: int secs; max timeout in seconds
 *         int usecs; max timeout in micro-seconds
 *
 * OUTPUTS:
 *
 *****************************************************************************/

void devSetMaxCycleTime (int secs, int usecs)
{
  MAX_CYCLE_TIME.tv_sec = secs;
  MAX_CYCLE_TIME.tv_usec = usecs;
}

/******************************************************************************
 *
 * FUNCTION: void devStartPolling(DEV_PTR dev,
 *                                struct timeval *interval,
 *                                Handler handler)
 *
 * DESCRIPTION:
 * Start polling the device by calling the given hander every interval
 * milliseconds.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 *****************************************************************************/

void devStartPolling(DEV_PTR dev,
		     struct timeval *interval,
		     Handler handler, void *data)
{
  dev->pollInterval = *interval;
  addTime(&Now, &(dev->pollInterval), &(dev->pollTime));
  dev->pollHnd.handler = handler;
  dev->pollHnd.client_data = data;
  dev->pollHnd.remove = FALSE;
}

/******************************************************************************
 *
 * FUNCTION: void devStopPolling(DEV_PTR dev)
 *
 * DESCRIPTION:
 * Stop polling the device by calling the given hander every interval
 * milliseconds.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 *****************************************************************************/

void devStopPolling(DEV_PTR dev)
{
  dev->pollInterval.tv_sec = TIME_MAX;
  dev->pollInterval.tv_usec = 0;
  dev->pollTime.tv_sec = TIME_MAX;
  dev->pollTime.tv_usec = 0;
  dev->pollHnd.handler = NULL;
  dev->pollHnd.client_data = NULL;
}

/******************************************************************************
 *
 * FUNCTION: BOOLEAN devIsPolling(DEV_PTR dev)
 *
 * DESCRIPTION:
 * Returns true if the device is polling.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 *****************************************************************************/

BOOLEAN devIsPolling(DEV_PTR dev)
{
  if (dev->pollInterval.tv_sec == TIME_MAX)
    return FALSE;
  else
    return TRUE;
}

/******************************************************************************
 *
 * FUNCTION: void devMainLoop(void)
 *
 * DESCRIPTION:
 * This is the main loop for collecting and processing input from all the
 * devices.  This routine should never return.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 *****************************************************************************/

void devMainLoop(void)
{
  devInit();
  for(;;)
    devProcessDevices(NULL);
}

/******************************************************************************
 *
 * FUNCTION: void ProcessDevices(struct timeval *maxTimeout)
 *
 * DESCRIPTION:
 * Loop listening to all the open connections.  Call the device output
 * handling routines and timeout routines as needed.  The parameter points to
 * a timeval struct that gives the maximum amount of time to wait for a
 * message or timeout or polling event.  A NULL pointer defaults to
 * MAX_CYCLE_TIME (10 seconds).
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 *****************************************************************************/

void devProcessDevices(struct timeval *maxTimeout)
{
  static struct timeval last_reconnect = {0,0};
  static fd_set readMask;
  
  int i, numReady=0, handlersInvoked;
  struct timeval timeout;
  long chars_available=0;
  
  /* set the current time */
  gettimeofday (&Now, NULL);
  
  /* set & check timeouts */
  timeout.tv_sec = TIME_MAX;
  timeout.tv_usec = 0;
  handlersInvoked = FALSE;
  
  for(i=0; i<=num_devices; i++){
    if (devices[i] != NULL) {
      if (lessTime(&(devices[i]->timeOutTime),&Now)) {
	devCancelTimeout(devices[i]);
	CallHandler(&(devices[i]->timeoutHnd),&Now);
	handlersInvoked = TRUE;
      }
      if (lessTime(&(devices[i]->pollTime),&Now)) {
	CallHandler(&(devices[i]->pollHnd),&Now);
	handlersInvoked = TRUE;
	/* set next time to poll, unless polling has been cancelled */
	if (devices[i]->pollHnd.handler) {
	  addTime(&Now, &(devices[i]->pollInterval), &(devices[i]->pollTime));
	}
      }
      if (lessTime(&(devices[i]->timeOutTime),&timeout)) {
	timeout = devices[i]->timeOutTime;
      }
      if (lessTime(&(devices[i]->pollTime),&timeout)) {
	timeout = devices[i]->pollTime;
      }
    }
  }
  
  /* Significant time may have passed if handlers were called -- reset "now" */
  if (handlersInvoked) gettimeofday (&Now, NULL);
  
  if (fd_isZero(&readMask)) {
    bcopy((char *)&devListenConnections,(char *)&readMask, sizeof(fd_set));
    
    if(lessTime(&Now,&timeout))
      {
	/* have time to do a select */
	subTime(&timeout,&Now);
	if (maxTimeout != NULL) {
	  if(lessTime(maxTimeout, &timeout))
	    timeout = *maxTimeout;
	} else {
	  if(lessTime(&MAX_CYCLE_TIME, &timeout))
	    timeout = MAX_CYCLE_TIME;
	}
      }
    else
      timeout = ZERO_TIMEOUT;
    errno = 0;
#if defined(TRACE_LISTENING)
    { 
      static fd_set lastReadMask;
      if (! fd_equal(&readMask, &lastReadMask)) {
	lastReadMask = readMask;
	fprintf(stderr, "Listening to :");
	for(i=0; i<maxDevFd; i++) {
	  if (FD_ISSET(i,&readMask)) {
	    fprintf(stderr, " %d",i);
	  }
	}
	fprintf(stderr, "\n");
      }
    }
#endif
    numReady = select(FD_SETSIZE, &readMask, NULL, NULL, &timeout);
  } else {
    numReady = 1;
  }
  
  if (numReady < 0) {
#ifdef _WINSOCK_
    if (!(numReady == SOCKET_ERROR && WSAGetLastError() == WSAEINTR))
#else
    if (!(errno == EINTR))
#endif
      fprintf(stderr, "Error on select %d\n", errno);
    FD_ZERO(&readMask);
  } else if (numReady > 0) {
    for(i=0; i<maxDevFd; i++) {
      if (FD_ISSET(i,&readMask)) {
	if (FD_ISSET(i,&devListenConnections)) {
	  FD_CLR(i,&readMask);
#if defined(TRACE_LISTENING)
	  fprintf(stderr, "Receiving input on %d chars avail %ld\n",
		  i, devCharsAvailable(i));
#endif
	  if (open_devices[i] &&
	      (open_devices[i]->listen & ACCEPTING) &&
	      (FD_ISSET(i,&(open_devices[i]->acceptFds))))  {
	    (* (open_devices[i]->outputHnd))(i, chars_available);
	  } else {
	    chars_available = devCharsAvailable(i);
	    if (open_devices[i]->ttydev.lineDiscipline == NON_TTY_OPEN_SIMPLE) {
	      (* (open_devices[i]->outputHnd))(i, chars_available);
	    } else
	      if (chars_available <= 0) {
		/* possible problem ??? */
		/* A zero length message may indicate the socket is closed */
		/* Unless it is stdin and was in the background.  */
		if (i == stdin_fd) {
		  signal(SIGTTIN, devSignal);
		  inBackground = FALSE;
		} else {
		  if (open_devices[i]->closeOnZero)
		    devDisconnectDev(open_devices[i],FALSE,TRUE);
		}
	      } else {
		if (inBackground && (i == stdin_fd)) {
		  if (chars_available >1) {
		    inBackground = FALSE;
		    fprintf(stderr, "Now in forground \n");
		    signal(SIGTTIN, devSignal);
		    (* (open_devices[i]->outputHnd))(i, chars_available);
		  }
		} else {
		  (* (open_devices[i]->outputHnd))(i, chars_available);
		}
	      }
	  } 
	} else {
	  fprintf(stderr, "Clearing extraneous input on device %d (%s)\n",
		  i, open_devices[i] ? open_devices[i]->devName : "unknown");
	  FD_CLR(i, &readMask);
	}
      }
    }
  }
  
  if (Now.tv_sec - last_reconnect.tv_sec > RECONNECT_INTERVAL) {
    last_reconnect = Now;
    for(i=0; i<=num_devices; i++) {
      if ((devices[i] != NULL) && (devices[i]->status == RECONNECTING)) {
	if(devices[i]->reconnectHnd != NULL) {
	  (* devices[i]->reconnectHnd)(devices[i]);
	} else {
	  devices[i]->status = NOT_CONNECTED;
	}
      }
    }
  }
  if (devicesToFree > 0) {
    devReallyFreeDevs();
  }
  /* set the current time */
  gettimeofday (&Now, 0);
}

/*****************************************************************************
 *
 * FUNCTION: LINE_BUFFER_PTR createLineBuffer(int lineLength, char delimChar,
 *				 void (*processRoutine)(char *))
 *
 * DESCRIPTION: Creates a buffer data structure that is used to collect
 * input until one of the delimit characters is read.  The processing
 * routine is then called.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 * HISTORY:
 *
 *****************************************************************************/

DEV_LINE_BUFFER_PTR devCreateLineBuffer(int lineLimit, char *delimChar,
					void (*processRoutine)(char *, void *),
					BOOLEAN replaceDelimit,
					BOOLEAN partialLines,
					void *clientData)
{
  int length;
  DEV_LINE_BUFFER_PTR lineBuffer=NULL;
  
  if ((delimChar == NULL) && (replaceDelimit != FALSE)) {
    fprintf(stderr, 
	    "devCreateLineBuffer: can't replace a null delimit character\n");
    replaceDelimit = FALSE;
  }
  
  length = lineLimit*2;
  lineBuffer = (DEV_LINE_BUFFER_PTR)
    calloc((size_t) sizeof(LINE_BUFFER_TYPE),1);
  carmen_test_alloc(lineBuffer);
  lineBuffer->buffer = (char *)calloc((size_t) length+1,(int)1);
  carmen_test_alloc(lineBuffer->buffer);
  bzero(lineBuffer->buffer, length+1);
  lineBuffer->length = length;
  lineBuffer->nextChar = 0;
  lineBuffer->delimChar = delimChar;
  lineBuffer->lineLimit = lineLimit;
  lineBuffer->processRoutine = processRoutine;
  lineBuffer->clientData = clientData;
  lineBuffer->replaceDelimit = replaceDelimit;
  if (replaceDelimit)
    lineBuffer->string = lineBuffer->buffer;
  else {
    lineBuffer->string = (char *)calloc((size_t) (lineLimit+1),1);
    carmen_test_alloc(lineBuffer->string);
    bzero(lineBuffer->string, lineLimit+1);
  }
  lineBuffer->partialLines = partialLines;
  
  return lineBuffer;
}

/*****************************************************************************
 *
 * FUNCTION: void processOutput (DEV_PTR device, 
 *                               DEV_LINE_BUFFER_PTR lineBuffer,
 *                               int chars_available)
 *
 * DESCRIPTION:
 *  Collects characters from the device into the line buffer.
 *  Calls the processRoutine if the delimChar is found, with the delimChar
 *  character replaced by \0, to indicate end of line.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 * HISTORY:
 *
 *****************************************************************************/

void devProcessOutput(DEV_PTR device, int fd, DEV_LINE_BUFFER_PTR lineBuffer, 
		      int chars_available)
{
  char *lineEnd=NULL, *bufferEnd=NULL;
  int numRead=0, remainChar=0;
  
  if (device->debug) {
    if (device->debug_file) {
      fprintf(device->debug_file, "Dev %s Preloaded: ",
	      device->devName);
      printBuffer(device->debug_file,lineBuffer->nextChar, lineBuffer->buffer);
    } else {
      fprintf(stderr, "Dev %s Preloaded: ", device->devName);
      printBuffer(stderr,lineBuffer->nextChar, lineBuffer->buffer);
    }
  }
  while (chars_available > 0) {
    /* read in the output */
    numRead = devReadN(fd, &(lineBuffer->buffer[lineBuffer->nextChar]),
		       MIN(chars_available,
			   (lineBuffer->length - lineBuffer->nextChar)));
    if (device->debug) {
      if (device->debug_file) {
	fprintf(device->debug_file, "Dev %s Read (%d): %d ", 
		device->devName, fd, numRead);
	printBuffer(device->debug_file,numRead,
		    &(lineBuffer->buffer[lineBuffer->nextChar]));
      } else {
	fprintf(stderr, "Dev %s Read (%d): %d ",
		device->devName, fd, numRead);
	printBuffer(stderr,numRead,
		    &(lineBuffer->buffer[lineBuffer->nextChar]));
      }
    }
    if (numRead == 0) {
      /* Handle error here. The port is closed */
    } else {
      lineBuffer->nextChar += numRead;
      /*   bufferEnd = &(lineBuffer->buffer[MIN(numRead,lineBuffer->lineLimit-1)]);*/
      bufferEnd = &(lineBuffer->buffer[lineBuffer->nextChar]);
      if (lineBuffer->delimChar != NULL) {
	for(lineEnd = lineBuffer->buffer;
	    ((lineEnd != bufferEnd) &&
	     (*lineEnd != *(lineBuffer->delimChar)));
	    lineEnd++);
	if (lineEnd == bufferEnd) {
	  if (lineBuffer->nextChar < lineBuffer->lineLimit){
	    lineEnd = NULL;
	  } else {
	    lineEnd -= 1;
	  }
	}
      } else {
	/* just check the length. */
	if (lineBuffer->nextChar == lineBuffer->lineLimit)
	  lineEnd = &(lineBuffer->buffer[lineBuffer->nextChar-1]);
	else
	  lineEnd = NULL;
      }
      /* lineEnd = strpbrk(lineBuffer->buffer, lineBuffer->delimSet); */
      if ((lineEnd == NULL) && 
	  (lineBuffer->partialLines) &&
	  (lineBuffer->nextChar > 0) &&
	  (devCharsAvailable(fd) == 0)){
	lineEnd = &(lineBuffer->buffer[lineBuffer->nextChar-1]);
      }
      
      while (lineEnd != NULL) {
	if ((lineEnd - lineBuffer->buffer) > lineBuffer->lineLimit)
	  lineEnd = lineBuffer->buffer + lineBuffer->lineLimit;
	if ((lineBuffer->delimChar != NULL) &&
	    (*lineEnd == *(lineBuffer->delimChar))) {
	  if (lineBuffer->replaceDelimit) {
	    *lineEnd = '\0';
	  } else {
	    bcopy(lineBuffer->buffer, lineBuffer->string,
		  (int)lineEnd +1 - (int)lineBuffer->buffer);
	    lineBuffer->string[lineEnd +1 - lineBuffer->buffer] = '\0';
	  }
	} else {
	  bcopy(lineBuffer->buffer, lineBuffer->string,
		lineBuffer->lineLimit);
	  lineBuffer->string[lineBuffer->lineLimit] = '\0';
	}
	if (device->debug) {
	  if (device->debug_file) {
	    fprintf(device->debug_file, "Dev %s Received: ",
		    device->devName);
	    printBuffer(device->debug_file,strlen(lineBuffer->string)+1
			,lineBuffer->string);
	  } else {
	    fprintf(stderr, "Dev %s Received: ", device->devName);
	    printBuffer(stderr,strlen(lineBuffer->string)+1,
			lineBuffer->string);
	  }
	}
	(lineBuffer->processRoutine)(lineBuffer->string,
				     lineBuffer->clientData);
	remainChar = lineBuffer->nextChar - (lineEnd + 1 - lineBuffer->buffer);
	lineBuffer->nextChar = remainChar;
	bufferEnd = &(lineBuffer->buffer[MIN(remainChar,
					     lineBuffer->lineLimit)]);
	if(remainChar>0) {
	  bcopy(&(lineEnd[1]), lineBuffer->buffer, remainChar);
	  bzero(&(lineBuffer->buffer[remainChar]),
		lineBuffer->length - remainChar);
	  if (lineBuffer->delimChar != NULL) {
	    for(lineEnd = lineBuffer->buffer;
		((lineEnd != bufferEnd) &&
		 (*lineEnd != *(lineBuffer->delimChar)));
		lineEnd++);
	    if (lineEnd == bufferEnd) {
	      if (lineBuffer->nextChar < lineBuffer->lineLimit){
		lineEnd = NULL;
	      } else {
		lineEnd -= 1;
	      }
	    }
	  } else {
	    /* just check the length. */
	    if (lineBuffer->nextChar == lineBuffer->lineLimit)
	      lineEnd = &(lineBuffer->buffer[lineBuffer->nextChar-1]);
	    else
	      lineEnd = NULL;
	  }
	  /* lineEnd = strpbrk(lineBuffer->buffer, lineBuffer->delimSet); */
	  if ((lineEnd == NULL) && 
	      (lineBuffer->partialLines) &&
	      (lineBuffer->nextChar > 0) &&
	      (devCharsAvailable(fd) == 0)){
	    lineEnd = &(lineBuffer->buffer[lineBuffer->nextChar-1]);
	  }
	} else {
	  lineBuffer->nextChar = 0;
	  bzero(lineBuffer->buffer,lineBuffer->length+1);
	  lineEnd = NULL;
	}
      }
      if (device->debug) {
	if (device->debug_file) {
	  fprintf(device->debug_file, "Dev %s Remaining: ",
		  device->devName);
	  printBuffer(device->debug_file,remainChar, lineBuffer->buffer);
	} else {
	  fprintf(stderr, "Dev %s Remaining: ", device->devName);
	  printBuffer(stderr,remainChar, lineBuffer->buffer);
	}
      }
      chars_available -= numRead;
    }
  }
  if (device->debug) {
    if (device->debug_file) {
      fprintf(device->debug_file, "Dev %s Postloaded: ",
	      device->devName);
      printBuffer(device->debug_file,lineBuffer->nextChar, lineBuffer->buffer);
    } else {
      fprintf(stderr, "Dev %s Postloaded: ", device->devName);
      printBuffer(stderr,lineBuffer->nextChar, lineBuffer->buffer);
    }
  }
}


/*****************************************************************************
 *
 * FUNCTION: void devSetLineBufferData(DEV_LINE_BUFFER_PTR lineBuffer,
 *                                     void *clientData);
 *
 * DESCRIPTION: routine set the clientData for a line buffer.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 * HISTORY:
 *
 *****************************************************************************/

void devSetLineBufferData(DEV_LINE_BUFFER_PTR lineBuffer, void *clientData)
{
  lineBuffer->clientData = clientData;
}


/*****************************************************************************
 *
 * FUNCTION: void *devGetLineBufferData(DEV_LINE_BUFFER_PTR lineBuffer)
 *
 * DESCRIPTION: routine set the clientData for a line buffer.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 * HISTORY:
 *
 *****************************************************************************/

void *devGetLineBufferData(DEV_LINE_BUFFER_PTR lineBuffer)
{
  return lineBuffer->clientData;
}


/*****************************************************************************
 *
 * FUNCTION: void devFreeLineBuffer(DEV_LINE_BUFFER_PTR lineBuf)
 *
 * DESCRIPTION: Frees memory associated with a line buffer.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 * HISTORY:
 *
 *****************************************************************************/

void devFreeLineBuffer(DEV_LINE_BUFFER_PTR lineBuffer)
{
  if (lineBuffer == NULL) return;
  if (lineBuffer->buffer != lineBuffer->string) {
    free(lineBuffer->string);
    lineBuffer->string = NULL;
  }
  free(lineBuffer->buffer);
  lineBuffer->buffer = NULL;
  free(lineBuffer);
}

/*****************************************************************************
 *
 * FUNCTION: BOOLEAN WaitForResponse(DEV_PTR dev, int *doneFlag, long time_out)
 *
 * DESCRIPTION:
 * The following function allows to wait within an output handler
 * and keep on reading the devices.
 *
 * IMPORTANT!!!: The code of the output handler that is waiting must
 * be reentrant for the function to work. Otherwise, the device corresponding
 * to that output handler won't process properly the requests until
 * the waiting ends.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 * HISTORY:
 *
 *****************************************************************************/

BOOLEAN devWaitForResponse(DEV_PTR dev, int *doneFlag, long time_out)
{
  static struct timeval global_time_out = {0,0};
  
  if (time_out != 0) {
    Now.tv_sec += time_out;
    if (global_time_out.tv_sec == 0)
      global_time_out = Now;
    else {
      if (lessTime(&global_time_out, &Now))
	global_time_out = Now;
    }
  }
  *doneFlag = WAITING;
  waiting++;
  while (*doneFlag == WAITING) {
    devProcessDevices(NULL);
    if (global_time_out.tv_sec != 0) {
      if (lessTime(&global_time_out, &Now)) {
	printf ("Time out\n");
	waiting--;
	if (waiting == 0)
	  global_time_out.tv_sec = 0;
	return(FALSE);
      }
    }
  }
  waiting--;
  if (dev->debug) fprintf(stderr, "Exiting from waiting\n");
  if (waiting == 0)
    global_time_out.tv_sec = 0;
  return(TRUE);
}

/******************************************************************************
 *
 * FUNCTION: int getDevFD(DEV_PTR dev)
 *
 * DESCRIPTION:
 * Returns a file index associated with the device.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 *****************************************************************************/

int devGetFd(DEV_PTR dev)
{
  int fd;
  
  if (dev == NULL) return NO_FD;
  for(fd=0;!FD_ISSET(fd,&(dev->talkFds)); fd++);
  return fd;
}

/******************************************************************************
 *
 * FUNCTION: int getDevFD(DEV_PTR dev)
 *
 * DESCRIPTION:
 * Returns a file index associated with the device.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 *****************************************************************************/

DEV_PTR devGetDev(int fd)
{
  if ((fd <0) || (fd > FD_SETSIZE)) return NULL;
  return open_devices[fd];
}


/******************************************************************************
 *
 * FUNCTION: void devConnectDevReceiveOnly(DEV_PTR dev, int fd)
 *
 * DESCRIPTION:
 * Like devConnectDev, but the device is setup
 * only to listen to the fd and not to send anything.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 *****************************************************************************/

void devConnectDevReceiveOnly(DEV_PTR dev, int fd)
{
  int oldFd = dev->fd;
  devConnectDev(dev,fd);
  FD_CLR(fd, &(dev->talkFds));
  dev->fd = oldFd;
}

/******************************************************************************
 *
 * FUNCTION: void devConnectDevSendOnly(DEV_PTR dev, int fd)
 *
 * DESCRIPTION:
 * Like devConnectDev, but the device is setup
 * only to talk to the fd and not to listen.
 *
 * INPUTS:
 *
 * OUTPUTS:
 *
 *****************************************************************************/

void devConnectDevSendOnly(DEV_PTR dev, int fd)
{
  devConnectDev(dev,fd);
  FD_CLR(fd, &(dev->listenFds));
  FD_CLR(fd, &devListenConnections);
}
