/*
 * denseStereo.cpp
 *
 *  Created on: 17. apr. 2010
 *      Author: anderga
 */

#include <iostream>

#include <mex.h>







/*
 * Main entry point for the dense stereo algorithm.
 * The input to this function should be som parameters and the two
 * images that are captured. The images need to be rectified.
 *
 * Output should be 3 lists with x,y,z of matched features in the images.
 *
 * Usage:
 * 	[x, y, z]  = denseStereo( im1, im2, some parameters)
 *
 *
 */

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]){

    // Get input arguments
    if (nrhs == 0) {
        mexErrMsgTxt("StereoDense: Need input argument");
    }


}

