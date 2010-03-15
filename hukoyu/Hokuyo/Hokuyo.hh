/* Driver class (header) for hokuyo URG-04LX and UTM-30LX

    Aleksandr Kushleyev <akushley(at)seas(dot)upenn(dot)edu>
    University of Pennsylvania, 2008

    BSD license.
    --------------------------------------------------------------------
    Copyright (c) 2008 Aleksandr Kushleyev
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    3. The name of the author may not be used to endorse or promote products
      derived from this software without specific prior written permission.

      THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
      IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
      OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
      IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
      INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
      NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
      DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
      THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
      (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
      THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/


#ifndef HOKUYO_HH
#define HOKUYO_HH

//includes
#include <iostream>
#include <string>
#include <sstream>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <errno.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <pthread.h>
#include <math.h>

#include "SerialDevice.hh"

#ifdef MATLAB_MEX_FILE
#include "mex.h"
#endif


/** The default number of tries to communicate with the Hokuyo before giving up. */
#define DEFAULT_NUM_TRIES 5
#define SWITCH_MODE_TRIES 10
#define SEND_STOP_TRIES 10

/** Defines the initial baud of the Hokuyo */
#define HOKUYO_INIT_BAUD_RATE 115200

#define HOKUYO_READ_LINE_TIMEOUT 0.2
#define HOKUYO_READ_PACKET_TIMEOUT 0.050
#define HOKUYO_NUM_TEST_BAUD_RETRIES 2
#define HOKUYO_MAX_NUM_POINTS 3000

//actually 769 is the max number of points for range only scan for URG-04LX, 
//but when range+intensity+agc is requested, it goes up to 771
#define HOKUYO_MAX_NUM_POINTS_URG_04LX 771 
#define HOKUYO_MAX_NUM_POINTS_UTM_30LX 1081*2

#define HOKUYO_MAX_PACKET_LENGTH 15000 
#define HOKUYO_MAX_DATA_LENGTH 10000
#define HOKUYO_MAX_LINE_LENGTH 100

#define HOKUYO_TYPE_URG_04LX 0 
#define HOKUYO_TYPE_UTM_30LX 1 
#define HOKUYO_TYPE_URG_04LX_STRING "SOKUIKI Sensor URG-04LX"
//#define HOKUYO_TYPE_UTM_X001S_STRING "SOKUIKI Sensor TOP-URG UTM-X001S"
#define HOKUYO_TYPE_UTM_30LX_STRING "SOKUIKI Sensor TOP-URG UTM-30LX"


//special values of "skip" values used to get intensities from URG-04LX
//refer to hokuyo 04LX intensity mode manual for details
#define HOKUYO_AGC_1 219
#define HOKUYO_AGC_0 220
#define HOKUYO_RANGE_INTENSITY_1_AGC_1 221
#define HOKUYO_RANGE_INTENSITY_0_AGC_0 222
#define HOKUYO_RANGE_INTENSITY_AV_AGC_AV 223

#define HOKUYO_INTENSITY_0 237
#define HOKUYO_INTENSITY_1 238
#define HOKUYO_INTENSITY_AV 239

#define HOKUYO_RANGE_INTENSITY_0 253
#define HOKUYO_RANGE_INTENSITY_1 254
#define HOKUYO_RANGE_INTENSITY_AV 255

#define HOKUYO_TOP_URG_RANGE_INTENSITY 256

//these are the allowed scan names
#define HOKUYO_RANGE_STRING "range" //for 04LX and 30LX

//for 04LX only:
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

//for 30LX only:
#define HOKUYO_TOP_URG_RANGE_INTENSITY_STRING "top_urg_range+intensity"


//types of scan

//everything except the UTM-30LX ME scan
#define HOKUYO_SCAN_REGULAR 0

//only for UTM-30LX scan, which is weird in terms of 
//returning command confirmation, so need to take special care
//in handling the responses
#define HOKUYO_SCAN_SPECIAL_ME 1

