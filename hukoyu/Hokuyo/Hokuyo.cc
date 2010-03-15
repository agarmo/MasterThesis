/* Driver class for hokuyo URG-04LX and UTM-30LX

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

#include "Hokuyo.hh"

// Constructor

//connect using serial or USB connection
Hokuyo::Hokuyo()
{
  _type=HOKUYO_TYPE_URG_04LX;
  _baud=0;
  _comProtocol=HOKUYO_SERIAL;
  _device=std::string("Unknown");
  _mode=HOKUYO_SCIP20;
  _streaming=false;

  //initialize vars to invalid values
  _scan_start      = -1;
  _scan_end        = -1;
  _scan_skip       = -1;
  _encoding        = -1;
  _scan_type       = -1;
  _packet_length   = -1;
  _dist_min        = -1;
  _dist_max        = -1;
  _dist_res        = -1;
  _angle_res       = -1;
  _angle_min       =  0;
  _angle_max       =  0;
  _scan_rate       = -1;
  _count_min       = -1;
  _count_max       = -1;
  _count_zero      = -1;

  //initialize mutexes
  pthread_mutex_init(&_data_mutex,NULL);
  pthread_cond_init(&_data_cond,NULL);
}


Hokuyo::~Hokuyo() 
{
  if(sd.Disconnect())
    std::cout << "Hokuyo::~Hokuyo - sd.Disconnect() failed to close terminal" <<std::endl;

  //clean up mutexes  
  pthread_mutex_destroy(&_data_mutex);
  pthread_cond_destroy(&_data_cond);
}


//connect to the scanner - need to provide device name ("/dev/ttyACM0"),
//baud rate (e.g. 115200), and, optionally, mode (currently SCIP2.0 only)
int Hokuyo::Connect(std::string dev_str, const int baud_rate, const int mode) 
{
  _device=dev_str;

  //connect to the device
  if(sd.Connect((char *)(_device.c_str()),baud_rate)) 
  {
    std::cout << "Hokuyo::Connect - Connect() failed!" << std::endl;
    return -1;
  }
  
  //find out what rate the sensor is currently running at. _baud will be set to the current baud rate.
  if(!_testBaudRate(115200)) 
    std::cout << "Hokuyo baud rate is 115200bps..." << std::endl;
  
  else if(!_testBaudRate(19200))
    std::cout << "Hokuyo baud rate is 19200bps..." << std::endl;

  else if(!_testBaudRate(38400))
    std::cout << "Hokuyo baud rate is 38400bps..." << std::endl;

  else 
  {
    std::cout << "Hokuyo::Connect: failed to detect baud rate!" << std::endl;
    std::cout << "Hokuyo::Connect: make sure that the sensor is upgraded to SCIP 2.0" << std::endl;
    std::cout << "Hokuyo::Connect: the sensor does not have start in SCIP2.0 but must support it" << std::endl;
    sd.Disconnect();

    return -1;
  }
  
  fflush(stdout);
  
  // Make sure requested baud isn't already set
  if(_baud == baud_rate) 
    std::cout << "Hokuyo::Connect - Hokuyo is already operating @ " << baud_rate << std::endl;
  
  //need to set the baud rate
  else 
  {
    std::cout << "Hokuyo::Connect: Attempting to set requested baud rate..." << std::endl;
    if (!_setBaudRate(baud_rate))
    {
      //success
      std::cout << "Hokuyo::Connect: Operating @ " <<  _baud << std::endl;
    } 
    
    //could not set the baud rate
    else 
    {
      std::cout << "Hokuyo::Connect - Unable to set the requested Hokuyo baud rate of " <<  baud_rate << std::endl;
      sd.Disconnect();    
      return -1;
    }
  }
  
  // get the status data from sensor to determine its type
  if(_getSensorInfoAndParams())
  {
    std::cout << "Hokuyo::Connect - could not get status data from sensor" << std::endl;
    sd.Disconnect();   
    return -1;
  }
  
  std::cout << "Hokuyo::Connect: Turning the laser on" <<std::endl;
  if (LaserOn())
  {
    std::cout << "Hokuyo::Connect: was not able to turn the laser on" << std::endl;
    sd.Disconnect();
    return -1;
  }
  
  std::cout << "Hokuyo::Connect: Initialization Complete." <<std::endl;
  return 0;
}

//stop the sensor and disconnect from the device
int Hokuyo::Disconnect() {
  
  if (!sd.IsConnected())
    return 0;
  
  if (!LaserOff())
    std::cout<<"Hokuyo::Disconnect: Laser has been shut off"<<std::endl;
  
  else 
    std::cout<<"Hokuyo::Disconnect: ERROR: Wasn't able to shut off the laser"<<std::endl;

  sd.Disconnect();
  return 0;
}

//find the start of packet in case something got out of sync
int Hokuyo::FindPacketStart()
{
  static char data[HOKUYO_MAX_PACKET_LENGTH];
  char seq[2]={0x0A,0x0A};   //two LFs
  
  if (!sd.IsConnected())
  {
    std::cout << "Hokuyo::_findPacketStart: not connected to the sensor!" << std::endl;
    return -1;
  }

  //set the io mode to return when the termination sequence is read
  //then we know that we've reached the end of some packet  
  sd.Set_IO_BLOCK_W_TIMEOUT_W_TERM_SEQUENCE(seq,2,true);

  //read until the end sequence is found or until read too much
  int num_chars=sd.ReadChars(data,HOKUYO_MAX_PACKET_LENGTH);
  
  if ( num_chars< 1 )
  {
    std::cout << "Hokuyo::_findPacketStart: Error: terminating character was not read"<<std::endl;
    return -1;
  }
  return 0;
}

//measure the length of a packet in bytes
int Hokuyo::_measurePacketLength()
{
  static char dataIn[HOKUYO_MAX_PACKET_LENGTH];
  char seq[2]={0x0A,0x0A};   //two LFs
  
  if (!sd.IsConnected())
  {
    std::cout << "Hokuyo::_measurePacketLength: not connected to the sensor!" << std::endl;
    return -1;
  }
  
  //set the io mode to return when the termination sequence is read
  //then we know that we've reached the end of some packet
  sd.Set_IO_BLOCK_W_TIMEOUT_W_TERM_SEQUENCE(seq,2,true);
  
  //read until the end sequence is found or until read too much
  int num_chars=sd.ReadChars(dataIn,HOKUYO_MAX_PACKET_LENGTH);
  
  if ( num_chars< 1)
  {
    std::cout << "Hokuyo::_measurePacketLength: Error: terminating character was not read"<<std::endl;
    return -1;
  }

  //return the number of characters read, which is the packet length
  return num_chars;
}

//read the packet either as a one blocked read or look for the terminating sequence
int Hokuyo::_readPacket(char * data, int & packet_length, int timeout_us)
{
  if (!sd.IsConnected())
  {
    std::cout << "Hokuyo::_readPacket: not connected to the sensor!" << std::endl;
    return -1;
  }

  //if packet length is greater than zero, it means that we already know
  //the expected length, therefore just use that - dont need to explicitly
  //search for the end of packet - just need to check for it at the end.
  if (packet_length > 0)
  {
    //std::cout <<"Hokuyo::_readPacket: using previous packet length" <<std::endl;
    sd.Set_IO_BLOCK_W_TIMEOUT();  
    if (sd.ReadChars(data,packet_length,timeout_us) != packet_length){
      std::cout << "Hokuyo::_readPacket: could not read the number of expected characters!" << std::endl;
      return -1;
    }
  }

  //packet length is unknown, therefore we need to find it
  else 
  {
    //std::cout <<"Hokuyo::_readPacket: need to measure packet length" <<std::endl;
    char term_sequence[2]={0x0A,0x0A};
    
    //set the io mode to return when the termination sequence is read
    //then we know that we've reached the end of some packet
    sd.Set_IO_BLOCK_W_TIMEOUT_W_TERM_SEQUENCE(term_sequence, 2, true);

    //read until the end sequence is found or until read too much
    int chars_read=sd.ReadChars(data,HOKUYO_MAX_PACKET_LENGTH,timeout_us);
    
    if (chars_read < 0)
    {
      std::cout << "Hokuyo::_readPacket: could not read the terminating character while trying to determine the packet length!" << std::endl;
      return -1;
    }
    packet_length=chars_read;
  }
  
  return 0;
}

//extract the data by verifying the checksum and removing it along with LFs
int Hokuyo::_extractPacket(char * full_packet, char * extracted_packet, int packet_length)
{
  int lenNLFC=0;     //length of data without LFs and checksum
  int lineLength;
  int lineCount=0;
  
  
  if (packet_length < 1)
  {
    std::cout<<"Hokuyo::_extractPacket: bad length!!!"<<std::endl;
    return -1;
  }
  
  lineLength=_checkLineChecksum(full_packet, HOKUYO_MAX_LINE_LENGTH);
  
  //line length will be zero at the end of packet, when the last LF is read
  while (lineLength > 0)
  {
    lineCount++;
  
    //copy the data
    memcpy(extracted_packet,full_packet,lineLength);
    
    extracted_packet+=lineLength;
    full_packet+=(lineLength+2);     //advance the pointer 2 chars ahead to skip the checksum and endline char
    lenNLFC+=lineLength;
    
    //check the next line
    lineLength=_checkLineChecksum(full_packet, HOKUYO_MAX_LINE_LENGTH);
  }
  if (lineLength < 0)
  {
    std::cout<<"Hokuyo::_extractPacket: error extracting a line!!! line count="<<lineCount<<std::endl;
    return -1;
  }
  
  //std::cout<<"Hokuyo::_extractPacket: line count="<<lineCount<<std::endl;
  return lenNLFC;
}

//decode the packet using 2- or 3-character enconding
int Hokuyo::_decodePacket(char * extracted_packet, unsigned int * extracted_data, int extracted_length, int encoding)
  {
  int data_length=0;
  
  if (extracted_length < 1)
  {
    std::cout<<"Hokuyo::_decodePacket: bad length!!!"<<std::endl;
    return -1;
  }
  
  switch (encoding)
  {
  
    case HOKUYO_2DIGITS:
      data_length = 0;
      for(int i=0;i<extracted_length-1;i++)   //FIXME: do we really need -1 here?
      {        
        *extracted_data=((extracted_packet[i] - 0x30)<<6) + extracted_packet[i+1] - 0x30;
        i++;
        data_length++;
        if (data_length > HOKUYO_MAX_NUM_POINTS)
        {
          std::cout << "Hokuyo::_decodePacket: returned too many data points" << std::endl;
          return -1;
        }
      }
      break;
    
    case HOKUYO_3DIGITS:
      data_length = 0;
      for(int i=0;i<extracted_length-1;i++)   //FIXME: do we really need -1 here?
      {        
        *extracted_data=((extracted_packet[i] - 0x30)<<12) + ((extracted_packet[i+1] - 0x30)<<6) + extracted_packet[i+2] - 0x30;
        i+=2;
        extracted_data++;
        data_length++;
        if (data_length > HOKUYO_MAX_NUM_POINTS)
        {
          std::cout << "Hokuyo::_decodePacket: returned too many data points" << std::endl;
          return -1;
        }
      }
      break;
    default:
      std::cout << "Hokuyo::_decodePacket: bad encoding" << std::endl;
      return -1;
  }
  
  return data_length;
}


//verify the checksum of the line, which is the last character of the line
int Hokuyo::_checkLineChecksum(char * line, int max_length)
{
  
  char * end_line;
  int line_length;
  char sum=0;
  
  end_line=(char *)memchr((void *)line,0x0A, max_length);
  if (end_line == NULL)
  {
#ifdef HOKUYO_LOW_LEVEL_DEBUG
    std::cout<< "Hokuyo::_checkLineChecksum: Error: end of line is not found"<<std::endl;
#endif
    return -1;
  }
  
  line_length=end_line-line;
  
  //sanity check
  if ( (line_length < 0) || (line_length > HOKUYO_MAX_LINE_LENGTH))
  {
#ifdef HOKUYO_LOW_LEVEL_DEBUG
    std::cout<< "Hokuyo::_checkLineChecksum: Error: bad line length"<<std::endl;
#endif
    return -1;
  }
  
  //empty line
  if (line_length==0)
    return 0;
  
  //compute the checksum up to the checksum character
  for (int j=0;j<line_length-1;j++)
    sum+=line[j];
  
  //encode the sum
  sum = (sum & 0x3f) + 0x30;
  
  if (sum!=line[line_length-1])
  {
#ifdef HOKUYO_LOW_LEVEL_DEBUG
    std::cout<<"Hokuyo::_checkLineChecksum:Checksum ERROR!!!"<<std::endl;
#endif
    //std::cout<<"line_len="<<line_length<<std::endl;
    //std::cout<<"line: "<<line<<std::endl;
    //std::cout<<"prev line: "<<std::endl;
    //std::cout<<(line-100)<<std::endl;
    
    return -1;
  }
  
  return (line_length-1);
}

//read one line from the sensor and optionally verify the checksum
//max specifies the maximum expected length of the line
int Hokuyo::_readLine(char * line,int max_chars, int timeout_us,bool checkSum)
{
  char seq[1]={0x0A};   //LF
  
  if (!sd.IsConnected())
  {
    std::cout << "Hokuyo::_readLine: not connected to the sensor!" << std::endl;
    return -1;
  }
  
  //set the io mode to return when LF is read or timeout or read too much
  sd.Set_IO_BLOCK_W_TIMEOUT_W_TERM_SEQUENCE(seq,1,true);
  
  //read the line
  int num_chars=sd.ReadChars(line,max_chars, timeout_us);
  
  if ( num_chars< 1)
  {
#ifdef HOKUYO_LOW_LEVEL_DEBUG
    std::cout << "Hokuyo::_readLine: Error: terminating character was not read"<<std::endl;
#endif
    return -1;
  }
  
   //empty line, therefore return
  if (num_chars==1)
  {
    if (line[0]==0x0A)
      return 0;

    else
    {
#ifdef HOKUYO_LOW_LEVEL_DEBUG
      std::cout << "Hokuyo::_readLine: Error: read one char and it was not an endl"<<std::endl;
#endif
      return -1;
    }
  }
  
  if (!checkSum)
    return num_chars-1;
  

  if (_checkLineChecksum(line, num_chars) != num_chars-2)
  {
#ifdef HOKUYO_LOW_LEVEL_DEBUG
    std::cout << "Hokuyo::_readLine: Error: checksum error"<<std::endl;
#endif
    return -1;
  }
  
  return (num_chars-2);
}

//turn the laser on
int Hokuyo::LaserOn()
{

  if (LaserOff())
    return -1;
  
  if (_laserOnOff(HOKUYO_TURN_LASER_ON))
  {
#ifdef HOKUYO_LOW_LEVEL_DEBUG
    std::cout<<"Hokuyo::LaserOn: ERROR: Wasn't able to turn on the laser"<<std::endl;
#endif
    return -1;
  }
#ifdef HOKUYO_LOW_LEVEL_DEBUG
  std::cout<<"Hokuyo::LaserOn: Laser has been turned on"<<std::endl;
#endif
  return 0;
}

//turn the laser off
int Hokuyo::LaserOff()
{
  for (int i=0;i<HOKUYO_NUM_STOP_LASER_RETRIES; i++)
  {
    if (!_laserOnOff(HOKUYO_TURN_LASER_OFF))
    {
#ifdef HOKUYO_LOW_LEVEL_DEBUG
      std::cout<<"Hokuyo::LaserOff: Laser has been shut off"<<std::endl;
#endif
      return 0;
    }

    else if (i==HOKUYO_NUM_STOP_LASER_RETRIES-1)
    {
#ifdef HOKUYO_LOW_LEVEL_DEBUG
      std::cout<<"Hokuyo::LaserOff: ERROR: Wasn't able to shut off the laser"<<std::endl;
#endif
    }
    usleep(HOKUYO_LASER_STOP_DELAY_US);
  }
  return -1;
}


//turn on/off the laser
int Hokuyo::_laserOnOff(bool turnOn)
{
  char line[HOKUYO_MAX_LINE_LENGTH];
    
  if (!sd.IsConnected())
  {
    std::cout << "Hokuyo::_laserOnOff: not connected to the sensor!" << std::endl;
    return -1;
  }
  
  char cmd[3];
  char resp1[2];
  char resp2[2];
  
  if (turnOn)
  {
    memcpy(cmd,"BM\n",3);
    memcpy(resp1,"00",2);
    memcpy(resp2,"02",2);
  }
  
  else 
  {
    memcpy(cmd,"QT\n",3);
    memcpy(resp1,"00",2);
    memcpy(resp2,"00",2);
  }

  //clear out the serial input buffer  
  sd.FlushInputBuffer();
  
  //turn the laser on or off
  if(sd.WriteChars(cmd,3) != 3)
  {
#ifdef HOKUYO_LOW_LEVEL_DEBUG
    std::cout << "Hokuyo::_laserOnOff: could not write command" << std::endl;
#endif
    return -1;
  }
  
  //read back the echo of the command
  int timeout_us=200000;
  int line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,false);
  if(line_len < 0)
  {
#ifdef HOKUYO_LOW_LEVEL_DEBUG
    std::cout << "Hokuyo::_laserOnOff: could not read line 1" << std::endl;
#endif
    return -1;
  }
  
  if(strncmp(cmd,line,line_len))
  {
#ifdef HOKUYO_LOW_LEVEL_DEBUG
    std::cout << "Hokuyo::_laserOnOff: response does not match (line 1):" << std::endl;
    _printStrings(cmd,line,line_len);
#endif
    return -1;
  }
    
  //read back the status response
  line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,true);
  if(line_len < 0)
  {
#ifdef HOKUYO_LOW_LEVEL_DEBUG
    std::cout << "Hokuyo::LaserOnOff: could not read line 2" << std::endl;
#endif
    return -1;
  }
 
  if( (strncmp(resp1,line,line_len)!=0) && (strncmp(resp2,line,line_len)!=0))
  {
#ifdef HOKUYO_LOW_LEVEL_DEBUG
    std::cout << "Hokuyo::LaserOnOff: response does not match (line 2):" << std::endl;
    _printStrings(resp1,line,line_len);
    _printStrings(resp2,line,line_len);
#endif
    return -1;
  }
  
  //read off the LF
  line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,false);
  if(line_len < 0)
  {
#ifdef HOKUYO_LOW_LEVEL_DEBUG
    std::cout << "Hokuyo::LaserOnOff: could not read line 3" << std::endl;
#endif
    return -1;
  }
  
  if (!turnOn)
  {
    _streaming=false;
  }

  
  return 0;
}

//print strings for debuggin purposes
void Hokuyo::_printStrings(const char * exp, char * got, int num)
{
  std::cout << "Expected: ";
  for (int i=0;i<num;i++) 
  {
    if (exp[i]==0x0A)
    {
      break;
    }
    std::cout<<exp[i];
  }
  
  std::cout <<std::endl<< "Got: ";
  for (int i=0;i<num;i++) std::cout<<got[i];
  std::cout <<std::endl;
}


//create a scan request string based on the desired scan parameters
int Hokuyo::CreateScanRequest(int scanStart, int scanEnd, int scanSkip, int encoding, 
                              int scanType, int numScans, char * req, 
                              bool & need_to_read_off_cmd_and_status)
{

  //error checking
  if (scanStart < 0)
  {
    std::cout << "Hokuyo::CreateScanRequest: ERROR: bad scanStart" <<std::endl;
    return -1;
  }
  
  if (scanEnd < 0)
  {
    std::cout << "Hokuyo::CreateScanRequest: ERROR: bad scanEnd" <<std::endl;
    return -1;
  }
  
  if ((_type == HOKUYO_TYPE_UTM_30LX) && (scanEnd > HOKUYO_MAX_NUM_POINTS_UTM_30LX))
  {
    std::cout << "Hokuyo::CreateScanRequest: ERROR: scanEnd is greater than maximum number of points for UTM-30LX" <<std::endl;
    std::cout << "Maximum= "<<HOKUYO_MAX_NUM_POINTS_UTM_30LX<<", requested= "<<scanEnd<<std::endl;
    return -1;
  }
  
  if ((_type == HOKUYO_TYPE_URG_04LX) && (scanEnd > HOKUYO_MAX_NUM_POINTS_URG_04LX))
  {
    std::cout << "Hokuyo::CreateScanRequest: ERROR: scanEnd is greater than maximum number of points for URG-04LX" <<std::endl;
    std::cout << "Maximum= "<<HOKUYO_MAX_NUM_POINTS_URG_04LX<<", requested= "<<scanEnd<<std::endl;
    return -1;
  }
  
  //correct order of start and end
  if (scanEnd <= scanStart)
  {
    std::cout << "Hokuyo::CreateScanRequest: ERROR: scanEnd < scanStart" <<std::endl;
    return -1;
  }
  
  //valid number of scans
  if ((numScans < 0) || (numScans > 99))
  {
    std::cout << "Hokuyo::CreateScanRequest: ERROR: bad numScans" <<std::endl;
    return -1;
  }
  
  //
  if ((_type == HOKUYO_TYPE_UTM_30LX) && (encoding == HOKUYO_2DIGITS))
  {
    std::cout << "Hokuyo::CreateScanRequest: ERROR: UTM-30LX does not support 2-digit data mode." <<std::endl;
    return -1;
  }

  //single scan
  if (numScans==1)
  {
    if (scanType==HOKUYO_SCAN_REGULAR)
    {  
      if (encoding == HOKUYO_3DIGITS)
      {
        sprintf(req,"GD%04d%04d%02x\n",scanStart,scanEnd,scanSkip);
        need_to_read_off_cmd_and_status=false;
      }

      else if (encoding == HOKUYO_2DIGITS)
      {
        sprintf(req,"GS%04d%04d%02x\n",scanStart,scanEnd,scanSkip);
        need_to_read_off_cmd_and_status=false;
      }

      else 
      {
        std::cout << "Hokuyo::CreateScanRequest: ERROR: invalid selection of character encoding:"<<encoding << std::endl;
        return -1;
      }
    } 
    
    else if ((scanType==HOKUYO_SCAN_SPECIAL_ME) && (_type == HOKUYO_TYPE_UTM_30LX) ) 
    {
      if (encoding == HOKUYO_3DIGITS)
      {
        sprintf(req,"ME%04d%04d%02x0%02d\n",scanStart,scanEnd,scanSkip,numScans);
        need_to_read_off_cmd_and_status=true;
      }
      
      else
      {
        std::cout << "Hokuyo::CreateScanRequest: ERROR: invalid selection of character encoding :"<<encoding <<" not supported by UTM_30LX"  << std::endl;
        return -1;
      }
    }
    else 
    {
      std::cout << "Hokuyo::CreateScanRequest: invalid scan type :"<<scanType <<" not supported by the sensor" << std::endl;
      return -1;
    }
  }
  
  //streaming
  else if (numScans==0)
  {
    need_to_read_off_cmd_and_status=true;
    if (scanType==HOKUYO_SCAN_REGULAR)
    {  
      if (encoding == HOKUYO_3DIGITS)
        sprintf(req,"MD%04d%04d%02x0%02d\n",scanStart,scanEnd,scanSkip,numScans);

      else if (encoding == HOKUYO_2DIGITS)
        sprintf(req,"MS%04d%04d%02x0%02d\n",scanStart,scanEnd,scanSkip,numScans);

      else
      {
        std::cout << "Hokuyo::CreateScanRequest: ERROR: invalid selection of character encoding:"<<encoding << std::endl;
        return -1;
      }
    } 

    else if ((scanType==HOKUYO_SCAN_SPECIAL_ME) && (_type == HOKUYO_TYPE_UTM_30LX) )
    {
      if (encoding == HOKUYO_3DIGITS)
        sprintf(req,"ME%04d%04d%02x0%02d\n",scanStart,scanEnd,scanSkip,numScans);
      else 
      {
        std::cout << "Hokuyo::CreateScanRequest: ERROR: invalid selection of character encoding :"<<encoding <<" not supported by UTM_30LX"  << std::endl;
        return -1;
      }
    }
    
    else 
    {
      std::cout << "Hokuyo::CreateScanRequest: invalid scan type :"<<scanType <<" not supported by the sensor" << std::endl;
      return -1;
    }
  }
  
  else 
  {
    std::cout << "Hokuyo::CreateScanRequest: invalid number of scans :"<< numScans<< std::endl;
    return -1;
  }
  
  return 0;
}


//request a scan (obsolete function)
int Hokuyo::RequestScan(char * req)
{
    
  if (!sd.IsConnected())
  {
    std::cout << "Hokuyo::RequestScan: ERROR: not connected to the sensor!" << std::endl;
    return -1;
  }  
  
  sd.FlushInputBuffer();

  //write the data request command
  int reqLen = strlen(req);
  if(sd.WriteChars(req,reqLen) != reqLen)
  {
    std::cout << "Hokuyo::RequestScan: ERROR: could not write command" << std::endl;
    return -1;
  }
  
  return 0;
}


//get the confirmation of the command, requesting a scan
int Hokuyo::ConfirmScan(char * req, int timeout_us)
{
  char line[HOKUYO_MAX_LINE_LENGTH];
  int line_len;    
  //read back the echo of the command    
  line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,false);
  if(line_len < 0)
  {
    std::cout << "Hokuyo::ConfirmScan: ERROR: could not read echo of the command (line 1)" << std::endl;
    return -1;
  }
  
  if(strncmp(req,line,line_len))
  {
    std::cout << "Hokuyo::ConfirmScan: ERROR: response does not match the echo of the command (line 1):"<<std::endl;
    _printStrings(req,line,line_len);
    return -1;
  }
  
  //read back the status response
  line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,true);
  if(line_len < 0)
  {
    std::cout << "Hokuyo::ConfirmScan: ERROR: could not read status response(line 2)" << std::endl;
    return -1;
  }
  
  if(strncmp("00",line,2))
  {
    std::cout << "Hokuyo::ConfirmScan: ERROR: status response does not match (line 2):"<<std::endl;
    _printStrings("00",line,line_len);
      
    if(strncmp("10",line,2)==0)
    {
      std::cout << "Hokuyo::ConfirmScan: ERROR: (response from scanner)it looks like we need to turn the laser back on" << std::endl;
      LaserOn();
    }
    
    else if(strncmp("01",line,2)==0)
      std::cout << "Hokuyo::ConfirmScan: ERROR: (response from scanner) Starting step has non-numeric value" << std::endl;

    else if(strncmp("02",line,2)==0)
      std::cout << "Hokuyo::ConfirmScan: ERROR: (response from scanner) End step has non-numeric value" << std::endl;

    else if(strncmp("03",line,2)==0)
      std::cout << "Hokuyo::ConfirmScan: ERROR: (response from scanner) Cluster count (skip) has non-numeric value" << std::endl;

    else if(strncmp("04",line,2)==0)
      std::cout << "Hokuyo::ConfirmScan: ERROR: (response from scanner)End step is out of range" << std::endl;

    else if(strncmp("05",line,2)==0)
      std::cout << "Hokuyo::ConfirmScan: ERROR: (response from scanner)End step is smaller than starting step" << std::endl;

    else
      std::cout << "Hokuyo::ConfirmScan: ERROR: (response from scanner)Looks like hardware trouble" << std::endl;

    return -1;
  }
  
  return 0;
}

//request (if needed) a scan from sensor, read the packet and parse everything, returning good data
int Hokuyo::GetScan(unsigned int * range, int & n_range,int scan_start, 
                    int scan_end, int scan_skip, int encoding, 
                    int scan_type, int num_scans)
{
  
  char req[128];
  char good_status[2];
  char line[HOKUYO_MAX_LINE_LENGTH];
  char full_packet[HOKUYO_MAX_PACKET_LENGTH];
  char extracted_packet[HOKUYO_MAX_PACKET_LENGTH];
  int line_len;
  bool need_to_request_scan;

  if (!sd.IsConnected())
  {
    std::cout << "Hokuyo::GetScan: ERROR: not connected to the sensor!" << std::endl;
    return -1;
  }
  

  //settings have changed, therefore we need to figure out the packet length
  if ( (_scan_start!=scan_start) || (_scan_end!=scan_end) || (_scan_skip!=scan_skip) || (_encoding!=encoding) || (_scan_type!=scan_type)  )
  {
    _packet_length=-1;
    _scan_ready=false;


    if (_streaming)
    {
      if (!LaserOff())
      {
        std::cout<<"Hokuyo::GetScan: Laser has been shut off"<<std::endl;
        usleep(100000);
      }
      
      else
      {
        std::cout<<"Hokuyo::GetScan: ERROR: Wasn't able to shut off the laser"<<std::endl;
      }
    }
  
    //std::cout<<"need to check packet size!!!!" <<std::endl;
  }
  
  else
  {
    //std::cout<<"reusing old packet size" <<std::endl;
  }
  
  memcpy(good_status,"99",2*sizeof(char));
  bool need_to_read_off_cmd_and_status;
  if (CreateScanRequest(scan_start,scan_end,scan_skip,encoding,scan_type,num_scans,req,need_to_read_off_cmd_and_status))
  {
    std::cout << "Hokuyo::GetScan: ERROR: could not create scan request string!" << std::endl;
    return -1;
  }
  
  if (num_scans==1)
    need_to_request_scan=true;

  else if (num_scans==0)
  {
    if (!_streaming)
      need_to_request_scan=true;

    else
      need_to_request_scan=false;
  }

  else 
  {
    std::cout << "Hokuyo::GetScan: ERROR: invalid number of scans:"<< num_scans << std::endl;
    return -1;
  }
  
  
  int timeout_us=0;
  switch (_type)
  {
    case HOKUYO_TYPE_URG_04LX:
      timeout_us=HOKUYO_URG_04LX_GET_SCAN_TIMEOUT_US;
      break;
      
    case HOKUYO_TYPE_UTM_30LX:
      timeout_us=HOKUYO_UTM_30LX_GET_SCAN_TIMEOUT_US;
      break;
  }
  
  
  //only send the request command if we need to
  //this means only when we request a single scan or to start streaming
  if (need_to_request_scan)
  {
    //std::cout<<"Requesting: "<<req<<std::endl;
    if (RequestScan(req))
    {
      std::cout << "Hokuyo::GetScan: ERROR: could not request scan!" << std::endl;
      return -1;
    }
  
    if (ConfirmScan(req,timeout_us))
    {
      std::cout << "Hokuyo::GetScan: ERROR: could not confirm scan!" << std::endl;
      return -1;
    }
    //the read was successful, so it means that if we requested streaming, we got it
    if (num_scans==0)
      _streaming=true;
    
    if ((num_scans==0) || (scan_type==HOKUYO_SCAN_SPECIAL_ME))
    {
      //read off the LF character
      line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,false);
      if(line_len < 0)
      {
        std::cout << "Hokuyo::GetScan: ERROR: could not read LF (line 3) "<< std::endl;
        return -1;
      }

      if(line_len > 0)
      {
        std::cout << "Hokuyo::GetScan: ERROR: response does not match LF (line 3):" << std::endl;
        _printStrings("\n",line,line_len);
        return -1;
      }
    }
  }
  
  if (need_to_read_off_cmd_and_status)
  {
  
    //read back the echo of the command
    line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,false);
    if(line_len < 0)
    {
      std::cout << "Hokuyo::GetScan: could not read echo of the command (line @1)" << std::endl;
      return -1;
    }
    
    if(strncmp(req,line,line_len-2))     //FIXME: not checking the number of remaining scans
    {
      std::cout << "Hokuyo::GetScan: echo of the command does not match (line @1):"<< std::endl;
      _printStrings(req,line,line_len);
      return -1;
    }

    //read back the status response
    line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,true);
    if(line_len < 0)
    {
      std::cout << "Hokuyo::GetScan: could not read line @2" << std::endl;
      return -1;
    }
    
    if(strncmp(good_status,line,2))
    {
      std::cout << "Hokuyo::GetScan: response does not match (line @2):"<< std::endl;
      _printStrings(good_status,line,line_len);
      return -1;
    }
    //std::cout << "Hokuyo::GetScan: Read off cmd and status"<< std::endl;
  }
    

  //read the timestamp
  line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,true);
  if(line_len < 0)
  {
    std::cout << "Hokuyo::GetScan: could not read timestamp (line @3)" << std::endl;
    return -1;
  }
  
  //read the whole packet
  if (_readPacket(full_packet,_packet_length,timeout_us))
  {
    std::cout << "Hokuyo::GetScan: could not read data packet" << std::endl;
    return -1;
  }
  
  //verify the checksum, and remove LF and checksum chars from data
  int extracted_length=_extractPacket(full_packet, extracted_packet, _packet_length);
  if (extracted_length < 0)
  {
    std::cout << "Hokuyo::GetScan: could not extract data from packet" << std::endl;
    return -1;
  }
  
  //decode the packet into the provided buffer
  LockDataMutex();
  n_range=_decodePacket(extracted_packet, range, extracted_length, encoding);
  if ( (n_range < 0) || (n_range>HOKUYO_MAX_NUM_POINTS))
  {
    std::cout << "Hokuyo::GetScan: returned too many data points" << std::endl;
    //if (scan_ready != NULL) *scan_ready=false;
    _scan_ready=false;    
    UnlockDataMutex();        
    return -1;
  }
  //if (scan_ready != NULL) *scan_ready=true;
  _scan_ready=true;
  
  //signal that the newest data is available
  DataCondSignal();

  UnlockDataMutex();
  
  //update the scan params because the scan was successful
  _scan_start=scan_start;
  _scan_end=scan_end;
  _scan_skip=scan_skip;
  _encoding=encoding;
  _scan_type=scan_type;
  
  //the read was successful, so it means that if we requested streaming, we got it
  if (num_scans==0){
    _streaming=true;
  }
  
  return 0;
}

void Hokuyo::LockDataMutex()
{
  pthread_mutex_lock(&_data_mutex);
}

void Hokuyo::UnlockDataMutex()
{
  pthread_mutex_unlock(&_data_mutex);
}


void Hokuyo::DataCondSignal()
{
  pthread_cond_signal(&_data_cond);
}

//wait until data is ready or time out
int Hokuyo::DataCondWait(int timeout_ms)
{  
  timeval time_start;
  timespec max_wait_time;
  gettimeofday(&time_start,NULL);
  
  max_wait_time.tv_sec=time_start.tv_sec + ((time_start.tv_usec < 1000000-timeout_ms*1000) ? 0:1);
  max_wait_time.tv_nsec=(time_start.tv_usec*1000 + timeout_ms*1000000)%1000000000;
  return pthread_cond_timedwait(&_data_cond,&(_data_mutex),&max_wait_time);
}


//set the desired baud rate of the sensor by sending the appropriate command
int Hokuyo::_setBaudRate(const int baud_rate) 
{
  char request[16];
  char resp1[10];
  char resp2[10];
  char resp3[10];
  char line[HOKUYO_MAX_LINE_LENGTH];
  int numBytes;
  bool check;
  
  if (!sd.IsConnected())
  {
    std::cout << "Hokuyo::_setBaudRate: ERROR: not connected to the sensor!" << std::endl;
    return -1;
  }
  
  switch (baud_rate) 
  {
    case 19200:
      strcpy(request,"SS019200\n");
      break;
    case 115200:
      strcpy(request,"SS115200\n");
      break;
    default:
      std::cout << "Hokuyo::_setBaudRate: invalid baudrate" << std::endl;
      return -1;
  }
  numBytes=9;
  check=true;
  strcpy(resp1,"00\n");
  strcpy(resp2,"03\n");
  strcpy(resp3,"04\n");
  
  // Send the command to Hokuyo
  
  sd.FlushInputBuffer();
  
  if (sd.WriteChars(request,numBytes) != numBytes)
  {
    std::cout << "Hokuyo::_setBaudRate: Unable to send command to hokuyo" << std::endl;
    return -1;
  }
  
  int timeout_us=100000;
  
  // Receive the confirmation that the command was accepted
  int line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,false);
  if( line_len < 0)
  {
    std::cout << "Hokuyo::_setBaudRate: could not read a line" << std::endl;
    return -1;
  }

  if(strncmp(request,line,line_len))
  {
    std::cout << "Hokuyo::_setBaudRate: response does not match:"<< std::endl;
    _printStrings(request,line,line_len);
    return -1;
  }
  
  // Receive the confirmation that the baud rate changed
  line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,true);
  if(line_len < 0)
  {
    std::cout << "Hokuyo::_setBaudRate: could not read a line" << std::endl;
    return -1;
  }

  if( (strncmp(resp1,line,line_len)!=0) && (strncmp(resp2,line,line_len)!=0) && (strncmp(resp3,line,line_len)!=0) )
  {
    std::cout << "Hokuyo::_setBaudRate: response does not match:" << std::endl;
    _printStrings(resp1,line,line_len);
    _printStrings(resp2,line,line_len);
    _printStrings(resp3,line,line_len);
    return -1;
  }
  
  // Set the host terminal baud rate to the test speed
  if(sd.SetBaudRate(baud_rate))
  {
    std::cout << "Hokuyo::_setBaudRate: could not set terminal baud rate" << std::endl;
    return -1;
  }
  
  _baud=baud_rate;
  return 0;
}

//try to get status information from the sensor, using a given baud rate
//this will only succeed if it is able to switch to SCIP2.0 mode
//and send QT (stop laser command) and receive appropriate confirmation,
//described in SCIP2.0 protocol
int Hokuyo::_testBaudRate(const int baud_rate)
{
  
  if (!sd.IsConnected())
  {
    std::cout << "Hokuyo::_testBaudRate: ERROR: not connected to the sensor!" << std::endl;
    return -1;
  }

/*
  if (LaserOff()){
    std::cout << "Hokuyo::_testBaudRate: ERROR: could not turn off the laser!" << std::endl;
    return -1;
  }
*/
  
  for (int i=0;i<HOKUYO_NUM_TEST_BAUD_RETRIES;i++)
  {
    // Attempt to get status information at the current baud 
    std::cout << "Testing " << baud_rate << "..." << std::endl;
    
    // Set the host terminal baud rate to the test speed 
    if(sd.SetBaudRate(baud_rate)==0)
    {
      
      //stop the laser in case it was streaming
      if (LaserOff())
      {
        //std::cout << "Hokuyo::_testBaudRate: ERROR: could not turn off the laser!" << std::endl;
        //return -1;
      }

      // Check if the Hokuyo replies to setting SCIP2.0 
      if (_setSCIP20())
      {
        std::cout << "Hokuyo::_testBaudRate: ERROR: could not send SCIP2.0 command" << std::endl;
        continue;
      }

      //verify that the mode is set to SCIP2.0 by sending the laser off command
      if (LaserOff()==0)
      {
        std::cout << "Hokuyo::_testBaudRate: SCIP2.0 Mode set." << std::endl;
        _baud=baud_rate;
        return 0;
      }
        
      else
        std::cout << "Hokuyo::_testBaudRate: _setSCIP20() failed!"  << std::endl;

    }
    else
      std::cout << "Hokuyo::_testBaudRate: sd.SetBaudRate() failed!" << std::endl;

    usleep(HOKUYO_TEST_BAUD_RATE_DELAY_US);
  }
  
  return -1;
}

