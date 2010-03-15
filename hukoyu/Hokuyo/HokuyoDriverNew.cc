/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_urglaser urglaser
 * @brief Hokuyo URG laser range-finder

The urglaser driver controls the Hokuyo URG scanning laser range-finder.
Communication with the laser can be either via USB or RS232. The driver
supports only SCIP2.0 protocol

@par Compile-time dependencies

- none

@par Provides

- @ref interface_laser

@par Requires

- none

@par Configuration requests

- PLAYER_LASER_REQ_GET_GEOM
- PLAYER_LASER_REQ_GET_CONFIG
- PLAYER_LASER_REQ_SET_CONFIG
- PLAYER_LASER_REQ_GET_ID

@par Configuration file options

- port (string)
  - Default: "/dev/ttyACM0"
  - Port to which the laser is connected.  Can be either a serial port or
    the port associated with USB acm device.  See use_serial.

- pose (float tuple m m rad)
  - Default: [0.0 0.0 0.0]
  - Pose (x,y,theta) of the laser, relative to its parent object (e.g.,
    the robot to which the laser is attached).

- min_angle, max_angle (angle float)
  - Default: [-135.0 135.0] in degrees
  - Minimum and maximum scan angles to return

- baud (integer)
  - Default: 115200
  - Baud rate to use when communicating with the laser over RS232.  Valid
    rates are: 19200, 57600, and 115200.  The driver will auto-detect the
    current rate then change to the desired rate.

- intensity (integer)
  - Default: 0
  - Specify whether intensity data should be read from the scanner. If
    the scanner is 04-LX, then the resolution step will increase by a factor
    of 3. This is done on the sensor in order to limit the amount of data
    to be transfered. If using 30-LX, then the standard resolution is maintained.
    If intensity is requested on 04-LX, the scan_skip value cannot be changed.

- scan_skip (integer)
  - Default: 1
  - Specify the skip value, which allows one to increase the resolution step
    of the scan. Setting this value to 2 will return half the number of points
    and the for each point, the minimum of the two neighboringmeasurements 
    will be returned. This value cannot be changed and will be ignored when using
    04-LX and reading intensity data.

@par Example

@verbatim
driver
(
  name "hokuyodrivernew"
  plugin "hokuyodrivernew"
  provides ["laser:0"]
  #port "/dev/ttyS1"
  port "/dev/ttyACM0"
  pose [0.0 0.0 0.0]
  min_angle -135.0
  max_angle 135.0
  #use_serial 1
  scan_skip 1
  baud 115200
  #alwayson 1
  #intensity 1
)
@endverbatim

@author Aleksandr Kushleyev. Original driver by Toby Collett, Nico Blodow

*/
/** @} */

#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <math.h>
#include <termios.h>

//#include <replace/replace.h>
using namespace std;

//#include "urg_laser.h"
#include "Hokuyo.hh"


#define URG04_MIN_STEP 44
#define URG04_MAX_STEP 725

#include <libplayercore/playercore.h>

class HokuyoDriverNew : public Driver
{
  public:

  // Constructor;
  HokuyoDriverNew(ConfigFile* cf, int section);
  // Destructor
  ~HokuyoDriverNew();

  // Implementations of virtual functions
  int Setup();
  int Shutdown();

  // This method will be invoked on each incoming message
  virtual int ProcessMessage(QueuePointer & resp_queue, 
                               player_msghdr * hdr,
                               void * data);

  private:
  // Main function for device thread.
  virtual void Main();

  //urg_laser_readings_t * Readings;
  unsigned int * laser_data;
  Hokuyo hokuyo;

  player_laser_data_t Data;
  player_laser_geom_t Geom;
  player_laser_config_t Conf;

  //bool UseSerial;
  int BaudRate;
  float user_min_angle, user_max_angle;
  char * Port;

  int sensor_type;
  int scan_skip;
  int count_zero;
  float min_angle, max_angle;
};


