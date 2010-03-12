/*****************************************************************************
 * PROJECT: TCA
 *
 * (c) Copyright Richard Goodwin, 1995. All rights reserved.
 *
 * FILE: devUtils.h
 *
 * ABSTRACT:
 * 
 * This file provides a set for routines for use with device interfaces.
 * A device interface is a set of routines that provides high level access
 * to a device through its device driver.  All device interfaces are required
 * to be able to connect to a socket rather than the device driver. The routine
 * provided in the file are to help with connecting to a simulator.
 *
 *****************************************************************************/

#ifndef DEVUTIL_LOADED
#define DEVUTIL_LOADED

/* 
 * Other headers you really need to use devUtils.
 */

#ifndef HANDLERS_H
#include "handlers.h"
#endif

#ifdef linux
#ifndef REDHAT_6
int ioctl(int fd, int request, void *arg);
#else
//extern int ioctl(int __fd, unsigned long int __request, ...);
#define fds_bits __fds_bits
#endif
#endif

/*****************************************************************************
 * CONSTANTS
 *****************************************************************************/

/* declare some constants that should be in a system header file. */
#define stdin_fd  0
#define stdout_fd 1
#define stderr_fd 2

/* define any device constants */

#define DEFAULT_LINE_LENGTH 80

/* defaults for communication */

#define DEV_DEFAULT_BAUD    B9600
#define DEV_DEFAULT_SIM_PORT    1621

/* Type declarations */

/*****************************************************************************
 * TYPES
 *****************************************************************************/

/*****************************************************************************
 * TYPE : DEV_TYPE, DEV_PTR
 *
 * Usage : This structure is used by devUtils to keep track of information 
 * about a particular device.  It should really be a private structure.  The 
 * macros and functions defined below should be used to access it.  It is 
 * currently in this file to maintain backward compatibility, but that will
 * probably change with the next release.
 *****************************************************************************/

/* A device output handler receives the socket descriptor and the number
 * of bytes to be read.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _dev *DEV_PTR;

typedef void (*DEVICE_OUTPUT_HND)(int, long );

typedef void (*DEVICE_HANDLER)(DEV_PTR);

/* 
 * Status of the device
 */

typedef enum { NOT_CONNECTED=0, CONNECTED=1, RECONNECTING=2} DEV_STATUS_TYPE;

/* 
 * Type of fd connection.  You "or" these constants to get an int.
 * The most common one is LISTENING | TALKING for connections with both input
 * and output. 
 */

typedef enum { 
  NOT_LISTENING =0, /* device is not listening for input on any open 
		       file descriptor or sending any output. */
  LISTENING =1,     /* Listen for input. */
  ACCEPTING =2,     /* Accept connections, used for servers and simulators. */
  TALKING =4,       /* Allow writing to the fd */
} LISTENING_TYPE;

/* 
 * Type of line discipline to use on a tty connection.  This controls how 
 * the settings on the tty device are set.  
 */

typedef enum { 
  TTY_OPEN_SIMPLE =0, /* Just open the device, don't set any parameters. */
  TTY_OPEN_RAW =1,    /* Ensure that the line editing mode and other 
			 features are turned off. */
  NON_TTY_OPEN_SIMPLE =2,/* open a non tty device such as the access bus */
} TTY_OPEN_TYPE;

/* 
 * Flag values used when waiting for a reply from a device.
 */

#define WAITING -1 /* used to indicate a handler is waiting */
#define NOT_WAITING 0 /* used to indicate a handler is waiting */

/* 
 * Initialization constants for file descriptors and fd_sets.
 */

#define NO_FD -1
#define NO_FDS {{0,0,0,0,0,0,0,0}}

/* 
 * The structure used to describe a device.
 * Should use macros below to access it. 
 */

#ifndef DEV_FUNCTIONAL_ONLY
typedef struct _dev {
  DEV_STATUS_TYPE status;
  BOOLEAN use_simulator;
  struct {
    const char *machine;
    int portNumber;
  } sim_dev;
  struct {
    const char *ttyPort;
    int baudCode;
    TTY_OPEN_TYPE lineDiscipline;
  } ttydev;
  const char *devName;
  fd_set acceptFds;
  fd_set listenFds;
  fd_set talkFds;
  fd_set fds;
  int fd;
  unsigned int listen;
  BOOLEAN debug;
  BOOLEAN closeOnZero;
  FILE *debug_file;
  fd_set *readMask;
  DEVICE_OUTPUT_HND outputHnd;
  _HandlerEntry timeoutHnd;
  struct timeval timeOutTime;
  _HandlerEntry pollHnd;
  struct timeval pollInterval;
  struct timeval pollTime;
  DEVICE_HANDLER sigHnd;
  DEVICE_HANDLER disconnectHnd;
  DEVICE_HANDLER reconnectHnd;
  void *clientData;
} DEV_TYPE;