int Hokuyo::_getSensorInfoAndParams()
{
  
  if (!sd.IsConnected())
  {
    std::cout << "Hokuyo::_getSensorInfoAndParams: ERROR: not connected to the sensor!" << std::endl;
    return -1;
  }
  
  if  (_getSensorInfo(_mode) != 0) 
  {
    std::cout << "Hokuyo::_getSensorInfoAndParams: Unable to get status!" << std::endl;
    return -1;
  }

  if  (_getSensorParams() != 0) 
  {
    std::cout << "Hokuyo::_getSensorInfoAndParams: Unable to get sensor params!" << std::endl;
    return -1;
  }
  return 0;
}


void Hokuyo::_printLine(char * line, int line_len)
{
  for (int i=0;i<line_len;i++)
  {
    std::cout<<line[i];
  }
  std::cout<<std::endl;
}


//parse sensor information of the format NAME:VALUE;
int Hokuyo::_parseString(char * buf, int buf_len, char cStart, char cStop, std::string name, std::string & value)
{
  int vStart, vStop;
  std::string tempString;
  std::string _name;

  tempString=std::string(buf,buf_len);

  //find the location of the delimiters
  vStart = tempString.find(':') + 1;
  vStop  = tempString.find(';') - 1;
  
  //extract and check the name
  _name = tempString.substr(0,vStart-1);

  //std::cout<<_name<<std::endl;

  if (_name.compare(name) !=0) return -1;

  //extract the string with the value
  value = tempString.substr(vStart, vStop-vStart + 1);

  return 0;
}