//character encoding
#define HOKUYO_2DIGITS 2
#define HOKUYO_3DIGITS 3

//SCIP modes
#define HOKUYO_SCIP1 0
#define HOKUYO_SCIP20 1

#define HOKUYO_SERIAL 0

//TCP is not supported any more
#define HOKUYO_TCP 1

//settings for serial device server (obsolete)
#define HOKUYO_SDS_HTTP_PORT 80
#define HOKUYO_SDS_START_DEVICE_PORT 8000
#define HOKUYO_SDS_CONFIG_CHANGE_CONFIRM_MAX_SIZE 512

//when serial device server returns an html document as a confirmation of accepted changes, the size is exactly 396
//if the size is different, then something went wrong
#define HOKUYO_SDS_CONFIG_CHANGE_CONFIRM_SIZE 396

#define HOKUYO_TURN_LASER_OFF false
#define HOKUYO_TURN_LASER_ON true
#define HOKUYO_NUM_STOP_LASER_RETRIES 5
#define HOKUYO_LASER_STOP_DELAY_US 50000
#define HOKUYO_TEST_BAUD_RATE_DELAY_US 100000

#define HOKUYO_URG_04LX_GET_SCAN_TIMEOUT_US 500000
#define HOKUYO_UTM_30LX_GET_SCAN_TIMEOUT_US 500000


//defines for parsing the sensor information
#define HOKUYO_INFO_DELIM_START ':'
#define HOKUYO_INFO_DELIM_STOP ';'

#define HOKUYO_VEND_STR "VEND"
#define HOKUYO_PROD_STR "PROD"
#define HOKUYO_FIRM_STR "FIRM"
#define HOKUYO_PROT_STR "PROT"
#define HOKUYO_SERI_STR "SERI"

#define HOKUYO_MODL_STR "MODL"
#define HOKUYO_DMIN_STR "DMIN"
#define HOKUYO_DMAX_STR "DMAX"
#define HOKUYO_ARES_STR "ARES"
#define HOKUYO_AMIN_STR "AMIN"
#define HOKUYO_AMAX_STR "AMAX"
#define HOKUYO_AFRT_STR "AFRT"
#define HOKUYO_SCAN_STR "SCAN"

class Hokuyo
{

public:
  

  void LockDataMutex();
  void UnlockDataMutex();
  void DataCondSignal();
  int DataCondWait(int timeout_ms);


  //Device type (e.g URG-04LX or UTM-30LX)
  int _type;
  
  //do we have a fresh scan? This is used by HokuyoReader class
  bool _scan_ready;

  // Constructor
  Hokuyo();

  // Destructor
  ~Hokuyo();
  
  // Initializes the Hokuyo
  int Connect(std::string dev_str, const int baud_rate, const int mode=HOKUYO_SCIP20);
  
  // send a specific command to hokuyo and count number of bytes in response
  //int sendCmdAndCountResponseBytes(char * command, int num);
  
  // Disconnect from Hokuyo
  int Disconnect();
  
  // send the command to turn on/off the laser
  int LaserOn();
  int LaserOff();

  // Get a scan from Hokuyo
  // proide a pointer to allocated buffer, reference var with number of points (ranges) and scan parameters
  int GetScan(unsigned int * range, int & n_range, int scan_start, int scan_end, int scan_skip, int encoding, int scan_type, int num_scans);

  //
  int CreateScanRequest(int scanStart, int scanEnd, int scanSkip, int encoding, int scanType, int numScans, char * req, bool & needToGetConfirm);
  
  
  //will need to request a scan if using in a single scan mode or
  //to start sensor in streaming mode
  int RequestScan(char * req);
  
  //confirm that the scan request was accepted
  int ConfirmScan(char * req, int timeout_us);
  
  //find the start of packet in case of loss of synchronization
  int FindPacketStart();

  //get scan params that should be sent to sensor in order to get a specific scan
  int GetScanTypeAndSkipFromName(int sensorType, char * scanTypeName, int * skip, int * scanType);