#endif

/* 
 * Device parameters that can be set at initialization.
 * Used in devSetParameters and devSetParameter.
 */

typedef enum 
{ 
  DEV_END_LIST=0,      /* Alternative to using NULL. */	
  DEV_USE_SIMULATOR=1, /* If set to true, connect to the simulator. */	
  DEV_SIM_MACHINE=2,   /* Name of machine running the simulator. */
  DEV_PORT_NUMBER=3,   /* TCP/IP port for the simulator. */
  DEV_TTY_PORT=4,      /* TTY device name, a string. (ie /dev/ttya) */
  DEV_BAUD_CODE=5,     /* Baud rate for the device. B9600 is the default. */
  DEV_LISTENING=6,     /* Setting for the file descriptors. */
  DEV_DEBUG=7,         /* If non-zero, print debug information. */
  DEV_DEBUG_FILE=8,    /* File descriptor for writing debug information,
			  defaults to stderr. */
  DEV_OUTPUTHND=9,     /* handler to call when the device has output. */
  DEV_SIGNALHND=10,    /* Handler to call when a signal (interrupt) is 
			  received. */
  DEV_DISCONNECTHND=11,/* Handler to call when the connection to the 
			  device closes. */
  DEV_RECONNECTHND=12, /* Handler to call to try to reestablish the
			  connection to the device. */
  DEV_CLIENT_DATA=13,  /* Pointer to user defined client data for the device.
			*/
  DEV_CLOSE_ON_ZERO=14,/* Close the fd if the received length is zero.
			* usually indicates some type of "signal" on a pipe.
			*/
  DEV_LINE_DISCIPLINE=15,/* Either raw or simple. TTY_OPEN_TYPE;*/
} DEV_PARAM_TYPE;

/*****************************************************************************
 * Interface routines and macros
 *****************************************************************************/

/*****************************************************************************
 * Main devUtils interface
 *
 * These are the functions you should call to interact with devUtils.
 * 
 *****************************************************************************/

/* The main devUtils routines. */

/* 
 * devCreateDev: Create a device data structure and initialize it.
 * The first parameter is a string that gives the name of the device.
 * This is followed by a variable length list of parameter name, 
 * parameter value pairs.  The list is terminated with a NULL parameter.
 * For example myDev = devCreateDev("myDev" DEV_OUTPUTHND, myOutputHnd, NULL);
 * Unset parameters are the to their defaults, usually NULL.
 */

DEV_PTR devCreateDev(const char *name, ...);

/*
 * devCreateTTYDev: Same as devCreateDev, except that the defaults are set
 * for a device that opens a tty port for I/O.
 */

DEV_PTR devCreateTTYDev(const char *name, ...);

/* 
 * devFreeDev: Disconnects the device and frees the memory associated with a 
 * device.  It can be called from the disconnectHnd.  The pointer is invalid
 * after the call.
 */

void devFreeDev(DEV_PTR dev);

/* 
 * devSetParameters: Set parameters for a device.
 * The first parameter is the device data structure.
 * This is followed by a variable length list of parameter name, 
 * parameter value pairs.  The list is terminated with a NULL parameter.
 * For example devSetParameters(myDev DEV_OUTPUTHND, myOutputHnd, NULL);
 * Unset parameters are left unchanged.
 */

void devSetParameters(DEV_PTR device, ...);

/* 
 * devSetParameter: Set a parameter for a device.
 * The first parameter is the device data structure.
 * This is followed by a parameter name, 
 * parameter value pair.
 * For example devSetParameter(myDev DEV_OUTPUTHND, myOutputHnd);
 * Unset parameters are left unchanged.
 */

void devSetParameter(DEV_PTR device, DEV_PARAM_TYPE param, void *value);

/* 
 * functions for setting the parameter values one at at time.
 */

