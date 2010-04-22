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

    vision->stereoProcess(camera->getFramesGray(0), camera->getFramesGray(1), STEREO_MATCH_BY_BM);


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


int calibrationLoad(StereoVision* vision, std::string filename)
{
    std::cout << "Loading calibration file..." << filename << std::endl;;
    if(0 == vision->calibrationLoad(filename.c_str())){
        std::cout << "OK" << std::endl;
        return 0;
    }else{
        std::cout << "-FAIL" << std::endl;
        return 1;
    }
}

void calibrationSave(StereoVision* vision, std::string filename)
{
    std::cout << "Saving calibration file..." << filename << std::endl;
    if(0 == vision->calibrationSave(filename.c_str())){
        std::cout << "OK" << std::endl;
    }else{
        std::cout <<"-FAIL"<< std::endl;
    }
}


void create3dOutput(StereoCamera *camera, StereoVision * vision, IplImage * dest){
	CvSize imageSize = vision->getImageSize();
	if(!imageRectifiedPair)
		imageRectifiedPair = cvCreateMat( imageSize.height, imageSize.width*2,CV_8UC3 );

	//process the stereo
	vision->stereoProcess(camera->getFramesGray(0), camera->getFramesGray(1), STEREO_MATCH_BY_BM);

	cvShowImage("left", camera->frames[0]);


	cvReprojectImageTo3D(vision->imageDepth, dest, vision->Q, 0);

	//cvShowImage( "depth", dest );
}



/*int main(){

	bool usefile = false;
	bool saveCalibration = false;

	CvMat C1, C2, D1, D2, F;

	IplImage *img3d = 0;

	CvSize resolution = cvSize(640,480);

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

				cvNamedWindow( "left");
				cvNamedWindow( "right");
				cvNamedWindow( "rectified", 1 );
				cvNamedWindow( "depth", 1 );
				vision->calibrationStart(cornersX, cornersY);
			}
		}else{
			if(0 == camera->capture()){
				if(!vision->getCalibrationDone()){
					cvShowImage("left",camera->frames[0]);
					cvShowImage("right",camera->frames[1]);
				}
				int c = cvWaitKey( 1 );
					if( c == 27 ) //if esc key break.
						break;

			}


			if(vision->getCalibrationStarted()){//start the calibration process to rectify the images

				if (usefile ){
					if(calibrationLoad(vision, "calibration.xml") == 0){
						std::cout << "Calibration Done !" << std::endl;
						usefile = false;
					}
				}else{

					IplImage * left_gray = camera->getFramesGray(0);
					IplImage * right_gray = camera->getFramesGray(1);

					result = vision->calibrationAddSample(left_gray, right_gray);

					if(0 == result){
						std::cout << "+OK" << std::endl;
						if(vision->getSampleCount() >= 15){
							vision->calibrationEnd(
									STEREO_CALIBRATE_INDIVIDUAL_CAMERAS,
									&D1, &C1, &D2, &C2, &F);
							std::cout << "Calibration Done !" << std::endl;
							if(saveCalibration)
								calibrationSave(vision, "calibration");
						}
					}else{
						std::cout <<"-FAIL Try a different position. Chessboard should be visible on both cameras." << std::endl;
					}
				}

			}else{ // Display the depth output.
				if(vision->getCalibrationDone()){
					//displayOutput(camera, vision);

					if (img3d == 0)
							img3d = cvCreateImage(resolution, IPL_DEPTH_16S, 3);

					create3dOutput(camera, vision, img3d);
					cvShowImage("depth", img3d);

				}
			}


		}
	}



	cvDestroyAllWindows();
	cvReleaseMat(&imageRectifiedPair);
	cvReleaseImage(&img3d);



    delete vision;
    delete camera;

    return 0;
}
*/