  //get number of quantities measured by a scan (only range, or range and intensity..)
  int GetNumOutputArgs(int sensorType, char * scanTypeName);


  //accessor functions for the sensor information
  std::string GetVendor();
  std::string GetProduct();
  std::string GetFirmware();
  std::string GetProtocol();
  std::string GetSerial();
  std::string GetModel();
  double GetDistMin();
  double GetDistMax();
  double GetAngleRes();
  double GetAngleMin();
  double GetAngleMax();
  double GetScanRate();
  double GetDistRes();
  int GetSensorType();
  int GetCountZero();
  int GetCountMin();
  int GetCountMax();

  //stuff for matlab mex files
#ifdef MATLAB_MEX_FILE
  //create empty output matrices for given number of output arguments
  void CreateEmptyOutputMatrices(int nlhs, mxArray * plhs[]);

  //parse the single data packet into different arrays if needed
  int CreateOutput(int sensorType, char * scanType, unsigned int * data, uint16_t n_points, int nlhs, mxArray * plhs[]);

  //parse the scan parameters, passed in from matlab
  int ParseScanParams(int sensorType,char * scanType,const mxArray * prhs[],int indexStart, int * start, int * stop, int * skip, int * encoding, int * specialScan);
#endif //MATLAB_MEX_FILE
  

protected:

  // SerialDevice object that's used for communication
  SerialDevice sd;
  
  pthread_mutex_t _data_mutex;
  pthread_cond_t _data_cond;

  // A path to the device at which the sick can be accessed.
  std::string _device;
  
  int _readPacket(char * data, int & packet_length, int timeout_us);
  int _measurePacketLength();
  int _extractPacket(char * full_packet, char * extracted_packet, int packet_length);
  int _decodePacket(char * extracted_packet, unsigned int * extracted_data, int extracted_length, int encoding);
  int _checkLineChecksum(char * line, int max_length);
  void _printStrings(const char * exp, char * got, int num);
  void _printLine(char * line, int line_len);
  int _laserOnOff(bool turnOn);
   int _parseString(char * buf, int buf_len, char cStart, char cStop, std::string name, std::string & value);

  //sensor information
  std::string _vendor,_product,_firmware,_protocol,_serial, _model, _dmin, _dmax, _ares, _amin, _amax, _afrt, _scan;
  double _dist_min, _dist_max, _angle_res, _angle_min, _angle_max, _scan_rate, _dist_res;
  int _count_min, _count_max, _count_zero;
    
  // Hokuyo mode (SCIP1 or SCIP2.0)
  int _mode;
  
  // is the scanner streaming data?
  bool _streaming;
  
  //scan start count
  int _scan_start;
  
  //scan end count
  int _scan_end;
  
  //this effectively sets the resolution (or so-called cluster count, aka "skip value")
  //however in urg 04LX special value of this variable is used to obtain intensity data
  int _scan_skip;
  
  //2 or 3 char encoding
  int _encoding;
  
  //regular or special ME intensity scan for UTM-30LX
  int _scan_type;
  
  //length of measurement packet, in bytes, including the EOL chars
  int _packet_length;
  
  // Communication protocol for the device: serial port or TCP (serial device server)
  int _comProtocol;

  // The baud rate at which to communicate with the Hokuyo
  int _baud;

  // Sets the baud rate for communication with the Hokuyo.
  int _setBaudRate(const int baud_rate);

  /** Tests communication with the Hokuyo at a particular baud rate. */
  int _testBaudRate(const int baud_rate);

  /** Gets the status of the Hokuyo. */
  int _getSensorInfoAndParams();
  int _getSensorInfo(int mode);
  int _getSensorParams();
  
  /** Reads a line and returns number of chars read */
  int _readLine(char * line, int max_chars, int timeout_us, bool checkSum);
  
  /** Set the SCIP2.0 protocol on hokuyo */
  int _setSCIP20();

};


#endif //HOKUYO_HH