//get and parse sensor parameters and store locally
int Hokuyo::_getSensorParams()
{
  char line[HOKUYO_MAX_LINE_LENGTH];
  char req[10];
  char resp[10];
  int numBytes;
  
  if (!sd.IsConnected())
  {
    std::cout << "Hokuyo::_getSensorParams: ERROR: not connected to the sensor!" << std::endl;
    return -1;
  }
  
  sd.FlushInputBuffer();
  
  strcpy(req,"PP\n");
  strcpy(resp,"00\n");
  numBytes=3;
  
  if (sd.WriteChars(req,numBytes) != numBytes)
  {
    std::cout << "Hokuyo::_getSensorParams: could not write request" << std::endl;
    return -1;
  }
  
  int timeout_us=200000;
  
  //read the "PP" back without checksum
  int line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,false);
  if(line_len < 0)
  {
    std::cout << "Hokuyo::_getSensorParams: could not read a line" << std::endl;
    return -1;
  }
  
  if(strncmp(req,line,line_len))
  {
    std::cout << "Hokuyo::_getSensorParams: response does not match:"<<std::endl;
    _printStrings(req,line,line_len);
    return -1;
  }
  
  //read the "00" status back with the checksum
  line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,true);
  if(line_len < 0)
  {
    std::cout << "Hokuyo::_getSensorParams: could not read a line" << std::endl;
    return -1;
  }
  
  if(strncmp(resp,line,line_len))
  {
    std::cout << "Hokuyo::_getSensorParams: response does not match:" << std::endl;
    _printStrings(resp,line,line_len);
    return -1;
  }

