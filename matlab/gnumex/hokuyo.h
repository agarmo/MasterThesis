#ifndef HOKUYO_DRIVER_
#define HOKUYO_DRIVER_


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define HOKUYO_TYPE_URG_04LX 0
#define HOKUYO_TYPE_UTM_30LX 1

#define HOKUYO_MAX_NUM_POINTS_UTM_30LX 1081*2
#define HOKUYO_MAX_NUM_POINTS_URG_04LX 771

#define HOKUYO_MAX_NUM_POINTS 3000

#define HOKUYO_3DIGITS 3


//for 04LX only:

#define HOKUYO_RANGE_STRING "range"
#define HOKUYO_RANGE_INTENSITY_AV_STRING "range+intensityAv"
#define HOKUYO_RANGE_INTENSITY_0_STRING "range+intensity0"
#define HOKUYO_RANGE_INTENSITY_1_STRING "range+intensity1"
#define HOKUYO_INTENSITY_AV_STRING "intensityAv"
#define HOKUYO_INTENSITY_0_STRING "intensity0"
#define HOKUYO_INTENSITY_1_STRING "intensity1"
#define HOKUYO_RANGE_INTENSITY_AV_AGC_AV_STRING "range+intensityAv+AGCAv"
#define HOKUYO_RANGE_INTENSITY_0_AGC_0_STRING "range+intensity0+AGC0"
#define HOKUYO_RANGE_INTENSITY_1_AGC_1_STRING "range+intensity1+AGC1"
#define HOKUYO_AGC_0_STRING "AGC0"
#define HOKUYO_AGC_1_STRING "AGC1"


static HANDLE HComm = INVALID_HANDLE_VALUE;
static char* ErrorMessage = "no error.";



enum {
    Timeout = 800,
    LineLength = 16 + 64 + 1 + 1 + 1,
};

enum {
    MODL = 0,		//!< Sensor Model
    DMIN,			//!< Min detection range [mm]
    DMAX,			//!< Man detection range [mm]
    ARES,			//!< Angular resolution (division of 360degree)
    AMIN,			//!< Min Measurement step
    AMAX,			//!< Max Measurement step
    AFRT,			//!< Front Step 
    SCAN,			//!< Standard scan speed
};


typedef struct {
    char * model;		//!< Obtained Sensor Model,  MODL
    long distance_min;		//!< Obtained DMIN 
    long distance_max;		//!< Obtained DMAX 
    int area_total;		//!< Obtained ARES 
    int area_min;			//!< Obtained AMIN 
    int area_max;			//!< Obtained AMAX 
    int area_front;		//!< Obtained AFRT 
    int scan_rpm;			//!< Obtained SCAN 

    int first;			//!< Scan Start position
    int last;			//!< Scan end position
    int max_size;			//!< Max. size of data
    long last_timestamp; //!< Time stamp of the last obtained data
} urg_state_t;



static int com_connect(const char* device, long baudrate);
static void com_disconnect(void);
static int com_send(const char* data, int size);
static int com_recv(char* data, int max_size, int timeout);

// Send data(Commands) to URG 
static int urg_sendTag(const char* tag);

// Read data (Reply) from URG until the termination 
static int urg_readLine(char *buffer);

// Send data (Commands) to URG and wait for reply
static int urg_sendMessage(const char* command, int timeout, int* recv_n);

// Read URG parameters
static int urg_getParameters(urg_state_t* state);

// Process to connect with URG 
static int urg_connect(urg_state_t* state, const char* port, const long baudrate);

// Disconnect URG 
static void urg_disconnect(void);

// Data read using GD-Command
static int urg_captureByGD(const urg_state_t* state);

// Data read using MD-Command
static int urg_captureByMD(const urg_state_t* state, int capture_times);

// Decode 6 bit data from URG 
static long urg_decode(const char* data, int data_byte);

// Receive URG distance data 
static int urg_addRecvData(const char buffer[], long data[], int* filled);

// Receive URG data
static int urg_receiveData(urg_state_t* state, long data[], size_t max_size);


#endif
