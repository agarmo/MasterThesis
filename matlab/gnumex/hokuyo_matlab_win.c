/*!
  \file
  \brief Sample source for data reading from URG in Win32

  \author Satofumi KAMIMURA

  $Id$

  Compiling and Running
  - For Visual Studio 
  - Decompress "capture_sample.zip" folder and select "capture_sample.sln"
  - After project opens in Visual Studio, press F5 to Build and Run.
  - If COM port is not detected, change the com_port in the main.

  - For MinGW, Cygwin etc.
  - % g++ capture_sample.cpp -o capture_sample
  - % ./capture_sample
  - If COM port is not detected, change the com_port in the main.

  \attention Change the comp_port and com_baudrate inside main()
  \attention It is asumed that USB is used for connection, Program will not run properly if serial communication is used.
  \attention Baud rate setting is not required for USB connection and thus not performed.
  \attention Author does not bear responsibility for any loss or damage caused by using this code.
  \attention Bug reports are welcome

*/

#include "hokuyo.c" // dumme matlab mex compiler. 

#include <mex.h> // to create this int a mex file.

static urg_state_t urg_state;

//get number of types of measurements (range, intensity, AGC...) in the given scan
//since a single packet may contain several types of information
int getNumOutputArgs(int sensorType, char * scanTypeName)
{

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

    printf("Error: URG_04-LX does not support this scan type: %d\n",
            *scanTypeName);

    return -1;

}


void createEmptyOutputMatrices(int nlhs, mxArray * plhs[])
{
    int i;
    for (i=0; i<nlhs;i++)
    {
        plhs[i] = mxCreateDoubleMatrix(0, 0, mxREAL);
    }
}

int createOutput(int sensorType, char * scanType, unsigned long * data, unsigned int n_points, int nlhs, mxArray * plhs[])
{
    int numOutArgs=  1; //getNumOutputArgs(sensorType, scanType);
    if (numOutArgs <= 0)
    {
        createEmptyOutputMatrices(nlhs,plhs);
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
            printf("createOutput: ERROR: invalid sensor type =%d\n",n_points);
            createEmptyOutputMatrices(nlhs,plhs);
            return -1;
    }

    if (n_points > maxNumPoints)
    {
        printf("createOutput: ERROR:too many points received = %d,\n", n_points); 
        createEmptyOutputMatrices(nlhs,plhs);
        return -1;
    }

    if (n_points % numOutArgs != 0)
    {
        printf("createOutput: ERROR: (number of data points) mod (number of output vars) is not zero!! n_points=%d, n_vars=%d\n",n_points, numOutArgs);
        createEmptyOutputMatrices(nlhs,plhs);
        return -1;
    }

    mxArray *out0, *out1, *out2;
    int i;
    switch (numOutArgs)
    {
        case 1:
            out0=mxCreateDoubleMatrix(n_points,1,mxREAL);
            for (i=0;i<n_points;i++)
            {
                mxGetPr(out0)[i]=(double)(data[i]);
            }

            plhs[0]=out0;
            break;

        case 2:
            out0=mxCreateDoubleMatrix(n_points/2,1,mxREAL);
            out1=mxCreateDoubleMatrix(n_points/2,1,mxREAL);

            for (i=0;i<n_points;i+=2)
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

            for (i=0;i<n_points;i+=3)
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
            createEmptyOutputMatrices(nlhs,plhs);
            return -1;
    }

    if (nlhs > numOutArgs)
    {
        for (i=numOutArgs;i<nlhs;i++)
        {
            plhs[i]=mxCreateDoubleMatrix(0, 0, mxREAL);
        }
    }
    return 0;
}


