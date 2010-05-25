% Intrinsic and Extrinsic Camera Parameters
%
% This script file can be directly excecuted under Matlab to recover the camera intrinsic and extrinsic parameters.
% IMPORTANT: This file contains neither the structure of the calibration objects nor the image coordinates of the calibration points.
%            All those complementary variables are saved in the complete matlab data file Calib_Results.mat.
% For more information regarding the calibration model visit http://www.vision.caltech.edu/bouguetj/calib_doc/


%-- Focal length:
fc = [ 879.740911256035470 ; 878.328961205926360 ];

%-- Principal point:
cc = [ 338.694338443000050 ; 269.831349308786170 ];

%-- Skew coefficient:
alpha_c = 0.000000000000000;

%-- Distortion coefficients:
kc = [ -0.056010403140087 ; -0.212032169465038 ; 0.000275672996056 ; 0.004980589655730 ; 0.000000000000000 ];

%-- Focal length uncertainty:
fc_error = [ 4.066031815418859 ; 3.923922335760103 ];

%-- Principal point uncertainty:
cc_error = [ 6.416473845260544 ; 4.859775739250413 ];

%-- Skew coefficient uncertainty:
alpha_c_error = 0.000000000000000;

%-- Distortion coefficients uncertainty:
kc_error = [ 0.023828694541940 ; 0.125362334558286 ; 0.001382576002960 ; 0.002457994459925 ; 0.000000000000000 ];

%-- Image size:
nx = 640;
ny = 480;


%-- Various other variables (may be ignored if you do not use the Matlab Calibration Toolbox):
%-- Those variables are used to control which intrinsic parameters should be optimized

n_ima = 19;						% Number of calibration images
est_fc = [ 1 ; 1 ];					% Estimation indicator of the two focal variables
est_aspect_ratio = 1;				% Estimation indicator of the aspect ratio fc(2)/fc(1)
center_optim = 1;					% Estimation indicator of the principal point
est_alpha = 0;						% Estimation indicator of the skew coefficient
est_dist = [ 1 ; 1 ; 1 ; 1 ; 0 ];	% Estimation indicator of the distortion coefficients


%-- Extrinsic parameters:
%-- The rotation (omc_kk) and the translation (Tc_kk) vectors for every calibration image and their uncertainties

%-- Image #1:
omc_1 = [ 1.947258e+000 ; 1.873621e+000 ; 1.513388e-001 ];
Tc_1  = [ -2.711137e+002 ; -8.325442e+001 ; 8.313798e+002 ];
omc_error_1 = [ 5.605542e-003 ; 7.101680e-003 ; 1.161786e-002 ];
Tc_error_1  = [ 6.135245e+000 ; 4.726637e+000 ; 4.552723e+000 ];

%-- Image #2:
omc_2 = [ 2.028467e+000 ; 1.990613e+000 ; 1.244749e-001 ];
Tc_2  = [ -1.681559e+002 ; -6.863491e+001 ; 6.452172e+002 ];
omc_error_2 = [ 5.595478e-003 ; 6.259867e-003 ; 1.200217e-002 ];
Tc_error_2  = [ 4.729346e+000 ; 3.623222e+000 ; 3.345748e+000 ];

%-- Image #3:
omc_3 = [ -2.808751e+000 ; -1.094072e+000 ; -2.742475e-001 ];
Tc_3  = [ -1.941977e+002 ; 5.574485e-001 ; 6.010837e+002 ];
omc_error_3 = [ 7.682021e-003 ; 3.106278e-003 ; 1.386674e-002 ];
Tc_error_3  = [ 4.417540e+000 ; 3.401806e+000 ; 3.170145e+000 ];

%-- Image #4:
omc_4 = [ 1.320306e+000 ; 1.370327e+000 ; 7.132166e-001 ];
Tc_4  = [ -2.293833e+002 ; -5.866199e+001 ; 7.135631e+002 ];
omc_error_4 = [ 5.585792e-003 ; 5.789275e-003 ; 6.347932e-003 ];
Tc_error_4  = [ 5.302526e+000 ; 4.067350e+000 ; 4.019939e+000 ];

%-- Image #5:
omc_5 = [ 1.950176e+000 ; 1.981155e+000 ; 8.897767e-002 ];
Tc_5  = [ -1.511898e+002 ; -1.138271e+002 ; 5.001351e+002 ];
omc_error_5 = [ 4.490992e-003 ; 5.826868e-003 ; 9.682335e-003 ];
Tc_error_5  = [ 3.695245e+000 ; 2.817857e+000 ; 2.699431e+000 ];

%-- Image #6:
omc_6 = [ 1.898276e+000 ; 1.799040e+000 ; 5.000395e-001 ];
Tc_6  = [ -1.536658e+002 ; -1.079762e+002 ; 7.275624e+002 ];
omc_error_6 = [ 5.917128e-003 ; 5.677189e-003 ; 9.270784e-003 ];
Tc_error_6  = [ 5.337396e+000 ; 4.070888e+000 ; 3.871505e+000 ];

%-- Image #7:
omc_7 = [ -1.907095e+000 ; -1.998420e+000 ; 5.676614e-001 ];
Tc_7  = [ -1.788264e+002 ; -1.055976e+002 ; 6.963487e+002 ];
omc_error_7 = [ 6.384722e-003 ; 4.726733e-003 ; 9.507246e-003 ];
Tc_error_7  = [ 5.109363e+000 ; 3.925112e+000 ; 3.253117e+000 ];