void devSetUseSimulator(DEV_PTR device, BOOLEAN useSim);
void devSetSimMachine(DEV_PTR device, const char *machine);
void devSetPort(DEV_PTR device, int port);
void devSetBaudRate(DEV_PTR device, int baudRate);
void devSetListening(DEV_PTR device, unsigned int listen);
void devSetDebug(DEV_PTR device, BOOLEAN debug);
void devSetDebugFile(DEV_PTR device, FILE *);
void devSetOutputHnd(DEV_PTR device, DEVICE_OUTPUT_HND hnd);
void devSetSignalHnd(DEV_PTR device, void (* hnd)(DEV_PTR));
void devSetDisconnectHnd(DEV_PTR device, void (* hnd)(DEV_PTR));
void devSetReconnectHnd(DEV_PTR device, void (* hnd)(DEV_PTR));
void devSetClientData(DEV_PTR device, void * data);
void devSetCloseOnZero(DEV_PTR device, BOOLEAN closeOnZero);

/* 
 * functions for getting the parameter values one at at time.
 */

const char *devGetName(DEV_PTR device);
fd_set *devGetFds(DEV_PTR device);
BOOLEAN devGetUseSimulator(DEV_PTR device);
const char *devGetSimMachine(DEV_PTR device);
int devGetPort(DEV_PTR device);
int devGetBaudRate(DEV_PTR device);
unsigned int devGetListening(DEV_PTR device);
BOOLEAN devGetDebug(DEV_PTR device);
FILE *devGetDebugFile(DEV_PTR device);
DEVICE_OUTPUT_HND devGetOutputHnd(DEV_PTR device);
DEVICE_HANDLER devGetSignalHnd(DEV_PTR device);
DEVICE_HANDLER devGetDisconnectHnd(DEV_PTR device);
DEVICE_HANDLER devGetReconnectHnd(DEV_PTR device);
struct timeval *devGetPollInterval(DEV_PTR device);
void *devGetClientData(DEV_PTR device);
BOOLEAN devGetCloseOnZero(DEV_PTR device);

/*
 * devConnect: connect the device according the the parameter settings.
 */

void devConnect(DEV_PTR device);

/* 
 * devMainLoop: Loop for ever processing events and dispatching them to the 
 * correct handlers for each device.
 */

void devMainLoop(void);

/* 
 * ProcessDevices: Process events for up to maxTime time.
 * If the timeout is NULL, the maximum time to process is infinite.
 * Note that this function will return after 1 or more events have been 
 * processed, even if the max allowed time has not elapsed. 
 */

void devProcessDevices(struct timeval *maxTime);

/* 
 * devStartPolling: call the polling routine every interval usecs.  Note that 
 * because the operating system is not a real time system, the delay in 
 * calling the polling routine maybe arbitrarily long.
 */

void devStartPolling(DEV_PTR dev,
		     struct timeval *interval,
		     Handler handler, void *data);

/* 
 * devStopPolling: stop calling the polling routine.
 */

void devStopPolling(DEV_PTR dev);

/* 
 * devIsPolling: Returns true if the device is being polled.
 */

BOOLEAN devIsPolling(DEV_PTR dev);

/* 
 * setTimeout: Set an alarm to go off at a specific time.  
 * The handler function is called if the alarm goes off.
 */

void devSetAlarm(DEV_PTR dev, struct timeval *timeoutTime,
		 Handler timeoutHnd, void *data);

/* 
 * setTimer: Set an alarm to go off at a delay time from now.
 * The handler function is called if the alarm goes off.
 */

void devSetTimer(DEV_PTR dev, struct timeval *delay,
		 Handler timeoutHnd, void *data);

/* 
 * cancelTimeout: remove a timeout set either with setTimeout or setTimer.
 */

void devCancelTimeout(DEV_PTR dev);

/* routines for parsing characters from a device. */

/* 
 * LINE_BUFFER_POINTER: private structure used to keep track of partial lines
 * and other parsing information.
 */

typedef struct _dev_line_buffer_type *DEV_LINE_BUFFER_PTR;

/* 
 * createLineBuffer: Create a data structure to keep track of information
 * needed to parse output from a device and dispatch it to a routine for 
 * processing.  This handles partial lines and more than one line ready for
 * input.  The parameters specify how to recognize a "line"
 *
 * processRoutine: function to call when a complete line is recognized.
 *
 * lineLimit: maximum number of characters that can be in a line.
 *     For fixed length lines, set this to the expected length.
 *
 * delimChar: Characters that delimit the end of the line.  
 * If any character in this string is found it the input, it is taken to 
 * be the end of the line.
 * 
 * replaceDelimit: Should the delimit character be replaced with '\0' before
 * the parse routine is called?
 * 
 * partialLines: Should incomplete lines also be passed to the parser.
 * 
 */