void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]){

    int ret;
    unsigned long *data;
    int n_points;
    mxArray *mxRange, *mxIntensityAv, *mxIntensity0, *mxIntensity1, *mxAGCAv, *mxAGC0, *mxAGC1;
    int dims[2], port;
    int start,stop,skip,baud;
    int i;
    int encoding = HOKUYO_3DIGITS;

    int com_baudrate = 115200 ; //default

    urg_state.max_size = sizeof(long)*HOKUYO_MAX_NUM_POINTS_URG_04LX;

    data = malloc(urg_state.max_size);


    // Get input arguments
    if (nrhs == 0) {
        mexErrMsgTxt("hokuyoAPI: Need input argument");
    }

    const int BUFLEN = 256;
    char buf[BUFLEN];
    if (mxGetString(prhs[0], buf, BUFLEN) != 0) {
        mexErrMsgTxt("hokuyoAPI: Could not read string.");
    }


    if (strcasecmp(buf, "open") == 0){ 

        if (nrhs != 3) 
            mexErrMsgTxt("hokuyoAPI: Please enter correct arguments: 'open', <device>, <baud rate>\n");

        switch ((int)mxGetPr(prhs[2])[0]){
            case 19200:
                com_baudrate = 19200;
                break;
            case 115200:
                com_baudrate = 115200;
                break;
            default:
                mexErrMsgTxt("hokuyoAPI: Invalid hokuyo baud rate. Options are 19200, 115200. Using Default.");
        }


        // If there are 3 args, then the 2nd should be the device 
        if (nrhs == 3){
            if (mxGetString(prhs[1], buf, BUFLEN) != 0) {
                mexErrMsgTxt("hokuyoAPI: Could not read string while reading the device name");
            }

            // Connect with URG 
            int ret = urg_connect(&urg_state, buf, 
                    com_baudrate);
            if (ret < 0) {
                // Show error message and end
                printf("hokuyoAPI: return urg_connect is < 0, Error Message: %s\n", 
                        ErrorMessage);

            }else
                printf("Connected with URG-04LX\n");


        }else{
            mexErrMsgTxt("hokuyoAPI: Please enter correct arguments\n");
        }



        ///////////End of the connection open routine/////////////

        //}else if(strcasecmp(buf, "getScan")){
        //need to check if the LRF is connected.



        int CaptureTimes = 1;


        // Data receive using GD-Command
        printf("using GD command\n");

        // It is necessary to switch-on the laser using BM-Command to receive data from GD-Command
        int recv_n = 0;
//        urg_sendMessage("BM", Timeout, &recv_n);

        int i;
        for (i = 0; i < CaptureTimes; ++i) {
            urg_captureByMD(&urg_state, CaptureTimes);
            int n = urg_receiveData(&urg_state, data, urg_state.max_size);
            if (n > 0) {
                printf("front: %ld, urg_timestamp: %d\n",
                        data[urg_state.area_front], urg_state.last_timestamp);
            }
        }

        printf("Max num points: %d, nlhs: %d\n", urg_state.max_size, nlhs);

        if (createOutput(0, buf, data, urg_state.max_size, nlhs, plhs) == -1)
            mexErrMsgTxt("HokuyoAPI: error parsing scan data\n");


        //////////////END OF  THE GETSCAN RUOUTINE////////////


        //    }else if (strcasecmp(buf, "close"))
        // need to switch off the sensor when finnished. 

        recv_n = 0;
        urg_sendMessage("QT", Timeout, &recv_n); //send the QT-command

        urg_disconnect(); //disconnect the com port


    }else{
        int recv_n = 0;
        urg_sendMessage("QT", Timeout, &recv_n); //send the QT-command


        urg_disconnect();
        printf("disconnected the com port\n");
    }
    int recv_n = 0;
    urg_sendMessage("QT", Timeout, &recv_n); //send the QT-command


    urg_disconnect();
    printf("disconnected the com port\n");

    free(data);

    return;


}

/*
int main(int argc, char *argv[]) {

  // COM port setting
  const char* com_port = "COM3";
  const long com_baudrate = 115200;

  // Connect with URG 
  urg_state_t urg_state;
  int ret = urg_connect(&urg_state, com_port, com_baudrate);
  if (ret < 0) {
    // Show error message and end
    printf("%s\n", ErrorMessage);

	// If not terminated immidiately. (Remove if not necessary)
	getchar();
    exit(1);
  }

  int max_size = urg_state.max_size;
  long* data = new long [max_size];

  enum { CaptureTimes = 5 };


  //////////////////////////////////////////////////////////////////////
  // Data receive using GD-Command
  printf("using GD command\n");

  // It is necessary to switch-on the laser using BM-Command to receive data from GD-Command
  int recv_n = 0;
  urg_sendMessage("BM", Timeout, &recv_n);

  for (int i = 0; i < CaptureTimes; ++i) {
    urg_captureByGD(&urg_state);
    int n = urg_receiveData(&urg_state, data, max_size);
    if (n > 0) {
      printf("front: %ld, urg_timestamp: %d\n",
	     data[urg_state.area_front], urg_state.last_timestamp);
    }
  }
  printf("\n");

  /////////////////////////////////////////////////////////////////////
  // Data receive using MD-Command
  printf("using MD command\n");

  urg_captureByMD(&urg_state, CaptureTimes);
  for (int i = 0; i < CaptureTimes; ++i) {
    int n = urg_receiveData(&urg_state, data, max_size);
    if (n > 0) {
      printf("front: %ld, urg_timestamp: %d\n",
	     data[urg_state.area_front], urg_state.last_timestamp);
    }
  }
  // Laser will switch-off automatically after data receive ends using MD-Command

  urg_disconnect();
  delete [] data;

  // If not terminated immidiately. (Remove if not necessary)
  getchar();
  return 0;
}


*/
