/*
max3.c     implements ...

  goes through and tests to see if the pixel is a maximum in its 3x3 neighbourhood

  2001 written by Phil Torr
  Microsoft Research Cambridge
*/

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
	double    *out, *in, max;
	bool is_max;
	
	/* parameter checks */
	if ((nrhs != 1) || (nlhs != 1)) {
		mexErrMsgTxt ("Usage: Y = max3 (im1)\n\n");
		return;
	}
	
	/* reading the parameters */
	height = mxGetM (prhs [0]);
	width = mxGetN (prhs [0]);
	in = (double *) mxGetPr (prhs [0]);
	
	/* require memory for return */
	plhs [0] = mxCreateDoubleMatrix (height, width, mxREAL);
	out = (double *) mxGetPr (plhs [0]);
	

	//fill in border pixels with a negative...
	for (i = 0; i < height; i++)
	{
		j = 0; 
		out [j * height + i] = -10001.0;
		j = width-1; 
		out [j * height + i] = -10001.0;
	}

	
	//fill in border pixels with a negative...
	for (j = border; j < width-border; j++)
	{
		i = 0;
		out [j * height + i] = -10001.0;
		i = height-1; 
		out [j * height + i] = -10001.0;
	}

	
	/* check for maximum */
	for (i = border; i < height-border; i++)
		for (j = border; j < width-border; j++) {
			max = in [j * height + i]; 
			is_max = true;
			if (max >0.0)
			{
				/* finding the maximum in the neighbourhood */
				for (l = -border; l < border+1; l++)
					for (k = -border; k < border+1; k++) 
						if (in [(j + l) * height + (i + k)] >= max) {
							if ((l!=0) || (k!=0))
								//max = in [(j + l) * height + (i + k)];
								is_max = false;
						}
			}
			else
				is_max = false; //if not over threshold, here zero
			if (is_max)
				out [j * height + i] = max;
			else
				out [j * height + i] = -10001.0;
		}
		
		return;
}