#ifdef HOKUYO_PRINT_SENSOR_INFO
  std::cout << std::endl<<"Sensor Params:" <<std::endl;
  std::cout <<"#########################################" <<std::endl;
#endif
  //model
  line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,false);
  if(line_len < 0)
  {
    std::cout << "Hokuyo::_getSensorParams: could not read model line" << std::endl;
    return -1;
  }

  //parse the model information
  if (_parseString(line, line_len, HOKUYO_INFO_DELIM_START, 
      HOKUYO_INFO_DELIM_STOP, std::string(HOKUYO_MODL_STR), _model) != 0)
  {
    std::cout << "Hokuyo::_getSensorParams: could not parse model info" << std::endl;
    return -1;
  }
#ifdef HOKUYO_PRINT_SENSOR_INFO
  _printLine(line,line_len);
#endif
  
  //minimum distance
  line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,false);
  if(line_len < 0)
  {
    std::cout << "Hokuyo::_getSensorParams: could not read dmin line" << std::endl;
    return -1;
  }

  //parse the dmin information
  if (_parseString(line, line_len, HOKUYO_INFO_DELIM_START, 
      HOKUYO_INFO_DELIM_STOP, std::string(HOKUYO_DMIN_STR), _dmin) != 0)
  {
    std::cout << "Hokuyo::_getSensorParams: could not parse dmin info" << std::endl;
    return -1;
  }

