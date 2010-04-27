
#include <mex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "urg/urg_ctrl.h"


void urg_exit(urg_t *urg, const char *message)
{
	printf("%s: %s\n", message, urg_error(urg));
	urg_disconnect(urg);

	return;
}

void createEmptyOutputMatrices(int nlhs, mxArray * plhs[])
{
    int i;
    for (i=0; i<nlhs;i++)
    {
        plhs[i] = mxCreateDoubleMatrix(0, 0, mxREAL);
    }
}


int createOutput(urg_t * urg,
		unsigned long * data,
		unsigned long timestamp,
		unsigned int n_points,
		int nlhs,
		mxArray * plhs[],
		int giveIntensity)
{
	int numOutArgs;
	if(giveIntensity)
		numOutArgs =  2; //getNumOutputArgs(sensorType, scanType);
	else
		numOutArgs = 1;

    if (numOutArgs <= 0)
    {
        createEmptyOutputMatrices(nlhs,plhs);
    }

    int maxNumPoints = urg_dataMax(urg);


    if (n_points > maxNumPoints)
    {
        printf("createOutput: ERROR:too many points received = %d,\n", n_points);
        createEmptyOutputMatrices(nlhs,plhs);
        return -1;
    }

    if (n_points % numOutArgs != 0)
    {
        printf("createOutput: ERROR: (number of data points) mod (number of output vars) is not zero!! n_points=%d, n_vars=%d\n",
        		n_points, numOutArgs);
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
            	if (i <= 44){
            		mxGetPr(out0)[i] = 0.0;
            	}else {
            		mxGetPr(out0)[i]=(double)(data[i]);
            	}
            }

            plhs[0]=out0;
            break;

        case 2:
            out0=mxCreateDoubleMatrix(n_points/2,1,mxREAL);
            out1=mxCreateDoubleMatrix(n_points/2,1,mxREAL);

            for (i=0;i<n_points;i+=2)
            {
            	if ( i <= 44){
            		mxGetPr(out0)[i/2]=0.0;
            		mxGetPr(out1)[i/2]=0.0;
            	}else{
            		mxGetPr(out0)[i/2]=(double)(data[i]);
            		mxGetPr(out1)[i/2]=(double)(data[i+1]);
            	}
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

	static urg_t urg; //structure for urg control

	int data_max;
	long *data;
	int ret;
	int n;

	long com_baudrate;

    // Get input arguments
    if (nrhs == 0) {
        mexErrMsgTxt("hokuyoAPI: Need input argument");
    }

    const int BUFLEN = 256;
    char buf[BUFLEN];
    if (mxGetString(prhs[0], buf, BUFLEN) != 0) {
        mexErrMsgTxt("hokuyoAPI: Could not read string.");
    }

    if (strcasecmp(buf, "open") == 0){ //conncetion routine for urg device

        if (nrhs != 3){ 
            mexErrMsgTxt("hokuyoAPI: Please enter correct arguments: 'open', <device>, <baud rate>\n");
        }

        switch ((int)mxGetPr(prhs[2])[0]){
            case 19200:
                com_baudrate = 19200;
                break;
            case 115200:
                com_baudrate = 115200;
                break;
            default:
                mexErrMsgTxt("hokuyoAPI: Invalid hokuyo baud rate. Options are 19200, 115200. Using Default.");
                com_baudrate = 115200;
                break;
        }

        /* Connection */
        // If there are 3 args, then the 2nd should be the device 
        if (nrhs == 3){
        	if (mxGetString(prhs[1], buf, BUFLEN) != 0) {
        		mexErrMsgTxt("hokuyoAPI: Could not read string while reading the device name");
        	}

        	ret = urg_connect(&urg, buf, com_baudrate);

        	if (ret < 0) {
        		printf("hokuyoAPI: return urg_connect is < 0, exiting\n");
        		urg_exit(&urg, "urg_connect()");
        	}else{
        		printf("Connected with URG-04LX\n");
        	}
        }else{
        	mexErrMsgTxt("hokuyoAPI: Please enter correct arguments\n");
        }

    }else if(strcasecmp(buf, "getReading")== 0){ //get one reading
    	int getIntensity = 0;

    	if (nrhs == 2){
//    		printf("Intensity reading\n");
    		getIntensity = 1;
    	}else if (nrhs == 1){
//    		printf("normal reading\n");
    	}else{
    		printf("Wrong use. Use 'getReading', or 'getReading', 'intensity' for intensity readings as well\n");
    		return;
    	}

    	//if(urg_isConnected(&urg) == 0){
    		/* Reserve for reception data */
    		data_max = urg_dataMax(&urg);
    		data = (long*)malloc(sizeof(long) * data_max);
    		if (data == NULL) {
    			mexErrMsgTxt("hokuyoAPI: Malloc error!\n");
    		}

    		/* Request for GD data */
    		if(getIntensity){
    			ret = urg_requestData(&urg, URG_GD_INTENSITY, URG_FIRST, URG_LAST);
    		}else{
    			ret = urg_requestData(&urg, URG_GD, URG_FIRST, URG_LAST);
    		}
    		if (ret < 0) {
    			urg_exit(&urg, "urg_requestData()");
    		}

    		/* Reception */
    		n = urg_receiveData(&urg, data, data_max);
//    		printf("# n = %d\n", n);
    		if (n < 0) {
    			printf("Error receiving data\n");
    			urg_exit(&urg, "urg_receiveData()");
    		}else{
    			if (createOutput(&urg, data, urg_recentTimestamp(&urg), n, nlhs, plhs, getIntensity) == -1){
    				printf("Error creating output\n");
    			}
    		}

    		//urg_disconnect(&urg);
    		free(data);

    	//}else{
   // 		mexErrMsgTxt("URG device not connected. Connect using 'open'\n");

    	//}

    }else if(strcasecmp(buf, "close")== 0){
 //   	if(urg_isConnected(&urg)== 0){
    		urg_disconnect(&urg);
    		printf("Closing urg connection\n");
   // 	}else{
    //		mexErrMsgTxt("URG device not connected.\n");
    	//}

    }else if(strcasecmp(buf, "getParameters") == 0){ // opens device, get parameters and closes the connection

    	urg_parameter_t parameters;

        if (nrhs != 3){
             mexErrMsgTxt("hokuyoAPI: Please enter correct arguments: 'getParameters', <device>, <baud rate>\n");
         }

         switch ((int)mxGetPr(prhs[2])[0]){
             case 19200:
                 com_baudrate = 19200;
                 break;
             case 115200:
                 com_baudrate = 115200;
                 break;
             default:
                 mexErrMsgTxt("hokuyoAPI: Invalid hokuyo baud rate. Options are 19200, 115200. Using Default.");
                 com_baudrate = 115200;
                 break;
         }

         /* Connection */
         // If there are 3 args, then the 2nd should be the device
         if (nrhs == 3){
         	if (mxGetString(prhs[1], buf, BUFLEN) != 0) {
         		mexErrMsgTxt("hokuyoAPI: Could not read string while reading the device name");
         	}

         	ret = urg_connect(&urg, buf, com_baudrate);

         	if (ret < 0) {
         		printf("hokuyoAPI: return urg_connect is < 0, exiting\n");
         		urg_exit(&urg, "urg_connect()");
         	}else{
         		printf("Connected with URG-04LX\n");
         	}
         }else{
         	mexErrMsgTxt("hokuyoAPI: Please enter correct arguments\n");
         }

         /* Get sensor parameter */
         ret = urg_parameters(&urg, &parameters);
         printf("urg_getParameters: %s\n", urg_error(&urg));
         if (ret < 0) {
        	 printf("Could not get parameters \n");
        	 urg_disconnect(&urg);
         }

         urg_disconnect(&urg);

         /* Display */
         printf("distance_min: %ld\n", parameters.distance_min_);
         printf("distance_max: %ld\n", parameters.distance_max_);
         printf("area_total: %d\n", parameters.area_total_);
         printf("area_min: %d\n", parameters.area_min_);
         printf("area_max: %d\n", parameters.area_max_);
         printf("area_front: %d\n", parameters.area_front_);
         printf("scan_rpm: %d\n", parameters.scan_rpm_);
         printf("\n");

         /* Display information from URG structure (same resource as above) */
         printf("urg_getDistanceMax(): %ld\n", urg_maxDistance(&urg));
         printf("urg_getDistanceMin(): %ld\n", urg_minDistance(&urg));
         printf("urg_getScanMsec(): %d\n", urg_scanMsec(&urg));
         printf("urg_getDataMax(): %d\n", urg_dataMax(&urg));

    }else if(strcasecmp(buf, "continuousReading") == 0){ //using the MD/MS command

    	// Need an array of pointers to data arrays with scans
    	// Read the data in a for loop n times, put in n matrices.

    	int captureTimes = 1; //default 1
    	int i;

		int timestamp = -1;
    	int previous_timestamp;
    	int remain_times;
		int scan_msec;

    	if (nrhs == 2){ // assume that the capture times are the second argument.
    		captureTimes = (int)(mxGetPr(prhs[1])[0]);
    	}else {
    		printf("Need exactly 2 arguments. Usage: 'continuousReading', <no of captures>\n");
    		return;
    	}

    	urg_parameter_t parameter;


    	/* Reserve for receive buffer */
    	data_max = urg_dataMax(&urg);
    	data = (long*)malloc(sizeof(long) * data_max);
    	if (data == NULL) {
    		printf("Malloc error\n");
    		return;
    	}

    	urg_parameters(&urg, &parameter);
    	scan_msec = urg_scanMsec(&urg);

    	/* Request for MD data */
    	/* To get data continuously for more than 100 times, set capture times equal
    	     to infinity times(UrgInfinityTimes) */
    	/* urg_setCaptureTimes(&urg, UrgInfinityTimes); */

    	urg_setCaptureTimes(&urg, captureTimes);

    	/* Request for data */
    	ret = urg_requestData(&urg, URG_MD, URG_FIRST, URG_LAST);
    	if (ret < 0) {
    		urg_exit(&urg, "urg_requestData()");
    	}

    	for (i = 1; i < captureTimes; ++i) {
    		/* Reception */
    		n = urg_receiveData(&urg, data, data_max);
    		printf("n = %d\n", n);
    		if (n < 0) {
    			urg_exit(&urg, "urg_receiveData()");
    		} else if (n == 0) {
    			printf("n == 0\n");
    			--i;
    			continue;
    		}

    		/* Display the front data with timestamp */
    		/* Delay in reception of data at PC causes URG to discard the data which
    	       cannot be transmitted. This may  results in remain_times to become
    	       discontinuous */
    		previous_timestamp = timestamp;
    		timestamp = urg_recentTimestamp(&urg);
    		remain_times = urg_remainCaptureTimes(&urg);


    		if (remain_times <= 0) {
    			break;
    		}

    	}

    }else{
    	printf("doing nothing\n");
    }

    return;


}
