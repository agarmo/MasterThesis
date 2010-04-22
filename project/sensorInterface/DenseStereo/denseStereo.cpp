/*
 * denseStereo.cpp
 *
 *  Created on: 17. apr. 2010
 *      Author: anderga
 */

#include <iostream>

#include <mex.h>

#include "StereoVision.h"
#include "StereoCamera.h"


void createCamMatOutput(StereoVision * vision,
		CvMat* C1, CvMat* C2, CvMat* D1, CvMat* D2, CvMat* F,
		int nlhs,
		mxArray * plhs[])
{




}


void doCalibration(	StereoCamera *camera,
		StereoVision *vision,
		CvSize resolution,
		int iterations, //number of pictures of the chessboard
		int flag,
		int nlhs,
		mxArray * plhs[])
{

	bool calibrationDone = false;

	CvMat C1, C2, D1, D2, F;

	int cornersX, cornersY;

	cornersX = 8;
	cornersY = 5;

	int result;

	while(!calibrationDone){
		if(!camera->ready){
			std::cout <<"Connecting to cameras..." << std::endl;
			if(0 != camera->setup(resolution)){
				mexErrMsgTxt("-FAILED\n");
			}else{
				//std::cout <<"...OK" << std::endl;
				cvNamedWindow( "left");
				cvNamedWindow( "right");
				vision->calibrationStart(cornersX, cornersY);
			}
		}else{
			if(0 == camera->capture()){
				if(!vision->getCalibrationDone()){
					cvShowImage("left",camera->frames[0]);
					cvShowImage("right",camera->frames[1]);
				}
				int c = cvWaitKey( 1 );
				if( c == 27 ) {//if esc key break.
					mexErrMsgTxt("StereoCamAPI: canceled by user\n");
					break;
				}
			}


			if(vision->getCalibrationStarted()){//start the calibration process to rectify the images

				result = vision->calibrationAddSample(
									camera->getFramesGray(0),
									camera->getFramesGray(1));

				if(0 == result){
//					std::cout << "+OK" << std::endl;
					if(vision->getSampleCount() >= iterations){
						vision->calibrationEnd(flag, &D1, &C1, &D2, &C2, &F);

						std::cout << "Calibration Done !" << std::endl;
						calibrationDone = true;

						//output the matrices


					}
				}else{
					std::cout <<"-FAIL Try a different position. Chessboard should be visible on both cameras." << std::endl;
				}

			}
		}

	}
	cvDestroyAllWindows();
	//cvReleaseMat(&imageRectifiedPair);

}

void loadCalibration(){


}


//function for getting the disparity map.
void getDisparityMap(){



}

// return the 3d coordinates from the scene.
void get3DOutuput(){


}



/*
 * Main entry point for the dense stereo algorithm.
 * The input to this function should be some parameters and the two
 * images that are captured. The images need to be rectified.
 *
 * Output should be 3 lists with x,y,z of matched features in the images.
 *
 * Usage:
 * 	[x, y, z, cameraParameters]  = denseStereo(<command>, <option>, <parameters>)
 *
 * Input:
 * 		<command>:
 * 			- calibrate 	-> output camera matrix, distortion coefficients,
 * 								 extrincic and fundamental matrix.
 * 			- getReading 	-> input camera parameters
 * 							-> output x,y,z or disparity
 * 		<option>:
 * 			- disparity 	-> output disparity
 * 			- 3dpoints		-> output 3d points relative to camera.
 *
 * 		<parameters>:
 * 			- camera matrix -> two matrices
 * 			- distrotion	-> two vectors
 * 			- extrincic		-> two matrices
 *
 * Output:
 * 		x, y, z - array with points
 * 		camera parameters
 *
 *
 *
 */

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]){

	CvSize resolution = cvSize(640,480);

	StereoCamera* camera = new StereoCamera();

	StereoVision* vision = new StereoVision(resolution);


    // Get input arguments
    if (nrhs == 0) {
        mexErrMsgTxt("StereoDense: Need input argument");
    }

    const int BUFLEN = 256;
    char buf[BUFLEN]; //string buffer

    if (mxGetString(prhs[0], buf, BUFLEN) != 0) {
        mexErrMsgTxt("StereoCamAPI: Could not read string.");
        //do cleanup
        delete camera;
        delete vision;
        return;
    }

    if (strcasecmp(buf, "calibrate") == 0){ //calibration routine of the camera.

    	//check if there are right number of output elements.

    	if(nlhs != 7){ //need seven output arguments for storing camera parameters
    		mexErrMsgTxt("StereoCamAPI: Need 7 output argumetns\n"
				"Usage: <cam1 mat, cam2 mat, dist1, dist2, ext1, ext2, fundamental matrix\n");

    	}else{ //start the collection and distribution.



    	}

    }


    //do cleanup

    delete camera;
    delete vision;

    return;

}