DEV_LINE_BUFFER_PTR devCreateLineBuffer(int lineLimit, char *delimChar,
					void (*processRoutine)(char *, void *),
					BOOLEAN replaceDelimit,
					BOOLEAN partialLines,
					void *clientData);

/* 
 * processOutput: routine to read characters from the device, parse the 
 * input and call the parse routine.
 */

void devProcessOutput(DEV_PTR device, int fd, DEV_LINE_BUFFER_PTR lineBuffer, 
		      int chars_available);


/* 
 * devSetLineBufferData: routine set the clientData for a line buffer.
 */

void devSetLineBufferData(DEV_LINE_BUFFER_PTR lineBuffer, void *clientData);

/* 
 * devSetLineBufferData: routine set the clientData for a line buffer.
 */

void *devGetLineBufferData(DEV_LINE_BUFFER_PTR lineBuffer);

/* 
 * devFreeLineBuffer: Free memory for a line buffer.
 */

void devFreeLineBuffer(DEV_LINE_BUFFER_PTR lineBuf);

/* 
 * WaitForResponse: Process events and wait for some event that sets the 
 * doneFlage.  If timeout time expires first, the return false.
 */

BOOLEAN devWaitForResponse(DEV_PTR dev, int *doneFlag, long timeout);

/* 
 * Needs fixed.
 */

#define devExitFromWait(doneFlag) doneFlag = NOT_WAITING;

/* 
 * devShutDown: shutdown all devices.
 */

void devShutdown(void);

/* I/O functions. */

/* 
 * writeN : Write n characters from a buffer to a given device.
 * Handle pipe brakes and interrupted system calls.
 */

BOOLEAN devWriteN(int fd, const char *buf, int nChars);

/* 
 * readN: Read nchars of data from sd into buf. Continue to read until
 * nchars have been read.  Return 0 if end of file reached and -1 if
 * there was an error.  Handle pipe breaks.
 */

int devReadN(int fd, char *buf, int nchars);

/* 
 * devCharsAvailable : Find out how many characters are available to be read.
 */

long devCharsAvailable(int sd);

/* 
 * flushChars: Flush any characters from the devide input file descriptor.
 */

void devFlushChars(DEV_PTR dev);

/*****************************************************************************
 * Utility routines
 *
 * These are useful functions you may want to use.
 * 
 *****************************************************************************/

/* 
 * PAUSE_MIN_DELAY: Sleep for the minimum time possible.  
 * This is really to get the OS to run other processes, if they are waiting.
 */

#undef PAUSE_MIN_DELAY
#if defined(__sgi)
#define PAUSE_MIN_DELAY() sginap(1);	/* delay 1/100 sec */
#else
#define PAUSE_MIN_DELAY() usleep(1);	/* delay 1/1000000 sec */
#endif /* SGI */

/* 
 * fd_isZero: Check to see if an fd_set is all zero.
 */

BOOLEAN fd_isZero(fd_set *fds);

/*****************************************************************************
 * Internal routines
 *
 * These functions are lower level access to devUtils internals.
 * You should try to avoid using them as much as possible.
 * 
 *****************************************************************************/

/* you don't need this if you don't use the internal data structure directly */
void devInit(void);  


/* 
 * Communication Utility routines, these are low level.  
 * You probably don't need them. 
 */

/* 
 * getDevFD: Get the file descriptor for I/O.
 */

int devGetFd(DEV_PTR dev);

/* 
 * devGetDev: Get the file descriptor for I/O.
 */

DEV_PTR devGetDev(int fd);

/* 
 * devConnectToSocket: attempt to connect to a "port" using either a unix 
 * socket or a TCP/IP socket.
 */

int devConnectToSocket(DEV_PTR dev);

/* 
 * devConnectToSimulator: Connect a device to the simulated device.
 */

int devConnectToSimulator(DEV_PTR dev);

/* 
 * serverInitialize: Initialize a server. Creates a socket, looks up the
 * server port number and binds the socket to that port number, and listens
 * to the socket for connection requests.
 */

BOOLEAN devServerInitialize(DEV_PTR dev);

/* 
 * connectTotty: open a tty for reading and writing.  Sets exclusive access.
 */

int devConnectTotty(DEV_PTR dev);

/* 
 * startListening: start listening for connection requests.
 */

BOOLEAN devStartListening(int port, DEV_PTR dev);

/* 
 * updateConnections: update the set of connections for a device using the 
 * given set of connections.  This is used for devices that manage their own
 * connections.
 */

void devUpdateConnections(DEV_PTR dev,fd_set *fds, int maxFD);

