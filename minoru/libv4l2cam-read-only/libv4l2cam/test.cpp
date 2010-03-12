/*
 * Copyright (C) 2009 Giacomo Spigler
 * CopyPolicy: Released under the terms of the GNU GPL v3.0.
 */

#include <iostream>
#include "libcam.h"

using namespace std;


int main() {
  Camera c("/dev/video0", 640, 480);


c.Update();

cout<<(int)c.data[0]<<endl;



  return 0;
}



