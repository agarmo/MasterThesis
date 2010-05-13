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
	STEREO_MATCH_BY_BM = 2,
	STEREO_MATCH_BY_GC = 3
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

    CvMat * Q; //perspective transformation matrix created by stereoRectify

    CvMat* imagesRectified[2];
    CvMat  *imageDepth,*imageDepthNormalized;

    CvMat * disp_gc[2];


    void calibrationStart(int cornersX,int cornersY);
    int calibrationAddSample(IplImage* imageLeft,IplImage* imageRight);
    int calibrationEnd(int flag, CvMat* dist1, CvMat* cam1, CvMat* dist2, CvMat* cam2,CvMat* fundamentalMat );

    int calibrationSave(const char* filename);
    int calibrationLoad(const char* filename);

    int stereoProcess(CvArr* imageSrcLeft,CvArr* imageSrcRight, int match);

    CvSize getImageSize(){return imageSize;}
    bool getCalibrationStarted(){return calibrationStarted;}
    bool getCalibrationDone(){return calibrationDone;}
    int getSampleCount(){return sampleCount;}

    void reCalibrate(int cornersX, int cornersY);

};


#endif /* STEREOVISION_H_ */
