/* Matlab MEX file to interface to Hokuyo URG-04LX and UTM-30LX driver

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
#include "mex.h"
#include <string>

static Hokuyo *pHokuyo = NULL;

void mexExit(void){
	printf("Exiting hokuyoAPI.\n");
	fflush(stdout);
	if (pHokuyo!=NULL) {
		pHokuyo->Disconnect();
		usleep(100000);
		delete pHokuyo;
		usleep(100000);
	}
}


void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]){
	int ret;
	unsigned int data[HOKUYO_MAX_NUM_POINTS];
  int n_points;
	mxArray *mxRange, *mxIntensityAv, *mxIntensity0, *mxIntensity1, *mxAGCAv, *mxAGC0, *mxAGC1;
	int dims[2], port;
	int start,stop,skip,baud;
	int i;
	int encoding = HOKUYO_3DIGITS;
	
	// Get input arguments
	if (nrhs == 0) {
		mexErrMsgTxt("hokuyoAPI: Need input argument");
	}
	
	const int BUFLEN = 256;
	char buf[BUFLEN];
	if (mxGetString(prhs[0], buf, BUFLEN) != 0) {
		mexErrMsgTxt("hokuyoAPI: Could not read string.");
	}
	
	if (strcasecmp(buf, "open") == 0) {
		if (pHokuyo != NULL) {
			std::cout << "hokuyoAPI: Port is already open \n" << std::endl;
			plhs[0] = mxCreateDoubleScalar(1);
			return;
		}
			
		if (nrhs != 3) mexErrMsgTxt("hokuyoAPI: Please enter correct arguments: 'open', <device>, <baud rate>\n");
		
		switch ((int)mxGetPr(prhs[2])[0]){
			case 19200:
				baud = 19200;
				break;
			case 115200:
				baud = 115200;
				break;
			default:
				mexErrMsgTxt("hokuyoAPI: Invalid hokuyo baud rate. Options are 19200, 115200");
		}
		
		
		// If there are 3 args, then the 2nd should be the device 
		if (nrhs == 3){
			if (mxGetString(prhs[1], buf, BUFLEN) != 0) {
				mexErrMsgTxt("hokuyoAPI: Could not read string while reading the device name");
			}
			
			// create an instance of the driver and initialize 
			pHokuyo = new Hokuyo();
		}
		else {
			mexErrMsgTxt("hokuyoAPI: Please enter correct arguments: ('open',device,baud_rate) \n");
		}
		
		if (pHokuyo->Connect(buf,baud,HOKUYO_SCIP20)){
			std::cerr << "hokuyoAPI: Unable to initialize Hokuyo!!!" << std::endl;
			fflush(stdout);
			delete pHokuyo;
			pHokuyo = NULL;
			plhs[0] = mxCreateDoubleScalar(0);
			return;
		}
		mexAtExit(mexExit);
		
		plhs[0] = mxCreateDoubleScalar(1);
		return;
	}
	
	else if (strcasecmp(buf, "getScan") == 0){
    if (nrhs < 6){
			mexErrMsgTxt("hokuyoAPI: Please enter correct arguments: ('getScan',scanTypeName,start,stop,skip,encoding)\n");
		}
    
    if (pHokuyo == NULL){
			std::cerr << "hokuyoAPI: device is not open!" << std::endl;
			pHokuyo->CreateEmptyOutputMatrices(nlhs, plhs);
			return;
		}
    
    //read scanTypeName
    if (mxGetString(prhs[1], buf, BUFLEN) != 0) {
		  mexErrMsgTxt("hokuyoAPI: Could not read string.");
	  }
    
    int indexParamStart=2;
    int scanStart, scanStop, scanSkip, encoding, scanType;
    
    if (pHokuyo->ParseScanParams(pHokuyo->_type,buf,prhs,indexParamStart, &scanStart, &scanStop, &scanSkip, &encoding, &scanType)){
      mexErrMsgTxt("hokuyoAPI: Error parsing the scan parameters.");
    }
    
    if (!pHokuyo->GetScan(data, n_points, scanStart, scanStop, scanSkip, encoding,scanType,1)) {
			pHokuyo->CreateOutput(pHokuyo->_type,buf,data,n_points,nlhs,plhs);
		} else {
      std::cerr << "hokuyoAPI: error getting points from hokuyo" << std::endl;
			pHokuyo->CreateEmptyOutputMatrices(nlhs, plhs);
		}
		return;
  }
	
	else {
    mexErrMsgTxt("hokuyoAPI: Unknown command. Allowed commands are: 'open', 'getScan'\n");
		//mexErrMsgTxt("hokuyoAPI: Unknown command. Allowed commands are: 'open', 'range', 'range+intensityAv', 'range+intensity0', 'range+intensity1','intensityAv', 'intensity0', 'intensity1', \n'range+intensityAv+AGCAv', 'intensity0+AGC0', 'intensity1+AGC1', 'AGC0', 'AGC1'\n");
	}
	
	plhs[0] = mxCreateDoubleScalar(0);
}