#ifdef HOKUYO_PRINT_SENSOR_INFO
  _printLine(line,line_len);
#endif
  
  //maximum distance
  line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,false);
  if(line_len < 0)
  {
    std::cout << "Hokuyo::_getSensorParams: could not read dmax line" << std::endl;
    return -1;
  }

  //parse the dmax information
  if (_parseString(line, line_len, HOKUYO_INFO_DELIM_START, 
      HOKUYO_INFO_DELIM_STOP, std::string(HOKUYO_DMAX_STR), _dmax) != 0)
  {
    std::cout << "Hokuyo::_getSensorParams: could not parse dmax info" << std::endl;
    return -1;
  }

#ifdef HOKUYO_PRINT_SENSOR_INFO
  _printLine(line,line_len);
#endif
  
  //angular resolution
  line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,false);
  if(line_len < 0)
  {
    std::cout << "Hokuyo::_getSensorParams: could not read ares line" << std::endl;
    return -1;
  }

  //parse the ares information
  if (_parseString(line, line_len, HOKUYO_INFO_DELIM_START, 
      HOKUYO_INFO_DELIM_STOP, std::string(HOKUYO_ARES_STR), _ares) != 0)
  {
    std::cout << "Hokuyo::_getSensorParams: could not parse ares info" << std::endl;
    return -1;
  }

#ifdef HOKUYO_PRINT_SENSOR_INFO
  _printLine(line,line_len);
#endif
  
  //minimum angle count
  line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,false);
  if(line_len < 0)
  {
    std::cout << "Hokuyo::_getSensorParams: could not read amin line" << std::endl;
    return -1;
  }

  //parse the amin information
  if (_parseString(line, line_len, HOKUYO_INFO_DELIM_START, 
      HOKUYO_INFO_DELIM_STOP, std::string(HOKUYO_AMIN_STR), _amin) != 0)
  {
    std::cout << "Hokuyo::_getSensorParams: could not parse amin info" << std::endl;
    return -1;
  }
#ifdef HOKUYO_PRINT_SENSOR_INFO
  _printLine(line,line_len);
#endif

  //max angle count
  line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,false);
  if(line_len < 0)
  {
    std::cout << "Hokuyo::_getSensorParams: could not read amax line" << std::endl;
    return -1;
  }

  //parse the amax information
  if (_parseString(line, line_len, HOKUYO_INFO_DELIM_START, 
      HOKUYO_INFO_DELIM_STOP, std::string(HOKUYO_AMAX_STR), _amax) != 0)
  {
    std::cout << "Hokuyo::_getSensorParams: could not parse amax info" << std::endl;
    return -1;
  }

