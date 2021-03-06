/*
 * StereoCamera.h
 *
 *  Created on: 17. apr. 2010
 *      Author: anderga
 */

#ifndef STEREOCAMERA_H_
#define STEREOCAMERA_H_

#include "opencv/cv.h"
#include "opencv/cxmisc.h"
#include "opencv/cvaux.h"
#include "opencv/highgui.h"



class StereoCamera
{
    CvCapture* captures[2];

    CvSize imageSize;

public:

    IplImage* frames[2];
    IplImage* framesGray[2];

    //IplImage* render[2];

    CvMat D1, D2, R1, R2; //used for parameter output and saving of estimated paramters.


    StereoCamera();
    StereoCamera(CvSize size);
    ~StereoCamera();

    int setup(CvSize imageSize);
    bool ready;
    int capture();
    IplImage* getFramesGray(int lr);

};


#endif /* STEREOCAMERA_H_ */
