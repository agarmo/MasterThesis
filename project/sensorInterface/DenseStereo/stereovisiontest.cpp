/*
 * DenseStereo.cpp
 *
 *  Created on: 17. apr. 2010
 *      Author: anderga
 */


#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>

#include <fstream>
#include <iomanip> // for manipulating streams



#include "StereoVision.h"
#include "StereoCamera.h"

#include <opencv/highgui.h>

int sampleTimeout;
CvMat* imageRectifiedPair;

std::string filename = "c:\\test2.txt";

void displayOutput(StereoCamera * camera, StereoVision * vision){

    CvSize imageSize = vision->getImageSize();
    if(!imageRectifiedPair)
    	imageRectifiedPair = cvCreateMat( imageSize.height, imageSize.width*2,CV_8UC3 );

    vision->stereoProcess(camera->getFramesGray(0), camera->getFramesGray(1), STEREO_MATCH_BY_BM);


    CvMat part;
    cvGetCols( imageRectifiedPair, &part, 0, imageSize.width );
    cvCvtColor( vision->imagesRectified[0], &part, CV_GRAY2BGR );
    cvGetCols( imageRectifiedPair, &part, imageSize.width,imageSize.width*2 );
    cvCvtColor( vision->imagesRectified[1], &part, CV_GRAY2BGR );

    for(int j = 0; j < imageSize.height; j += 16 )
    	cvLine( imageRectifiedPair, cvPoint(0,j),
    			cvPoint(imageSize.width*2,j),
    			CV_RGB((j%3)?0:255,((j+1)%3)?0:255,
    					((j+2)%3)?0:255));

    cvShowImage( "rectified", imageRectifiedPair );

    cvShowImage( "depth", vision->imageDepthNormalized );
}


int calibrationLoad(StereoVision* vision, std::string filename)
{
    std::cout << "Loading calibration file..." << filename;
    if(0 == vision->calibrationLoad(filename.c_str())){
        std::cout << " OK" << std::endl;
        return 0;
    }else{
        std::cout << " -FAIL" << std::endl;
        return 1;
    }
}

void calibrationSave(StereoVision* vision, std::string filename)
{
    std::cout << "Saving calibration file..." << filename;
    if(0 == vision->calibrationSave(filename.c_str())){
        std::cout << " OK" << std::endl;
    }else{
        std::cout <<" -FAIL"<< std::endl;
    }
}

void writeToFile(IplImage* _3dimage){

	//open the file

	FILE *File;

	File = fopen(filename.c_str(), "a");

	std::cout << "Check image atributes" << std::endl;

	if (_3dimage->depth == IPL_DEPTH_16S)
		std::cout << "Depth of image is signed 16 bit integer" << std::endl;
	else if (_3dimage->depth == IPL_DEPTH_32F)
		std::cout << "Depth of image is 32 bit float" << std::endl;
	else if (_3dimage->depth == IPL_DEPTH_8S)
		std::cout << "Depth of image is signed 8 bit integer" << std::endl;

	std::cout << "Number of channels in the image is " << _3dimage->nChannels << std::endl;

	if( _3dimage->dataOrder == IPL_DATA_ORDER_PIXEL)
		std::cout << "The dataorder of the image is Ordered Pixel" << std::endl;
	else if (_3dimage->dataOrder == IPL_DATA_ORDER_PLANE)
		std::cout << "The dataorder of the image is Ordered Plane" << std::endl;




	std::cout << "Starting to write file...";


	if (File != NULL){
		//access the image data.
		for (int y = 0; y < _3dimage->height; y++){
			float * ptr = (float*)(_3dimage->imageData + y*_3dimage->widthStep);
			for (int x = 0; x < _3dimage->width; x++){
/*
				File << fixed << std::setprecision(1) << (int)ptr[3*x] << "," <<
						fixed << std::setprecision(1) << (int)ptr[3*x+1] << "," <<
						fixed << std::setprecision(1) << (int)ptr[3*x+2];
				if ( x == _3dimage->width-1)
					File << " ";
				else
					File << ",";

					*/
				fprintf(File, "%f,%f,%f,", (float)ptr[3*x], (float)ptr[3*x+1], (float)ptr[3*x+2]);

			}
			fprintf(File, "\n");
		}
	}
	fclose(File);
	std::cout << " Done!" << std::endl;


}


void create3dOutput(StereoCamera *camera, StereoVision * vision, IplImage * dest){
	CvSize imageSize = vision->getImageSize();
	//if(!imageRectifiedPair)
		//imageRectifiedPair = cvCreateMat( imageSize.height, imageSize.width*2,CV_8UC3 );

	//process the stereo
	//vision->stereoProcess(camera->getFramesGray(0), camera->getFramesGray(1), STEREO_MATCH_BY_BM);

	//cvShowImage("left", camera->frames[0]);

	if (vision->Q == 0){
		std::cout << "Q matrix not set."<< std::endl;
		cvSetIdentity(vision->Q);
	}else if( vision->imageDepth == 0){
		std::cout << "Depth Image not set" << std::endl;
	}else{

		CvMat *realDisp = cvCreateMat(vision->imageDepth->height,
				vision->imageDepth->width,
				CV_16SC1);
		cvConvertScale( vision->imageDepth, realDisp, 1.0/16, 0 );

		cvReprojectImageTo3D(realDisp, dest, vision->Q, true);
//		cvSave("c:\\reprojection_matrix_Q.xml", vision->Q);
//		cvSave("c:\\resultImage3d.xml", dest);

		writeToFile(dest);
	}

	//cvShowImage( "depth", dest );
}


int main(int argc, char* argv[]){

	bool usefile = false;
	bool saveCalibration = false;

	bool writefile = false;
	int iterator = 0;

	CvMat C1, C2, D1, D2, F;

	IplImage *img3d = 0;

	CvSize resolution = cvSize(640,480);

	StereoCamera *camera = new StereoCamera(resolution);

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
					else if (c == 'c'){
						vision->reCalibrate(cornersX, cornersY);
						std::cout << "Recalibrate" << std::endl;
					}else if (c == 's'){
						writefile = true;
						std::cout << "User chose to write file"<< std::endl;
					}else if (c == 'l'){
						std::cout << "Loading calibration... " << std::endl;
						calibrationLoad(vision, "calibration1.xml");
						std::cout << "Loading calibration... Done!" << std::endl;
					}
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
						if(vision->getSampleCount() >= 30){
							vision->calibrationEnd(
									STEREO_CALIBRATE_BOTH_CAMERAS,
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
					displayOutput(camera, vision);

					if (writefile && iterator <= 4){
						if (img3d == 0)
							img3d = cvCreateImage( resolution, IPL_DEPTH_32F, 3 );

						create3dOutput(camera, vision, img3d);
						iterator++;
					}
					if (iterator > 4){
						writefile = false;
						iterator = 0;
					}


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
