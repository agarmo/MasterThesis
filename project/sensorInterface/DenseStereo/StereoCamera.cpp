/*
 * StereoCamera.cpp
 *
 *  Created on: 17. apr. 2010
 *      Author: anderga
 */

#include "StereoCamera.h"


StereoCamera::StereoCamera()
{
    for(int lr=0;lr<2;lr++){
        captures[lr] = 0;
        frames[lr] = 0;
        framesGray[lr] =0;

    }
    ready = false;
}

StereoCamera::StereoCamera(CvSize size)
{
    for(int lr=0;lr<2;lr++){
        captures[lr] = 0;
        frames[lr] = 0;
        framesGray[lr] =0;

    }
    imageSize = size;

    ready = false;
}



StereoCamera::~StereoCamera()
{
    for(int lr=0;lr<2;lr++){
        cvReleaseImage(&frames[lr]);
        cvReleaseImage(&framesGray[lr]);
        cvReleaseCapture(&captures[lr]);
        //cvReleaseImage(&render[lr]);
    }

}

int StereoCamera::setup(CvSize imageSize){
    this->imageSize = imageSize;

    captures[0] = cvCaptureFromCAM(0);
    captures[1] = cvCaptureFromCAM(1);

/*    frames[0] = cvCreateImage(imageSize,IPL_DEPTH_8U,3);
    framesGray[0] = cvCreateImage(imageSize,IPL_DEPTH_8U,1);
    frames[1] = cvCreateImage(imageSize,IPL_DEPTH_8U,3);
    framesGray[1] = cvCreateImage(imageSize,IPL_DEPTH_8U,1);
*/
    if( (captures[0]) && (captures[1])){

        for(int i=0;i<2;i++){
                cvSetCaptureProperty(captures[i] ,CV_CAP_PROP_FRAME_WIDTH,imageSize.width);
                cvSetCaptureProperty(captures[i] ,CV_CAP_PROP_FRAME_HEIGHT,imageSize.height);
        }


        ready = true;
        return 0;
    }else{
        ready = false;
        return 1;
    }

}

int StereoCamera::capture(){


    frames[0] = cvQueryFrame(captures[0]);
    frames[1]= cvQueryFrame(captures[1]);

    //render[0] = cvCloneImage(frames[0]);
    //render[1] = cvCloneImage(frames[1]);


    return (captures[0] && captures[1]) ? 0 : 1;
}

IplImage*  StereoCamera::getFramesGray(int lr){
    if(!frames[lr])
    	return 0;
    if(frames[lr]->depth == 1){
        framesGray[lr] = frames[lr];
        return frames[lr];
    }else{
        if(framesGray[lr] == 0)
        	framesGray[lr] = cvCreateImage(cvGetSize(frames[lr]),IPL_DEPTH_8U,1);
        cvCvtColor(frames[lr],framesGray[lr],CV_BGR2GRAY);
        return framesGray[lr];
    }
}
