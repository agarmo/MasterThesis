/*
 * stereovisiontest.cpp
 *
 *  Created on: 17. apr. 2010
 *      Author: anderga
 */


#include <iostream>
#include <string>
#include <cstdio>


#include "StereoVision.h"
#include "StereoCamera.h"


#include <opencv/highgui.h>

    int sampleTimeout;
    CvMat* imageRectifiedPair;

void displayOutput(StereoCamera * camera, StereoVision * vision){

    CvSize imageSize = vision->getImageSize();
    if(!imageRectifiedPair) imageRectifiedPair = cvCreateMat( imageSize.height, imageSize.width*2,CV_8UC3 );

    vision->stereoProcess(camera->getFramesGray(0), camera->getFramesGray(1));


    CvMat part;
    cvGetCols( imageRectifiedPair, &part, 0, imageSize.width );
    cvCvtColor( vision->imagesRectified[0], &part, CV_GRAY2BGR );
    cvGetCols( imageRectifiedPair, &part, imageSize.width,imageSize.width*2 );
    cvCvtColor( vision->imagesRectified[1], &part, CV_GRAY2BGR );
    for(int j = 0; j < imageSize.height; j += 16 )
    	cvLine( imageRectifiedPair, cvPoint(0,j),cvPoint(imageSize.width*2,j),CV_RGB((j%3)?0:255,((j+1)%3)?0:255,((j+2)%3)?0:255));
    cvShowImage( "rectified", imageRectifiedPair );


    cvShowImage( "depth", vision->imageDepthNormalized );
}




int main(){

	CvSize resolution = cvSize(640, 480);

	StereoCamera *camera = new StereoCamera();

    StereoVision* vision = new StereoVision(resolution);

	int cornersX, cornersY;

	cornersX = 8;
	cornersY = 5;

	int result;

	while(1){

		if(!camera->ready){
			std::cout <<"Connecting to cameras..." << std::endl;
			if(0 != camera->setup(resolution)){
				std::cout << "-FAILED" << std::endl;
			}else{
				std::cout <<"...OK" << std::endl;

				//cvNamedWindow( "left");
				//cvNamedWindow( "right");
				cvNamedWindow( "rectified", 1 );
				cvNamedWindow( "depth", 1 );
				vision->calibrationStart(cornersX, cornersY);
			}
		}else{
			if(0 == camera->capture()){
				cvShowImage("left",camera->render[0]);
				cvShowImage("right",camera->render[1]);
				int c = cvWaitKey( 1 );
				if( c == 27 ) //if esc key break.
					break;

			}



			if(vision->getCalibrationStarted()){ //start the calibration process to rectify the images
				IplImage * left_gray = camera->getFramesGray(0);
				IplImage * right_gray = camera->getFramesGray(1);

				result = vision->calibrationAddSample(left_gray, right_gray);

				if(0 == result){
					std::cout << "+OK" << std::endl;
					if(vision->getSampleCount() >= 10){
						vision->calibrationEnd();
						std::cout << "Calibration Done !" << std::endl;
											}
				}else{
					std::cout <<"-FAIL Try a different position. Chessboard should be visible on both cameras." << std::endl;
				}

			}else{ // Display the depth output.
				if(vision->getCalibrationDone()) displayOutput(camera, vision);
			}


		}
	}



	cvDestroyAllWindows();
	cvReleaseMat(&imageRectifiedPair);

    delete vision;
    delete camera;

    return 0;
}