#ifdef HOKUYO_PRINT_SENSOR_INFO
  _printLine(line,line_len);
#endif

  //front angle count
  line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,false);
  if(line_len < 0)
  {
    std::cout << "Hokuyo::_getSensorParams: could not read afrt line" << std::endl;
    return -1;
  }

  //parse the amax information
  if (_parseString(line, line_len, HOKUYO_INFO_DELIM_START, 
      HOKUYO_INFO_DELIM_STOP, std::string(HOKUYO_AFRT_STR), _afrt) != 0)
  {
    std::cout << "Hokuyo::_getSensorParams: could not parse afrt info" << std::endl;
    return -1;
  }

#ifdef HOKUYO_PRINT_SENSOR_INFO
  _printLine(line,line_len);
#endif
  
  //scan rate
  line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,false);
  if(line_len < 0)
  {
    std::cout << "Hokuyo::_getSensorParams: could not read scan line" << std::endl;
    return -1;
  }

  //parse the amax information
  if (_parseString(line, line_len, HOKUYO_INFO_DELIM_START, 
      HOKUYO_INFO_DELIM_STOP, std::string(HOKUYO_SCAN_STR), _scan) != 0)
  {
    std::cout << "Hokuyo::_getSensorParams: could not parse _scan info" << std::endl;
    return -1;
  }

#ifdef HOKUYO_PRINT_SENSOR_INFO
  _printLine(line,line_len);
  std::cout <<"#########################################" <<std::endl<<std::endl;
#endif

  _dist_min = (int)strtol(_dmin.c_str(),NULL,10) / 1000.0;  //convert to meters
  _dist_max = (int)strtol(_dmax.c_str(),NULL,10) / 1000.0;  
  _angle_res = 2 * M_PI / (int)strtol(_ares.c_str(),NULL,10);  //convert from total counts to actual resolution

  //error checking
  if (_dist_min <= 0)
  {
    std::cout << "Hokuyo::_getSensorParams: bad dmin" << std::endl;
    return -1;
  }

  if (_dist_max < _dist_min)
  {
    std::cout << "Hokuyo::_getSensorParams: bad dmax" << std::endl;
    return -1;
  }

  if (_angle_res <= 0)
  {
    std::cout << "Hokuyo::_getSensorParams: bad ares" << std::endl;
    return -1;
  }

  _count_min = (int)strtol(_amin.c_str(),NULL,10);
  _count_max = (int)strtol(_amax.c_str(),NULL,10);
  _count_zero = (int)strtol(_afrt.c_str(),NULL,10);

  if (_count_min > _count_zero)
  {
    std::cout << "Hokuyo::_getSensorParams: bad amin" << std::endl;
    return -1;
  }

  if (_count_max < _count_zero)
  {
    std::cout << "Hokuyo::_getSensorParams: bad amax" << std::endl;
    return -1;
  }

  _angle_min = ( _count_min - _count_zero) * _angle_res;
  _angle_max = ( _count_max - _count_zero) * _angle_res;
  _scan_rate = strtol(_scan.c_str(),NULL,10) / 60.0;   //Hz

  if (_scan_rate < 0)
  {
    std::cout << "Hokuyo::_getSensorParams: bad scan rate" << std::endl;
    return -1;
  }
  
  // read off the rest of the info from the buffer
  usleep(100000);
  sd.FlushInputBuffer();
  
  return 0;
}

//get and parse sensor information and store locally
int Hokuyo::_getSensorInfo(int mode)
{
  char line[HOKUYO_MAX_LINE_LENGTH];
  char req[10];
  char resp[10];
  int numBytes;
  
  if (!sd.IsConnected())
  {
    std::cout << "Hokuyo::_getSensorInfo: ERROR: not connected to the sensor!" << std::endl;
    return -1;
  }
  
  sd.FlushInputBuffer();
  
  strcpy(req,"VV\n");
  strcpy(resp,"00\n");
  numBytes=3;
  
  if (sd.WriteChars(req,numBytes) != numBytes)
  {
    std::cout << "Hokuyo::_getSensorInfo: could not write request" << std::endl;
    return -1;
  }
  
  int timeout_us=200000;

  //read the "VV" back without checksum
  int line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,false);
  if(line_len < 0)
  {
    std::cout << "Hokuyo::_getSensorInfo: could not read a line" << std::endl;
    return -1;
  }
  
  if(strncmp(req,line,line_len))
  {
    std::cout << "Hokuyo::_getSensorInfo: response does not match:"<<std::endl;
    _printStrings(req,line,line_len);
    return -1;
  }
  
  //read the "00" status back with the checksum
  line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,true);
  if(line_len < 0)
  {
    std::cout << "Hokuyo::_getSensorInfo: could not read a line" << std::endl;
    return -1;
  }
  
  if(strncmp(resp,line,line_len))
  {
    std::cout << "Hokuyo::_getSensorInfo: response does not match:" << std::endl;
    _printStrings(resp,line,line_len);
    return -1;
  }

#ifdef HOKUYO_PRINT_SENSOR_INFO
  std::cout << std::endl<<"Sensor Information:" <<std::endl;
  std::cout <<"#########################################" <<std::endl;
#endif
  
  //Vendor
  line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,false);
  if(line_len < 0)
  {
    std::cout << "Hokuyo::_getSensorInfo: could not read vendor line" << std::endl;
    return -1;
  }

  //parse the vendor information
  if (_parseString(line, line_len, HOKUYO_INFO_DELIM_START, 
      HOKUYO_INFO_DELIM_STOP, std::string(HOKUYO_VEND_STR), _vendor) != 0)
  {
    std::cout << "Hokuyo::_getSensorInfo: could not parse vendor info" << std::endl;
    return -1;
  }

#ifdef HOKUYO_PRINT_SENSOR_INFO  
  _printLine(line,line_len);
#endif
  
  //Product
  line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,false);
  if(line_len < 0)
  {
    std::cout << "Hokuyo::_getSensorInfo: could not read product line" << std::endl;
    return -1;
  }

  //parse the product information
  if (_parseString(line, line_len, HOKUYO_INFO_DELIM_START, 
      HOKUYO_INFO_DELIM_STOP, std::string(HOKUYO_PROD_STR), _product) != 0)
  {
    std::cout << "Hokuyo::_getSensorInfo: could not parse product info" << std::endl;
    return -1;
  }

#ifdef HOKUYO_PRINT_SENSOR_INFO
  _printLine(line,line_len);
#endif

  line[line_len-2]=0;  //last two characters are weird so just ignore them
  if (strncmp(line+5,HOKUYO_TYPE_UTM_30LX_STRING,line_len-7)==0)
  {
    _type = HOKUYO_TYPE_UTM_30LX;
    _dist_res = 0.001;
    std::cout<<"*** Sensor identified as HOKUYO TOP-URG UTM_30LX"<<std::endl;
  }
  else if (strncmp(line+5,HOKUYO_TYPE_URG_04LX_STRING,line_len-7)==0)
  {
    _type = HOKUYO_TYPE_URG_04LX;
    _dist_res = 0.001;
    std::cout<<"*** Sensor identified as HOKUYO URG 04LX"<<std::endl;
  }

  else
  {
    std::cout<<"<<<WARNING>>> Sensor could not be identified! Seeting sensor type to URG-04LX"<<std::endl;
    _type = HOKUYO_TYPE_URG_04LX;
  }
  
  //Firmware
  line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,false);
  if(line_len < 0)
  {
    std::cout << "Hokuyo::_getSensorInfo: could not read firmware line" << std::endl;
    return -1;
  }

  //parse the firmware information
  if (_parseString(line, line_len, HOKUYO_INFO_DELIM_START, 
      HOKUYO_INFO_DELIM_STOP, std::string(HOKUYO_FIRM_STR), _firmware) != 0)
  {
    std::cout << "Hokuyo::_getSensorInfo: could not parse firmware info" << std::endl;
    return -1;
  }

#ifdef HOKUYO_PRINT_SENSOR_INFO
  _printLine(line,line_len);
#endif
  
  //Protocol
  line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,false);
  if(line_len < 0)
  {
    std::cout << "Hokuyo::_getSensorInfo: could not read protocol line" << std::endl;
    return -1;
  }

  //parse the protocol information
  if (_parseString(line, line_len, HOKUYO_INFO_DELIM_START, 
      HOKUYO_INFO_DELIM_STOP, std::string(HOKUYO_PROT_STR), _protocol) != 0)
  {
    std::cout << "Hokuyo::_getSensorInfo: could not parse protocol info" << std::endl;
    return -1;
  }

#ifdef HOKUYO_PRINT_SENSOR_INFO
  _printLine(line,line_len);
#endif
  
  //Serial
  line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,false);
  if(line_len < 0)
  {
    std::cout << "Hokuyo::_getSensorInfo: could not read serial number line" << std::endl;
    return -1;
  }

  //parse the serial number information
  if (_parseString(line, line_len, HOKUYO_INFO_DELIM_START, 
      HOKUYO_INFO_DELIM_STOP, std::string(HOKUYO_SERI_STR), _serial) != 0)
  {
    std::cout << "Hokuyo::_getSensorInfo: could not parse serial number info" << std::endl;
    return -1;
  }

