#include "HokuyoReader.hh"

//#define HOKUYO_DEVICE "/dev/ttyACM0"
#define HOKUYO_DEVICE "/dev/cu.usbmodem0000103D1"

//SCAN_NAME is used to determine the scanType and scanSkip values
//See below for details

#define SCAN_NAME "range"
//#define SCAN_NAME "top_urg_range+intensity"
//#define SCAN_NAME "range+intensity1+AGC1"

int main(){
  unsigned int data[HOKUYO_MAX_NUM_POINTS];
  int n_data;          //number of returned points
  int scanStart=44;      //start of the scan
  int scanEnd=725;      //end of the scan
  int scanSkip=1;       //this is so-called "cluster count", however special values
                        //of this variable also let request Intensity and AGC values
                        //from URG-04LX. (see the intensity mode manual and Hokuyo.hh)

                        //If the scanner is 04LX and the scan name is not
                        //"range", then this skip value will be overwritten with the appropriate
                        //value in order to request the special scan. 
                        //See call to getScanTypeAndSkipFromName() below
                        
                        //Thus, for 04LX the skip (cluster)
                        //value is only effective if the scan name is "range". For 30LX, both
                        //"range" and "top_urg_range+intensity" should allow setting skip value
  

  int encoding=HOKUYO_3DIGITS; //HOKUYO_2DIGITS
                               //2 or 3 char encoding. 04LX supports both, but 30LX only 3-char
                               //2-char encoding reduces range to 4meters, but improves data
                               //transfer rates over standard serial port

  int scanType;                //scan type specifies whether a special scan is required, 
                               //such as HOKUYO_SCAN_SPECIAL_ME - for URG-30LX intensity mode
                               //otherwise use HOKUYO_SCAN_REGULAR. This will be automatically
                               //acquired by the getScanTypeAndSkipFromName() function below
  
  char device[128];           //the device of the scanner, for example "/dev/ttyACM0"
  strcpy(device,HOKUYO_DEVICE);

  int baud=115200;            //communication baud rate (does not matter for USB connection)

  char scanName[128];        //name of the scan - see Hokuyo.hh for allowed types
  strcpy(scanName,SCAN_NAME); 

  

  HokuyoReader hokuyoReader;
  if (hokuyoReader.Connect(device,baud)){
    std::cerr<<"testHokuyoReader: could not connect to the sensor."<<std::endl;
    exit(-1);
  }

	std::cout<<hokuyoReader.GetVendor()<<std::endl<<hokuyoReader.GetProduct()<<std::endl<<hokuyoReader.GetFirmware()
					 <<std::endl<<hokuyoReader.GetProtocol()<<std::endl<<hokuyoReader.GetSerial()<<std::endl;


	std::cout<<"---------------"<<std::endl;
	std::cout<<hokuyoReader.GetModel()<<std::endl
					 <<hokuyoReader.GetDistMin()<<std::endl
					 <<hokuyoReader.GetDistMax()<<std::endl
					 <<hokuyoReader.GetAngleRes()<<std::endl
					 <<hokuyoReader.GetAngleMin()<<std::endl
					 <<hokuyoReader.GetAngleMax()<<std::endl
					 <<hokuyoReader.GetScanRate()<<std::endl<<std::endl;
	

  int sensorType= hokuyoReader._type;
  int newSkip;

  //get the special skip value (if needed) and scan type, depending on the scanName and sensor type
  if (hokuyoReader.GetScanTypeAndSkipFromName(sensorType, scanName, &newSkip, &scanType)){
    std::cerr<<"testHokuyoReader: getScanTypeAndSkipFromName: Error getting the scan parameters."<<std::endl;
    exit(-1);
  }

  if (newSkip!=1){            //this means that a special value for skip must be used in order to request
    scanSkip=newSkip;         //a special scan from 04LX. Otherwise, just keep the above specified value of 
  }                           //skip

  //set the scan parameters
  if (hokuyoReader.SetScanParams(scanName,scanStart, scanEnd, scanSkip, encoding, scanType)){
    std::cerr<<"testHokuyoReader: setScanSettings: Error setting the scan parameters."<<std::endl;
    exit(-1);
  }


  //get the number of output arguments (number of types of data that will be returned)
  //Since there is only one pointer provided to Getscan function, all the data will be written to that array.
  //So, if there is range and intensity data, it will be alternating and will have to be separated from
  //the array that looks like this: 1212121212 (where 1 represents first data type, 2 - second).
  //For 04LX it is possible to have 3 types of data, so they will come like this : 123123123123..
  int numOutputArgs=hokuyoReader.GetNumOutputArgs(sensorType,scanName);
  if (numOutputArgs < 0){
    std::cerr<<"testHokuyoReader: bad number of output args.."<<std::endl;
    return -1;
  }
  std::cout << "Number of output arguments = " << numOutputArgs <<std::endl;

  for (int i=0;i<10;i++){
    if (hokuyoReader.GetScan(data, n_data)){
      std::cerr<<"testHokuyoReader: scan failed"<<std::endl;
      //return -1;
    }
    else
      std::cerr<<"testHokuyoReader: iteration #"<<i<<" : scan succeeded! got " <<n_data<<" points"<<std::endl;
  }

  hokuyoReader.Disconnect();
  
  return 0;
}
