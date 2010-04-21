/*
 * StereoVision.h
 *
 *  Created on: 17. apr. 2010
 *      Author: anderga
 */

#ifndef STEREOVISION_H_
#define STEREOVISION_H_

#include <opencv/cv.h>
#include <opencv/cxmisc.h>
#include <opencv/cvaux.h>
#include <opencv/highgui.h>

using namespace std;
#include <vector>

enum{
	STEREO_CALIBRATE_BOTH_CAMERAS = 0,
	STEREO_CALIBRATE_INDIVIDUAL_CAMERAS = 1,
};


class StereoVision
{
private:
    //chesboard board corners X,Y, N = X*Y ,  number of corners = (number of cells - 1)
    int cornersX,cornersY,cornersN;
    int sampleCount;
    bool calibrationStarted;
    bool calibrationDone;

    CvSize imageSize;


    vector<CvPoint2D32f> ponintsTemp[2];
    vector<CvPoint3D32f> objectPoints;
    vector<CvPoint2D32f> points[2];
    vector<int> npoints;

public:
    StereoVision(int imageWidth,int imageHeight);
    StereoVision(CvSize size);
    ~StereoVision();

    //matrices resulting from calibration (used for cvRemap to rectify images)
    CvMat *mx1,*my1,*mx2,*my2;

    CvMat* imagesRectified[2];
    CvMat  *imageDepth,*imageDepthNormalized;


    void calibrationStart(int cornersX,int cornersY);
    int calibrationAddSample(IplImage* imageLeft,IplImage* imageRight);
    int calibrationEnd(int flag);

    int calibrationSave(const char* filename);
    int calibrationLoad(const char* filename);

    int stereoProcess(CvArr* imageSrcLeft,CvArr* imageSrcRight);

    CvSize getImageSize(){return imageSize;}
    bool getCalibrationStarted(){return calibrationStarted;}
    bool getCalibrationDone(){return calibrationDone;}
    int getSampleCount(){return sampleCount;}

};


#endif /* STEREOVISION_H_ */
