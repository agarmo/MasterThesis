/*  Matlab MEX file to interface to Hokuyo URG-04LX and UTM-30LX using the HokuyoReader driver

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

#include <string>
#include "HokuyoReader.hh"
#include "Hokuyo.hh"
#include "mex.h"


static HokuyoReader *pHokuyoReader = NULL;

void mexExit(void){
	printf("Exiting hokuyoReaderAPI.\n");
	fflush(stdout);
	if (pHokuyoReader!=NULL) {
		delete pHokuyoReader;
	}
}


void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]){
	int ret;
	unsigned int data[HOKUYO_MAX_NUM_POINTS];
  int n_points;
	mxArray *mxRange, *mxIntensityAv, *mxIntensity0, *mxIntensity1, *mxAGCAv, *mxAGC0, *mxAGC1;
	int dims[2], port;
	int start,stop,skip,baud;
	int offset=0;
	int i;
	int encoding = HOKUYO_3DIGITS;
	
	// Get input arguments
	if (nrhs == 0) {
		mexErrMsgTxt("hokuyoReaderAPI: Need input argument");
	}
	
	const int BUFLEN = 256;
	char buf[BUFLEN];
	if (mxGetString(prhs[0], buf, BUFLEN) != 0) {
		mexErrMsgTxt("hokuyoReaderAPI: Could not read string.");
	}
	
	if (strcasecmp(buf, "open") == 0) {
		if (pHokuyoReader != NULL) {
			std::cout << "hokuyoReaderAPI: Port is already open \n" << std::endl;
			plhs[0] = mxCreateDoubleScalar(1);
			return;
		}
			
		if (nrhs != 3) mexErrMsgTxt("hokuyoReaderAPI: Please enter correct arguments: open <device> <baud rate>\n");
		
		switch ((int)mxGetPr(prhs[2])[0]){
			case 19200:
				baud = 19200;
				break;
			case 115200:
				baud = 115200;
				break;
			default:
				mexErrMsgTxt("hokuyoReaderAPI: Invalid hokuyo baud rate. Options are 19200, 115200");
		}
		
		
		// 2nd argument should be the device 
		if (nrhs == 3){
			if (mxGetString(prhs[1], buf, BUFLEN) != 0) {
				mexErrMsgTxt("hokuyoReaderAPI: Could not read string while reading the device name");
			}
			
			// create an instance of the driver and initialize 
			pHokuyoReader = new HokuyoReader();
		}
		else {
			mexErrMsgTxt("hokuyoReaderAPI: Please enter correct arguments: ('open',device,baud_rate) \n");
		}
		
		if (pHokuyoReader->Connect(buf,baud)){
			std::cerr << "hokuyoReaderAPI: Unable to initialize Hokuyo!!!" << std::endl;
			fflush(stdout);
			delete pHokuyoReader;
			pHokuyoReader = NULL;
			plhs[0] = mxCreateDoubleScalar(0);
			return;
		}
		mexAtExit(mexExit);
		
		plhs[0] = mxCreateDoubleScalar(1);
		return;
	}
	
  else if (strcasecmp(buf, "setScanSettings") == 0) {
    if (pHokuyoReader == NULL){
			std::cerr << "hokuyoReaderAPI: setScanSettings: device is not open!" << std::endl;
			plhs[0] = mxCreateDoubleMatrix(0, 0, mxREAL);
			return;
		}    

    if (nrhs < 6){
			mexErrMsgTxt("hokuyoReaderAPI: setScanSettings: Please enter correct arguments: ('setScanSettings',scanTypeName,start,stop,skip,encoding)\n");
		}

    //read scanTypeName
    if (mxGetString(prhs[1], buf, BUFLEN) != 0) {
		  mexErrMsgTxt("hokuyoReaderAPI: setScanSettings: Could not read string.");
	  }
    
    int indexParamStart=2;
    int scanStart, scanStop, scanSkip, encoding, scanType;
  
    std::cerr<<"hokuyoReaderAPI: setScanSettings: parsing params"<<std::endl;
    
    if (pHokuyoReader->ParseScanParams(pHokuyoReader->_type,buf,prhs,indexParamStart, &scanStart, &scanStop, &scanSkip, &encoding, &scanType)){
      mexErrMsgTxt("hokuyoReaderAPI: setScanSettings: Error parsing the scan parameters.");
    }

    std::cerr<<"hokuyoReaderAPI: setScanSettings: setting params"<<std::endl;

    if (pHokuyoReader->SetScanParams(buf,scanStart, scanStop, scanSkip, encoding, scanType)){
      mexErrMsgTxt("hokuyoReaderAPI: setScanSettings: Error setting the scan parameters.");
    }

    std::cerr<<"hokuyoReaderAPI: setScanSettings: done"<<std::endl;

    plhs[0] = mxCreateDoubleScalar(1);    
    return;
  }
	
	else if (strcasecmp(buf, "getScan") == 0) {
		if (pHokuyoReader == NULL){
			std::cerr << "hokuyoReaderAPI: device is not open!" << std::endl;
			plhs[0] = mxCreateDoubleMatrix(0, 0, mxREAL);
			return;
		}
		
		if (!pHokuyoReader->GetScan(data, n_points)) {
			pHokuyoReader->CreateOutput(pHokuyoReader->_type,pHokuyoReader->_scan_type_name,data,n_points,nlhs,plhs);
		} else {
      std::cerr << "hokuyoReaderAPI: error getting points from hokuyo" << std::endl;
			pHokuyoReader->CreateEmptyOutputMatrices(nlhs, plhs);
		}
		return;
	}
  
  else if (strcasecmp(buf, "pauseSensor") == 0) {
		if (pHokuyoReader == NULL){
			std::cerr << "hokuyoReaderAPI: device is not open!" << std::endl;
			plhs[0] = mxCreateDoubleScalar(0);
			return;
		}
		
		if (pHokuyoReader->PauseSensor()) {
      std::cerr << "hokuyoReaderAPI: error pausing hokuyo" << std::endl;
			plhs[0] = mxCreateDoubleScalar(0);
      return;
		}
    plhs[0] = mxCreateDoubleScalar(1);
		return;
	}
  
  else if (strcasecmp(buf, "resumeSensor") == 0) {
		if (pHokuyoReader == NULL){
			std::cerr << "hokuyoReaderAPI: device is not open!" << std::endl;
			plhs[0] = mxCreateDoubleScalar(0);
			return;
		}
		
		if (pHokuyoReader->ResumeSensor()) {
      std::cerr << "hokuyoReaderAPI: error resuming hokuyo" << std::endl;
			plhs[0] = mxCreateDoubleScalar(0);
      return;
		}
    plhs[0] = mxCreateDoubleScalar(1);
		return;
	}
  else {
		mexErrMsgTxt("hokuyoReaderAPI: wrong arguments. choices are: open, setScanSettings, getScan, pauseSensor, resumeSensor");
  }
  
}