#ifdef HOKUYO_PRINT_SENSOR_INFO
  _printLine(line,line_len);
  std::cout <<"#########################################" <<std::endl<<std::endl;
#endif
  
  // read off the rest of the info from the buffer
  usleep(100000);
  sd.FlushInputBuffer();
  
  return 0;
}

//set SCIP2.0 protocol
int Hokuyo::_setSCIP20()
{
  
  if (!sd.IsConnected())
  {
    std::cout << "Hokuyo::_setSCIP20: ERROR: not connected to the sensor!" << std::endl;
    return -1;
  }
  
  //removing this check so that the sensor is always switched to SCIP2.0
  /*
  if (_mode == HOKUYO_SCIP20){
    std::cout << "Hokuyo::_setSCIP20: SCIP2.0 is already set!" << std::endl;
    return 0;
  }
  */
  
  if (sd.WriteChars("SCIP2.0\n",8) != 8)
  {
    std::cout << "Hokuyo::_setSCIP20: could not write request" << std::endl;
    return -1;
  }
  
  char line[HOKUYO_MAX_LINE_LENGTH];
  int timeout_us = 200000;
  
  int line_len=_readLine(line,HOKUYO_MAX_LINE_LENGTH,timeout_us,false);  
  if(line_len < 0)
  {
    std::cout << "Hokuyo::_setSCIP20: could not read a line" << std::endl;
    return -1;
  }
  
  if(strncmp("SCIP2.0",line,line_len))
  {
    std::cout << "Hokuyo::_setSCIP20: response does not match:" << std::endl;
    _printStrings("SCIP2.0",line,line_len);
    return -1;
  }

  usleep(100000);
  sd.FlushInputBuffer();
  _mode=HOKUYO_SCIP20;
  return 0;
}

//sensor info accessor functions
std::string Hokuyo::GetVendor() { return _vendor; }
std::string Hokuyo::GetProduct() { return _product; }
std::string Hokuyo::GetFirmware() { return _firmware; }
std::string Hokuyo::GetProtocol() { return _protocol; }
std::string Hokuyo::GetSerial() { return _serial; }
std::string Hokuyo::GetModel() { return _model; }
double Hokuyo::GetDistMin() { return _dist_min; }
double Hokuyo::GetDistMax() { return _dist_max; }
double Hokuyo::GetAngleRes() { return _angle_res; }
double Hokuyo::GetAngleMin() { return _angle_min; }
double Hokuyo::GetAngleMax() { return _angle_max; }
double Hokuyo::GetScanRate() { return _scan_rate; }
double Hokuyo::GetDistRes() { return _dist_res; }
int    Hokuyo::GetCountZero() { return _count_zero; }
int    Hokuyo::GetCountMin() { return _count_min; }
int    Hokuyo::GetCountMax() { return _count_max; }
int Hokuyo::GetSensorType() { return _type; }

//get the required scan type and skip value from scan name
//skip value will differ from desired skip value when using
//04LX and requesting intensity data. Intensity data requires
//setting a special value of skip, which tells the sensor to
//send back the special packet
int Hokuyo::GetScanTypeAndSkipFromName(int sensorType, char * scanTypeName, int * skip, int * scanType)
{
  
  switch(sensorType)
  {
  
    case HOKUYO_TYPE_URG_04LX:

      *scanType=HOKUYO_SCAN_REGULAR;

      if (strcmp(scanTypeName, HOKUYO_RANGE_STRING) == 0)
      {
        *skip = 1;
        break;
      }
      
      if (strcmp(scanTypeName, HOKUYO_RANGE_INTENSITY_AV_STRING) == 0)
      {
        *skip=HOKUYO_RANGE_INTENSITY_AV;        
        break;
      }

      if (strcmp(scanTypeName, HOKUYO_RANGE_INTENSITY_0_STRING) == 0)
      {
        *skip=HOKUYO_RANGE_INTENSITY_0;        
        break;
      }

      if (strcmp(scanTypeName, HOKUYO_RANGE_INTENSITY_1_STRING) == 0)
      {
        *skip=HOKUYO_RANGE_INTENSITY_1;        
        break;
      }

      if (strcmp(scanTypeName, HOKUYO_INTENSITY_AV_STRING) == 0)
      {
        *skip=HOKUYO_INTENSITY_AV;        
        break;
      }

      if (strcmp(scanTypeName, HOKUYO_INTENSITY_0_STRING) == 0)
      {
        *skip=HOKUYO_INTENSITY_0;        
        break;
      }

      if (strcmp(scanTypeName, HOKUYO_INTENSITY_1_STRING) == 0)
      {
        *skip=HOKUYO_INTENSITY_1;        
        break;
      }

      if (strcmp(scanTypeName, HOKUYO_RANGE_INTENSITY_AV_AGC_AV_STRING) == 0)
      {
        *skip=HOKUYO_RANGE_INTENSITY_AV_AGC_AV;        
        break;      
      }

      if (strcmp(scanTypeName, HOKUYO_RANGE_INTENSITY_0_AGC_0_STRING) == 0)
      {
        *skip=HOKUYO_RANGE_INTENSITY_0_AGC_0;        
        break;
      }

      if (strcmp(scanTypeName, HOKUYO_RANGE_INTENSITY_1_AGC_1_STRING) == 0)
      {
        *skip=HOKUYO_RANGE_INTENSITY_1_AGC_1;        
        break;
      }

      if (strcmp(scanTypeName, HOKUYO_AGC_0_STRING) == 0)
      {
        *skip=HOKUYO_AGC_0;        
        break;
      }

      if (strcmp(scanTypeName, HOKUYO_AGC_1_STRING) == 0)
      {
        *skip=HOKUYO_AGC_0;        
        break;
      }
      
      std::cout << "getScanTypeAndSkipFromName: Error: URG_04-LX does not support this scan mode: " <<scanTypeName<<std::endl;
      return -1;
      
    case HOKUYO_TYPE_UTM_30LX:

      if (strcmp(scanTypeName, HOKUYO_RANGE_STRING) == 0)
      {
        *scanType=HOKUYO_SCAN_REGULAR;  
        *skip = 1;      
        break;
      }

      if (strcmp(scanTypeName, HOKUYO_TOP_URG_RANGE_INTENSITY_STRING) == 0)
      {
        *scanType=HOKUYO_SCAN_SPECIAL_ME;  
        *skip = 1;    
        break;
      }
      
      std::cout << "getScanTypeAndSkipFromName: Error: UTM_30LX does not support this scan mode: " <<scanTypeName<<std::endl;
      return -1;
      
    default:
      std::cout << "getScanTypeAndSkipFromName: Error: unknown sensor type: "<<std::endl;
      return -1;
  }
  return 0;
}

//get number of types of measurements (range, intensity, AGC...) in the given scan
//since a single packet may contain several types of information
int Hokuyo::GetNumOutputArgs(int sensorType, char * scanTypeName)
{
  switch(sensorType)
  {
  
    case HOKUYO_TYPE_URG_04LX:
      if (strcmp(scanTypeName, HOKUYO_RANGE_STRING) == 0)
        return 1;
      if (strcmp(scanTypeName, HOKUYO_RANGE_INTENSITY_AV_STRING) == 0)
        return 2;
      if (strcmp(scanTypeName, HOKUYO_RANGE_INTENSITY_0_STRING) == 0)
        return 2;
      if (strcmp(scanTypeName, HOKUYO_RANGE_INTENSITY_1_STRING) == 0)
        return 2;
      if (strcmp(scanTypeName, HOKUYO_INTENSITY_AV_STRING) == 0)
        return 1;
      if (strcmp(scanTypeName, HOKUYO_INTENSITY_0_STRING) == 0)
        return 1;
      if (strcmp(scanTypeName, HOKUYO_INTENSITY_1_STRING) == 0)
        return 1;
      if (strcmp(scanTypeName, HOKUYO_RANGE_INTENSITY_AV_AGC_AV_STRING) == 0)
        return 3;
      if (strcmp(scanTypeName, HOKUYO_RANGE_INTENSITY_0_AGC_0_STRING) == 0)
        return 3;
      if (strcmp(scanTypeName, HOKUYO_RANGE_INTENSITY_1_AGC_1_STRING) == 0)
        return 3;
      if (strcmp(scanTypeName, HOKUYO_RANGE_INTENSITY_1_AGC_1_STRING) == 0)
        return 3;
      if (strcmp(scanTypeName, HOKUYO_AGC_0_STRING) == 0)
        return 1;
      if (strcmp(scanTypeName, HOKUYO_AGC_1_STRING) == 0)
        return 1;
      
      std::cout << "Error: URG_04-LX does not support this scan type: " <<scanTypeName<<std::endl;
      return -1;
      
    case HOKUYO_TYPE_UTM_30LX:
      if (strcmp(scanTypeName, HOKUYO_RANGE_STRING) == 0)
        return 1;
      if (strcmp(scanTypeName, HOKUYO_TOP_URG_RANGE_INTENSITY_STRING) == 0)
        return 2;
      
      std::cout << "Error: UTM_30LX does not support this scan type: " <<scanTypeName<<std::endl;
      return -1;
      
    default:
      std::cout << "Error: unknown sensor type: "<<std::endl;
      return -1;
  }
}

//useful functions for passing the data to matlab

