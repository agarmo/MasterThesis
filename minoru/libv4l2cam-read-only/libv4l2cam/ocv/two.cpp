/*
 * Copyright (C) 2009 Giacomo Spigler
 * CopyPolicy: Released under the terms of the GNU GPL v3.0.
 */

//2 Webcams!


#include <iostream>
#include <cv.h>
#include <highgui.h>
#include <stdio.h>

#include "../libcam.h"

using namespace std;


#define CAM2 1


int main() {
  int ww=640;
  int hh=480;

  Camera c("/dev/video2", ww, hh, 15);
#ifdef CAM2
  Camera c2("/dev/video1", ww, hh, 15);
#endif


//cout<<c.setSharpness(3)<<"   "<<c.minSharpness()<<"  "<<c.maxSharpness()<<" "<<c.defaultSharpness()<<endl;



  cvNamedWindow("l", CV_WINDOW_AUTOSIZE);
#ifdef CAM2
  cvNamedWindow("r", CV_WINDOW_AUTOSIZE);
#endif


  IplImage *l=cvCreateImage(cvSize(ww, hh), 8, 3);
  unsigned char *l_=(unsigned char *)l->imageData;
#ifdef CAM2
  IplImage *r=cvCreateImage(cvSize(ww, hh), 8, 3);
  unsigned char *r_=(unsigned char *)r->imageData;
#endif




  while(1){
//    c.Update();
#ifdef CAM2
    c.Update(&c2);
#else
    c.Update();
#endif






    c.toIplImage(l);
#ifdef CAM2
    c2.toIplImage(r);
#endif






    cvShowImage("l", l);
#ifdef CAM2
    cvShowImage("r", r);
#endif


    if( (cvWaitKey(10) & 255) == 27 ) break;
  }




/*
  cvSaveImage("l.png", l);
  cvSaveImage("r.png", r);
/**/


  cvDestroyWindow("l");
  cvReleaseImage(&l);
#ifdef CAM2
  cvDestroyWindow("r");
  cvReleaseImage(&l);
#endif

  return 0;
}





