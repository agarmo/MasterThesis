/*
patch_match.c     implements ...

  takes two images and does sum of differences of intensities at windows specified in each

  2001 written by Phil Torr
  Microsoft Research Cambridge
*/

//if correlation exceeds jump out then stop
#include <math.h>
#include <stdio.h>
#include "mex.h"

void mexFunction (
				  int		nlhs,		   /* number of expected outputs */
				  mxArray	**plhs, 	   /* matrix pointer array returning outputs */
				  int		nrhs,		   /* number of inputs */
				  const mxArray **prhs	   /* matrix pointer array for inputs */
				  ) {
	int       width, height, i, j, l, k, border =1;
	double    *out, *im1, *im2;
	bool is_max;
	int x1,y1,x2,y2,x1i,y1i,x2i,y2i,half_size,jump_out;
	double corr = 0.0;
	
	/* parameter checks */
	if ((nrhs < 7) || (nlhs != 1)) {
		mexErrMsgTxt ("Usage: Y = patch_match(im1,im2,x1,y1,x2,y2,half_size,jump_out)\n\n");
		return;
	}
	

	//not any checking here yet!!!!

	/* reading the parameters */
	height = mxGetM (prhs [0]);
	width = mxGetN (prhs [0]);
	im1 = (double *) mxGetPr (prhs [0]);
	im2 = (double *) mxGetPr (prhs [1]);
	x1 = (int)(mxGetScalar(prhs[2]))-1;
	y1 = (int)(mxGetScalar(prhs[3]))-1;
	x2 = (int)(mxGetScalar(prhs[4]))-1;
	y2 = (int)(mxGetScalar(prhs[5]))-1;
	half_size = (int)(mxGetScalar(prhs[6]));
	jump_out = (double)(mxGetScalar(prhs[7]));

//	printf("x1 y1 x2 y2 %d %d %d %d\n",x1,y1 ,x2,y2);
	
	//out = mxGetScalar(plhs [0]);

	/* require memory for return */
	plhs [0] = mxCreateDoubleMatrix (1, 1, mxREAL);
	out = (double *) mxGetPr (plhs [0]);
	

	// do correlation over a patch
	for (i = -half_size; i <= half_size; i++)
	{
		for (j = -half_size; j <= half_size; j++)
		{
			x1i = x1 + i;
			x2i = x2 + i;

			y1i = y1 + j;
			y2i = y2 + j;
			corr += fabs(im1 [y1i * height + x1i]- im2 [y2i * height + x2i]);
//			printf("im1 i j x1i y1i %d %d %d %d %lf \n", i,j,x1i,y1i,im1 [x1i * height + y1i]);
//			printf("im2 i j x2i y2i %d %d %d %d %lf \n", i,j,x2i,y2i,im2 [x2i * height + y2i]);
//			printf("corr i j  %d %d %lf \n", i,j,corr);

		}
		if (corr > jump_out)
		{
			out[0] = corr;
			return;
		}

	}

	out[0] = corr;

//	printf("corr \n %lf", corr);

	return;
}



			//	out[0] = im1 [x1 * height + y1]- im2 [x2 * height + y2];
//	out[0] = im1 [y1 * height + x1]- im2 [y2 * height + x2];
//	out[0] = im1 [y1 * width + x1]- im2 [y2 * width + x2];