////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
HokuyoDriverNew::HokuyoDriverNew (ConfigFile* cf, int section)
: Driver(cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_LASER_CODE)
{
  //init vars
  memset (&Data, 0, sizeof (Data));
  memset (&Geom, 0, sizeof (Geom));
  memset (&Conf, 0, sizeof (Conf));
  Geom.size.sw = (0.050);
  Geom.size.sl = (0.050);

  laser_data = new unsigned int[HOKUYO_MAX_DATA_LENGTH];
  //Readings = new urg_laser_readings_t;
  assert (laser_data);

  // read options from config file
  Geom.pose.px = (cf->ReadTupleLength (section, "pose", 0, 0));
  Geom.pose.py = (cf->ReadTupleLength (section, "pose", 1, 0));
  Geom.pose.pyaw = (cf->ReadTupleAngle  (section, "pose", 2, 0));

  //get the min/max angles from the config file
  min_angle = cf->ReadAngle (section, "min_angle", DTOR (-135));
  max_angle = cf->ReadAngle (section, "max_angle", DTOR (135));

  
  //read off the whether we want intensity values
  Conf.intensity = cf->ReadInt (section, "intensity", 0) == 1 ? 1 : 0;
   
  //read and check the baud rate. For USB, baud rate does not matter
  //also, use_serial flag is not needed, since nothing changes
  BaudRate = cf->ReadInt (section, "baud", 115200);  
  switch (BaudRate)
  {
    case 115200:
    case 57600:
    case 19200:
      break;
    default:
      PLAYER_WARN1 ("ignoring invalid baud rate %d", BaudRate);
      BaudRate = 115200;
      break;
  }

  Port = strdup (cf->ReadString (section, "port", "/dev/ttyACM0"));

  scan_skip = cf->ReadInt (section, "scan_skip", 1);
  scan_skip = scan_skip > 1 ? scan_skip : 1; 
}

