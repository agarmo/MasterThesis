/*!
  \file
  \brief

  \author Satofumi KAMIMURA

  $Id: delay.c 1374 2009-10-07 00:05:06Z satofumi $
*/


#include <windows.h>
#include <time.h>


enum {
  False = 0,
  True,
};

static int is_initialized_ = False;


void delay(int msec)
{
  if (is_initialized_ == False) {
    //timeBeginPeriod(1);
    is_initialized_ = True;
  }
  Sleep(msec);

}