/* 
 * connectDev: Change the device status to be connected and handle some book
 * keeping.  Connects the device for receiving and sending on the fd.
 */

void devConnectDev(DEV_PTR dev, int fd);

/* 
 * devConnectDevReceiveOnly: Like devConnectDev, but the device is setup
 * only to listen to the fd and not to send anything.
 */

void devConnectDevReceiveOnly(DEV_PTR dev, int fd);

/* 
 * devConnectDevSendOnly: Like devConnectDev, but the device is setup
 * only to talk to the fd and not to listen.
 */

void devConnectDevSendOnly(DEV_PTR dev, int fd);

/* 
 * disconnectDev: Change the device status to be not_connected and handle
 *  some book keeping.  
 */

void devDisconnectDev(DEV_PTR dev,BOOLEAN isDisconnected, BOOLEAN reconnect);

/* Device status functions */

/* 
 *  devHasFd: True if the device has a valid open file descriptor.
 */

BOOLEAN devHasFd(DEV_PTR dev);

/* The following is to maintain compatibility. */
#define devIsConnected(dev) devHasFd(dev)

/* 
 * isDisconnected: True is the device status is DISCONNECTED.
 * Note that a device that does not listen or accept new connections
 * may not have a valid fd, and thus devIsConnected will return false, but
 * the device may not be in the "NOT_CONNECTED" state, and devIsDisconnected
 * will also return false.
 */

BOOLEAN devIsDisconnected(DEV_PTR dev);

/* 
 * setMaxCycleTime: Max time devUtils will be idle.  Can be used to ensure the
 * process does not get swapped out.
 */

void devSetMaxCycleTime (int secs, int usecs);



/* 
 * File descriptor utility routines.
 */

void fd_copy(fd_set *src, fd_set *dst);
void fd_copy_or(fd_set *src, fd_set *dst);
void fd_copy_xor(fd_set *src, fd_set *dst);
BOOLEAN fd_equal(fd_set *set1, fd_set *set2);

/*****************************************************************************
 * Compatibility Macros.
 *****************************************************************************/
#ifndef DEV_FUNCTIONAL_ONLY

#define connectToSocket(dev) devConnectToSocket(dev)
#define connectToSimulator(dev) devConnectToSimulator(dev)
#define serverInitialize(dev, num_connections) devServerInitialize(dev)
#define startListening(port, connections, dev) devStartListening(port,dev)
#define updateConnections(dev,fds) devUpdateConnections(dev,fds,FD_SETSIZE)
#define isConnected(dev) devIsConnected(dev)
#define connectTotty(dev) devConnectTotty(dev)
#define getDevFD(dev) devGetFd(dev)
#define connectDev(dev, fd) devConnectDev(dev, fd)
#define disconnectDev(dev,isDisconnected, reconnect)\
devDisconnectDev\
(dev,isDisconnected,reconnect)
#define isDisconnected(dev) devIsDisconnected(dev)
#define flushChars(dev) devFlushChars(dev)
#define writeN(dev, buf, nChars) devWriteN(devGetFd(dev), buf, nChars)
#define readN(dev, fd, buf, nChars) devReadN(fd, buf, nChars)
#define numChars(sd) devCharsAvailable(sd)
#define ProcessDevices(maxTime) devProcessDevices(maxTime)
#define setTimeout(dev,delay,timeoutHnd, data) devSetAlarm(dev,delay,timeoutHnd,data)
#define setTimer(dev,delay,timeoutHnd,data) devSetTimer(dev,delay,timeoutHnd,data)
#define cancelTimeout(dev)  devCancelTimeout(dev) 
#define _line_buffer_type _dev_line_buffer_type
#define LINE_BUFFER_PTR DEV_LINE_BUFFER_PTR
#define createLineBuffer(lineLimit, delimChar,processRoutine, replaceDelimit, partialLines,clientData) devCreateLineBuffer(lineLimit,delimChar,processRoutine, replaceDelimit, partialLines,clientData)

#define processOutput(device, fd, lineBuffer, chars_available)\
devProcessOutput\
(device, fd, lineBuffer, chars_available)
#define WaitForResponse(dev, doneFlag, timeout) \
devWaitForResponse\
(dev, doneFlag, timeout)
#define setMaxTimeout(secs, usecs) devSetMaxCycleTime(secs, usecs)

#define DEFAULT_BAUD DEV_DEFAULT_BAUD
#define DEFAULT_PORT DEV_DEFAULT_SIM_PORT

#ifdef linux
int openRaw(const char *filename, mode_t io_flags);
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif	/* DEVUTIL_LOADED */