%-- Image #8:
omc_8 = [ 2.045388e+000 ; 2.232545e+000 ; -7.919920e-001 ];
Tc_8  = [ -1.585275e+002 ; -1.054850e+002 ; 5.636204e+002 ];
omc_error_8 = [ 2.534054e-003 ; 6.699680e-003 ; 1.083415e-002 ];
Tc_error_8  = [ 4.144935e+000 ; 3.197865e+000 ; 2.650437e+000 ];

%-- Image #9:
omc_9 = [ 1.868834e+000 ; 2.169872e+000 ; 2.753146e-001 ];
Tc_9  = [ -1.934816e+002 ; -9.950556e+001 ; 6.947599e+002 ];
omc_error_9 = [ 6.022616e-003 ; 7.726444e-003 ; 1.288544e-002 ];
Tc_error_9  = [ 5.129698e+000 ; 3.930523e+000 ; 3.745485e+000 ];

%-- Image #10:
omc_10 = [ -2.254934e+000 ; -2.061142e+000 ; 2.425796e-001 ];
Tc_10  = [ -2.100732e+002 ; -7.777947e+001 ; 7.456773e+002 ];
omc_error_10 = [ 7.020303e-003 ; 5.139201e-003 ; 1.294449e-002 ];
Tc_error_10  = [ 5.455494e+000 ; 4.207476e+000 ; 3.702490e+000 ];

%-- Image #11:
omc_11 = [ -1.979887e+000 ; -1.986478e+000 ; 3.602451e-001 ];
Tc_11  = [ -1.035620e+002 ; -8.570645e+001 ; 8.270575e+002 ];
omc_error_11 = [ 5.966840e-003 ; 5.378721e-003 ; 1.062224e-002 ];
Tc_error_11  = [ 6.016500e+000 ; 4.584210e+000 ; 3.763222e+000 ];

%-- Image #12:
omc_12 = [ 1.930295e+000 ; 1.647584e+000 ; 5.276764e-001 ];
Tc_12  = [ -1.983543e+002 ; -7.499148e+001 ; 5.278856e+002 ];
omc_error_12 = [ 5.398954e-003 ; 4.890911e-003 ; 8.350713e-003 ];
Tc_error_12  = [ 3.944291e+000 ; 3.026602e+000 ; 3.024519e+000 ];

%-- Image #13:
omc_13 = [ 1.929794e+000 ; 1.679017e+000 ; 3.830148e-001 ];
Tc_13  = [ -1.809246e+002 ; -9.151121e+001 ; 5.745729e+002 ];
omc_error_13 = [ 5.279950e-003 ; 5.313792e-003 ; 8.676396e-003 ];
Tc_error_13  = [ 4.245798e+000 ; 3.251923e+000 ; 3.168267e+000 ];

%-- Image #14:
omc_14 = [ 1.745248e+000 ; 1.504090e+000 ; -5.336868e-001 ];
Tc_14  = [ -1.880056e+002 ; -4.242591e+001 ; 6.670853e+002 ];
omc_error_14 = [ 4.054458e-003 ; 6.089241e-003 ; 8.118528e-003 ];
Tc_error_14  = [ 4.866726e+000 ; 3.760225e+000 ; 3.269349e+000 ];

%-- Image #15:
omc_15 = [ 2.032768e+000 ; 2.329226e+000 ; 3.163025e-001 ];
Tc_15  = [ -1.386842e+002 ; -1.019635e+002 ; 4.711342e+002 ];
omc_error_15 = [ 5.516014e-003 ; 5.874614e-003 ; 1.059274e-002 ];
Tc_error_15  = [ 3.492086e+000 ; 2.686560e+000 ; 2.554064e+000 ];

%-- Image #16:
omc_16 = [ -1.845812e+000 ; -2.091115e+000 ; 4.361143e-001 ];
Tc_16  = [ -1.308938e+002 ; -1.086665e+002 ; 6.463209e+002 ];
omc_error_16 = [ 5.746389e-003 ; 4.983777e-003 ; 9.510204e-003 ];
Tc_error_16  = [ 4.720886e+000 ; 3.609388e+000 ; 2.984673e+000 ];

%-- Image #17:
omc_17 = [ 2.051938e+000 ; 1.727947e+000 ; -5.036104e-001 ];
Tc_17  = [ -1.970716e+002 ; -6.331640e+001 ; 6.472425e+002 ];
omc_error_17 = [ 3.856140e-003 ; 5.957271e-003 ; 9.752162e-003 ];
Tc_error_17  = [ 4.729839e+000 ; 3.663095e+000 ; 3.233151e+000 ];

%-- Image #18:
omc_18 = [ -1.799529e+000 ; -2.007962e+000 ; -9.469027e-001 ];
Tc_18  = [ -1.331159e+002 ; -8.224198e+001 ; 5.938124e+002 ];
omc_error_18 = [ 3.814620e-003 ; 6.549881e-003 ; 9.366838e-003 ];
Tc_error_18  = [ 4.361602e+000 ; 3.334945e+000 ; 3.298097e+000 ];

%-- Image #19:
omc_19 = [ -1.624121e+000 ; -1.816985e+000 ; -1.054060e+000 ];
Tc_19  = [ -1.191408e+002 ; -3.581946e+001 ; 5.596052e+002 ];
omc_error_19 = [ 4.076666e-003 ; 6.752165e-003 ; 8.397953e-003 ];
Tc_error_19  = [ 4.086348e+000 ; 3.130793e+000 ; 3.088126e+000 ];