////////////////////////////////////////////////////////////////////////////////
HokuyoDriverNew::~HokuyoDriverNew ()
{
  //delete Readings;
  delete laser_data;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int
  HokuyoDriverNew::Setup ()
{

  if (hokuyo.Connect(Port, BaudRate) < 0)
  {
    this->SetError(1);
    return -1;
  }

  //correct the min/max angles if needed
  float laser_min_angle = hokuyo.GetAngleMin();
  float laser_max_angle = hokuyo.GetAngleMax();
  float laser_res        = hokuyo.GetAngleRes();
  count_zero            = hokuyo.GetCountZero();

  //first limit the angles to the sensor capabilities
  min_angle = min_angle >= laser_min_angle ? min_angle : laser_min_angle;
  max_angle = max_angle <= laser_max_angle ? max_angle : laser_max_angle;

  //add 0.49 to avoid accuracy issues during casting + round
  min_angle = ((int)(min_angle / laser_res - 0.49*laser_res)) * laser_res;
  max_angle = ((int)(max_angle / laser_res + 0.49*laser_res)) * laser_res;
  
  Conf.min_angle = min_angle;
  Conf.max_angle = max_angle;

  //get range info from the sensor
  Conf.max_range = hokuyo.GetDistMax();
  Conf.range_res = hokuyo.GetDistRes();
  
  Conf.scanning_frequency = hokuyo.GetScanRate();

  sensor_type = hokuyo.GetSensorType();

  if (Conf.intensity == 1)
  {
    //URG fixes resolution at 1/3 of normal when requesting rand + intensity
    if (sensor_type == HOKUYO_TYPE_URG_04LX)
    {    
      scan_skip = 3;
    }
  }

  Conf.resolution= laser_res * scan_skip;

  //correct for the edge effects if the scan_skip value is not 1
  int exp_num_points = ceil( (max_angle - min_angle + laser_res) / ( scan_skip * laser_res ) );
  max_angle = exp_num_points*scan_skip*laser_res - laser_res + min_angle;
  Conf.max_angle = max_angle;

  //std::cout<<"expected number of points="<<exp_num_points<<std::endl;

  StartThread ();
  
  return (0);
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
int
  HokuyoDriverNew::Shutdown ()
{
  // Stop and join the driver thread
  StopThread ();

  hokuyo.Disconnect ();

  return (0);
}


////////////////////////////////////////////////////////////////////////////////
int
  HokuyoDriverNew::ProcessMessage (QueuePointer &resp_queue,
                                  player_msghdr * hdr,
                                  void * data)
{
  if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ,
                             PLAYER_LASER_REQ_GET_GEOM, this->device_addr))
  {
    Publish (device_addr,resp_queue, PLAYER_MSGTYPE_RESP_ACK,
             hdr->subtype, &Geom, sizeof (Geom), NULL);
  }
    
  else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ,
                      PLAYER_LASER_REQ_GET_CONFIG, this->device_addr))
  {
    Publish (device_addr,resp_queue, PLAYER_MSGTYPE_RESP_ACK,
             hdr->subtype, &Conf, sizeof (Conf), NULL);
  }
  else if (Message::MatchMessage (hdr, PLAYER_MSGTYPE_REQ,
                      PLAYER_LASER_REQ_GET_ID, this->device_addr))
  {
    player_laser_get_id_config_t player_ID_conf;
    // Get laser identification information
    //player_ID_conf.serial_number = Laser.GetIDInfo ();
    player_ID_conf.serial_number = 0;
    
    Publish (device_addr, resp_queue, PLAYER_MSGTYPE_RESP_ACK, hdr->subtype, 
             &player_ID_conf, sizeof (player_ID_conf), NULL);
  }

  //TODO: implement the ability to change configuration online

  else
    return (-1);
  
  return (0);
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void
  HokuyoDriverNew::Main ()
{

  /*
  timeval timeStart,timeStop;
  gettimeofday(&timeStart,NULL);
  gettimeofday(&timeStop,NULL);
  */  

  // The main loop; interact with the device here
  for (;;)
  {
    /*    
    gettimeofday(&timeStop,NULL);
    double totalTime=(timeStop.tv_sec+timeStop.tv_usec*0.000001)-(timeStart.tv_sec+timeStart.tv_usec*0.000001);
    std::cout << "Hokuyo URG-04LX rate is "<<1/totalTime<<std::endl;
    gettimeofday(&timeStart,NULL);
    */

    // test if we are supposed to cancel
    pthread_testcancel ();

    // Process any pending messages
    ProcessMessages ();


    // update device data
    //Laser.GetReadings (Readings, min_i, max_i);
    int n_data;

    //calculate the start and stop angle counts + round
    int scan_start = (int) ( Conf.min_angle / Conf.resolution * scan_skip + count_zero + 0.49 );
    int scan_end   = (int) ( Conf.max_angle / Conf.resolution * scan_skip + count_zero + 0.49 );

    //std::cout<<"scan start:end = "<<scan_start<<":"<<scan_end<<std::endl;
    
    int ret;
    
    //request just ranges
    if (Conf.intensity == 0 )
    {
      ret = hokuyo.GetScan(laser_data, n_data, scan_start, scan_end, 
                scan_skip, HOKUYO_3DIGITS, HOKUYO_SCAN_REGULAR,0);

      if ( ret == 0)    //successful scan
      {
        Data.min_angle = Conf.min_angle;
        Data.max_angle = Conf.max_angle;
        
        Data.max_range    = Conf.max_range;
        Data.resolution   = Conf.resolution;
        Data.ranges_count = n_data;
        Data.ranges = new float [Data.ranges_count];

        Data.intensity_count = 0;
        
        for (unsigned int i = 0; i < Data.ranges_count; i++)
        {
          Data.ranges[i]  = laser_data[i]/1000.0;
        }
        Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_LASER_DATA_SCAN,&Data);
        delete [] Data.ranges;
      }
    }

    //get the ranges and intensities
    else
    {
      if (sensor_type == HOKUYO_TYPE_URG_04LX)
      {
        ret = hokuyo.GetScan(laser_data, n_data, scan_start, scan_end, 
                HOKUYO_RANGE_INTENSITY_AV_AGC_AV, HOKUYO_3DIGITS, HOKUYO_SCAN_REGULAR,0);
        
        if ( ret == 0)  //successful scan
        {
          Data.min_angle = Conf.min_angle;
          Data.max_angle = Conf.max_angle;
        
          Data.max_range    = Conf.max_range;
          Data.resolution   = Conf.resolution;
          
          //for URG 04-LX, this specific packet contains ranges, intensities, and AGC values
          //AGC= Auto Gain Control. Intensity values are not very useful, since they jump around a lot
          //AGC represents relative intensity better as it is a coarse components of the intensity
          //Where the intensity value, returned by the sensor is the fine component.
          
          //the three measurement types alternate, therefore must be read accordingly                   
          Data.ranges_count = n_data/3;
          Data.intensity_count = n_data/3;

          Data.ranges = new float [Data.ranges_count];
          Data.intensity = new uint8_t [Data.intensity_count];
        
          for (unsigned int i = 0; i < Data.ranges_count; i++)
          {
            Data.ranges[i]     = laser_data[3*i]/1000.0;
            Data.intensity[i]  = (uint8_t)(laser_data[3*i+2]/4); //FIXME: use the range of values better
          }
          Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_LASER_DATA_SCAN,&Data);
          delete [] Data.ranges;
          delete [] Data.intensity;
        }
      }

      else if (sensor_type == HOKUYO_TYPE_UTM_30LX)
      {
        ret = hokuyo.GetScan(laser_data, n_data, scan_start, scan_end, 
                scan_skip, HOKUYO_3DIGITS, HOKUYO_SCAN_SPECIAL_ME,0);
        
        if ( ret == 0)  //successful scan
        {
          Data.min_angle = Conf.min_angle;
          Data.max_angle = Conf.max_angle;
        
          Data.max_range    = Conf.max_range;
          Data.resolution   = Conf.resolution;
          Data.ranges_count = n_data/2;
          Data.intensity_count = n_data/2;
          Data.ranges = new float [Data.ranges_count];
          Data.intensity = new uint8_t [Data.intensity_count];
        
          for (unsigned int i = 0; i < Data.ranges_count; i++)
          {
            Data.ranges[i]     = laser_data[2*i]/1000.0;
            Data.intensity[i]  = (uint8_t)(laser_data[2*i+1]/40); //FIXME: use the range of values better
          }
          Publish (device_addr, PLAYER_MSGTYPE_DATA, PLAYER_LASER_DATA_SCAN,&Data);
          delete [] Data.ranges;
          delete [] Data.intensity;
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Things for building shared object, and functions for registering and creating
//  new instances of the driver
////////////////////////////////////////////////////////////////////////////////

// Factory creation function. This functions is given as an argument when
// the driver is added to the driver table
Driver*
  HokuyoDriverNew_Init(ConfigFile* cf, int section)
{
  // Create and return a new instance of this driver
  return ((Driver*)(new HokuyoDriverNew (cf, section)));
}

// Registers the driver in the driver table. Called from the
// player_driver_init function that the loader looks for

void HokuyoDriverNew_Register (DriverTable* table)
{
  table->AddDriver ("hokuyodrivernew", HokuyoDriverNew_Init);
}

////////////////////////////////////////////////////////////////////////////////
/* need the extern to avoid C++ name-mangling  */
extern "C" {
  int player_driver_init(DriverTable* table)
  {
    puts("HokuyoDriverNew_Register driver initializing");
    HokuyoDriverNew_Register(table);
    puts("HokuyoDriverNew_Register driver done");
    return(0);
  }
}