#ifdef MATLAB_MEX_FILE
void Hokuyo::CreateEmptyOutputMatrices(int nlhs, mxArray * plhs[])
{
  for (int i=0; i<nlhs;i++)
  {
    plhs[i] = mxCreateDoubleMatrix(0, 0, mxREAL);
  }
}

int Hokuyo::CreateOutput(int sensorType, char * scanType, unsigned int * data, uint16_t n_points, int nlhs, mxArray * plhs[])
{
  int numOutArgs=GetNumOutputArgs(sensorType,scanType);
  if (numOutArgs <= 0)
  {
    CreateEmptyOutputMatrices(nlhs,plhs);
    return -1;
  }
  
  int maxNumPoints;
  switch(sensorType)
  {
    case HOKUYO_TYPE_UTM_30LX:
      maxNumPoints=HOKUYO_MAX_NUM_POINTS_UTM_30LX;
      break;
    case HOKUYO_TYPE_URG_04LX:
      maxNumPoints=HOKUYO_MAX_NUM_POINTS_URG_04LX;
      break;
    default:
      std::cout << "createOutput: ERROR: invalid sensor type = " <<n_points << std::endl;
      CreateEmptyOutputMatrices(nlhs,plhs);
      return -1;
  }
  
  if (n_points > maxNumPoints)
  {
    std::cout << "createOutput: ERROR:too many points received = " <<n_points << std::endl;
    CreateEmptyOutputMatrices(nlhs,plhs);
    return -1;
  }
  
  if (n_points % numOutArgs != 0)
  {
    std::cout << "createOutput: ERROR: (number of data points) mod (number of output vars) is not zero!! n_points="<<n_points<<" n_vars="<< numOutArgs <<std::endl;
    CreateEmptyOutputMatrices(nlhs,plhs);
    return -1;
  }
 
  mxArray *out0, *out1, *out2;
  switch (numOutArgs)
  {
    case 1:
      out0=mxCreateDoubleMatrix(n_points,1,mxREAL);
      for (int i=0;i<n_points;i++)
      {
        mxGetPr(out0)[i]=(double)(data[i]);
      }

      plhs[0]=out0;
      break;
      
    case 2:
      out0=mxCreateDoubleMatrix(n_points/2,1,mxREAL);
      out1=mxCreateDoubleMatrix(n_points/2,1,mxREAL);
      
      for (int i=0;i<n_points;i+=2)
      {
        mxGetPr(out0)[i/2]=(double)(data[i]);
        mxGetPr(out1)[i/2]=(double)(data[i+1]);
      }
      
      plhs[0]=out0;
      plhs[1]=out1;
      break;
      
    case 3:
      out0=mxCreateDoubleMatrix(n_points/3,1,mxREAL);
      out1=mxCreateDoubleMatrix(n_points/3,1,mxREAL);
      out2=mxCreateDoubleMatrix(n_points/3,1,mxREAL);
      
      for (int i=0;i<n_points;i+=3)
      {
        mxGetPr(out0)[i/3]=(double)(data[i]);
        mxGetPr(out1)[i/3]=(double)(data[i+1]);
        mxGetPr(out2)[i/3]=(double)(data[i+2]);
      }
      
      plhs[0]=out0;
      plhs[1]=out1;
      plhs[2]=out2;
      break;
      
    default:
      CreateEmptyOutputMatrices(nlhs,plhs);
      return -1;
  }
  
  if (nlhs > numOutArgs)
  {
    for (int i=numOutArgs;i<nlhs;i++)
    {
      plhs[i]=mxCreateDoubleMatrix(0, 0, mxREAL);
    }
  }
  
  return 0;
}


int Hokuyo::ParseScanParams(int sensorType,char * scanTypeName,const mxArray * prhs[],int indexStart, int * start, int * stop, int * skip, int * encoding, int * scanType)
{
  *start = (int)mxGetPr(prhs[indexStart])[0];
  *stop = (int)mxGetPr(prhs[indexStart+1])[0];
  *encoding = (int)mxGetPr(prhs[indexStart+3])[0];

  if ((*encoding != 2) && (*encoding != 3))
  {
    std::cout<<"Please make sure that encoding is valid"<<std::endl;
    return -1;
  }
  
  if ((sensorType==HOKUYO_TYPE_UTM_30LX) && (*encoding==2))
  {
    std::cout<<"parseScanParams: WARNING: 30LX does not support 2-char encoding, setting it to 3 chars"<<std::endl;
    *encoding=3;
  }
      
  *encoding = *encoding==2 ? HOKUYO_2DIGITS : HOKUYO_3DIGITS;

  switch(sensorType)
  {
  
    case HOKUYO_TYPE_URG_04LX:

      *scanType=HOKUYO_SCAN_REGULAR;      

      if ( (*start >= HOKUYO_MAX_NUM_POINTS_URG_04LX) || (*stop >= HOKUYO_MAX_NUM_POINTS_URG_04LX) || (*start>*stop) || (*start < 0) )
      {
        std::cout<<"Please make sure that start and stop params are valid"<<std::endl;
        return -1;
      }

      if (strcmp(scanTypeName, HOKUYO_RANGE_STRING) == 0)
      {
        *skip = (int)mxGetPr(prhs[indexStart+2])[0];
        if (*skip <= 0)
        {
          std::cout <<"Bad skip value: "<<*skip<<std::endl;
          return -1;
        }        
        break;
      }

      if (strcmp(scanTypeName, HOKUYO_RANGE_INTENSITY_AV_STRING) == 0)
      {
        *skip=HOKUYO_RANGE_INTENSITY_AV;        
        break;
      }

      if (strcmp(scanTypeName, HOKUYO_RANGE_INTENSITY_0_STRING) == 0)
      {
        *skip=HOKUYO_RANGE_INTENSITY_0;        
        break;
      }

      if (strcmp(scanTypeName, HOKUYO_RANGE_INTENSITY_1_STRING) == 0)
      {
        *skip=HOKUYO_RANGE_INTENSITY_1;        
        break;
      }

      if (strcmp(scanTypeName, HOKUYO_INTENSITY_AV_STRING) == 0)
      {
        *skip=HOKUYO_INTENSITY_AV;        
        break;
      }

      if (strcmp(scanTypeName, HOKUYO_INTENSITY_0_STRING) == 0)
      {
        *skip=HOKUYO_INTENSITY_0;        
        break;
      }

      if (strcmp(scanTypeName, HOKUYO_INTENSITY_1_STRING) == 0)
      {
        *skip=HOKUYO_INTENSITY_1;        
        break;
      }

      if (strcmp(scanTypeName, HOKUYO_RANGE_INTENSITY_AV_AGC_AV_STRING) == 0)
      {
        *skip=HOKUYO_RANGE_INTENSITY_AV_AGC_AV;        
        break;      
      }

      if (strcmp(scanTypeName, HOKUYO_RANGE_INTENSITY_0_AGC_0_STRING) == 0)
      {
        *skip=HOKUYO_RANGE_INTENSITY_0_AGC_0;        
        break;
      }

      if (strcmp(scanTypeName, HOKUYO_RANGE_INTENSITY_1_AGC_1_STRING) == 0)
      {
        *skip=HOKUYO_RANGE_INTENSITY_1_AGC_1;        
        break;
      }

      if (strcmp(scanTypeName, HOKUYO_AGC_0_STRING) == 0)
      {
        *skip=HOKUYO_AGC_0;        
        break;
      }

      if (strcmp(scanTypeName, HOKUYO_AGC_1_STRING) == 0)
      {
        *skip=HOKUYO_AGC_0;        
        break;
      }
      
      std::cout << "Error: URG_04-LX does not support this scan mode: " <<scanTypeName<<std::endl;
      return -1;
      
    case HOKUYO_TYPE_UTM_30LX:
      
      if ( (*start >= HOKUYO_MAX_NUM_POINTS_UTM_30LX) || (*stop >= HOKUYO_MAX_NUM_POINTS_UTM_30LX) || (*start>*stop) || (*start < 0) )
      {
        std::cout<<"Please make sure that start and stop params are valid"<<std::endl;
        return -1;
      }

      if (strcmp(scanTypeName, HOKUYO_RANGE_STRING) == 0)
      {
        *scanType=HOKUYO_SCAN_REGULAR;  
        *skip = (int)mxGetPr(prhs[indexStart+2])[0];
        if (*skip <= 0)
        {
          std::cout <<"Bad skip value: "<<*skip<<std::endl;
          return -1;
        }        
        break;
      }

      if (strcmp(scanTypeName, HOKUYO_TOP_URG_RANGE_INTENSITY_STRING) == 0)
      {
        *scanType=HOKUYO_SCAN_SPECIAL_ME;  
        *skip = (int)mxGetPr(prhs[indexStart+2])[0];
        if (*skip <= 0)
        {
          std::cout <<"Bad skip value: "<<*skip<<std::endl;
          return -1;
        }    
        break;
      }
      
      std::cout << "Error: UTM_30LX does not support this scan mode: " <<scanTypeName<<std::endl;
      return -1;
      
    default:
      std::cout << "Error: unknown sensor type: "<<std::endl;
      return -1;
  }
  //std::cout <<"New scan parameters are: start=" <<*start <<" stop=" <<*stop <<" skip="<< *skip<<" encoding="<<*encoding<<std::endl;
  return 0;
}

#endif //MATLAB_MEX_FILE

