% Intrinsic and Extrinsic Camera Parameters
%
% This script file can be directly excecuted under Matlab to recover the camera intrinsic and extrinsic parameters.
% IMPORTANT: This file contains neither the structure of the calibration objects nor the image coordinates of the calibration points.
%            All those complementary variables are saved in the complete matlab data file Calib_Results.mat.
% For more information regarding the calibration model visit http://www.vision.caltech.edu/bouguetj/calib_doc/


%-- Focal length:
fc = [ 879.905981437936700 ; 878.840679887911850 ];

%-- Principal point:
cc = [ 354.703664502570460 ; 212.359222736396050 ];

%-- Skew coefficient:
alpha_c = 0.000000000000000;

%-- Distortion coefficients:
kc = [ -0.098931595328393 ; -0.230469786233955 ; -0.003806600881718 ; 0.002355035044068 ; 0.000000000000000 ];

%-- Focal length uncertainty:
fc_error = [ 4.000955237736177 ; 3.837310594907627 ];

%-- Principal point uncertainty:
cc_error = [ 6.218424399891893 ; 4.828540765722632 ];

%-- Skew coefficient uncertainty:
alpha_c_error = 0.000000000000000;

%-- Distortion coefficients uncertainty:
kc_error = [ 0.035677685610163 ; 0.469183242634159 ; 0.001489011028253 ; 0.001744180274720 ; 0.000000000000000 ];

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
omc_1 = [ 1.943228e+000 ; 1.874619e+000 ; 1.300930e-001 ];
Tc_1  = [ -2.042374e+002 ; -6.912838e+001 ; 8.370889e+002 ];
omc_error_1 = [ 5.267314e-003 ; 6.188783e-003 ; 1.052494e-002 ];
Tc_error_1  = [ 5.996084e+000 ; 4.670861e+000 ; 4.313231e+000 ];

%-- Image #2:
omc_2 = [ 2.022916e+000 ; 1.986623e+000 ; 9.957008e-002 ];
Tc_2  = [ -1.029495e+002 ; -5.778367e+001 ; 6.491684e+002 ];
omc_error_2 = [ 5.302922e-003 ; 5.321803e-003 ; 1.082300e-002 ];
Tc_error_2  = [ 4.609077e+000 ; 3.581046e+000 ; 3.141568e+000 ];

%-- Image #3:
omc_3 = [ -2.821227e+000 ; -1.096294e+000 ; -2.555615e-001 ];
Tc_3  = [ -1.291562e+002 ; 1.076488e+001 ; 6.043909e+002 ];
omc_error_3 = [ 6.991335e-003 ; 3.436308e-003 ; 1.299494e-002 ];
Tc_error_3  = [ 4.298171e+000 ; 3.345185e+000 ; 2.935923e+000 ];

%-- Image #4:
omc_4 = [ 1.313239e+000 ; 1.377954e+000 ; 6.948778e-001 ];
Tc_4  = [ -1.634392e+002 ; -4.648082e+001 ; 7.182606e+002 ];
omc_error_4 = [ 5.578930e-003 ; 5.590288e-003 ; 6.136748e-003 ];
Tc_error_4  = [ 5.157575e+000 ; 4.011458e+000 ; 3.804200e+000 ];

%-- Image #5:
omc_5 = [ 1.944188e+000 ; 1.978417e+000 ; 6.411048e-002 ];
Tc_5  = [ -8.719430e+001 ; -1.053593e+002 ; 5.044068e+002 ];
omc_error_5 = [ 4.697521e-003 ; 5.398363e-003 ; 9.390936e-003 ];
Tc_error_5  = [ 3.597874e+000 ; 2.781698e+000 ; 2.489461e+000 ];

%-- Image #6:
omc_6 = [ 1.893254e+000 ; 1.800783e+000 ; 4.794749e-001 ];
Tc_6  = [ -8.766947e+001 ; -9.572488e+001 ; 7.313655e+002 ];
omc_error_6 = [ 5.728285e-003 ; 5.115477e-003 ; 8.888175e-003 ];
Tc_error_6  = [ 5.197279e+000 ; 4.028859e+000 ; 3.677182e+000 ];

%-- Image #7:
omc_7 = [ -1.912450e+000 ; -1.993027e+000 ; 5.910523e-001 ];
Tc_7  = [ -1.132334e+002 ; -9.380183e+001 ; 7.014221e+002 ];
omc_error_7 = [ 6.225639e-003 ; 4.840246e-003 ; 9.301068e-003 ];
Tc_error_7  = [ 4.978367e+000 ; 3.878644e+000 ; 3.046898e+000 ];

%-- Image #8:
omc_8 = [ 2.039746e+000 ; 2.222186e+000 ; -8.175752e-001 ];
Tc_8  = [ -9.349420e+001 ; -9.574314e+001 ; 5.677763e+002 ];
omc_error_8 = [ 2.967307e-003 ; 6.359753e-003 ; 1.029528e-002 ];
Tc_error_8  = [ 4.035146e+000 ; 3.139533e+000 ; 2.462445e+000 ];

%-- Image #9:
omc_9 = [ 1.864280e+000 ; 2.167618e+000 ; 2.530290e-001 ];
Tc_9  = [ -1.275932e+002 ; -8.775033e+001 ; 6.994381e+002 ];
omc_error_9 = [ 5.423326e-003 ; 6.242078e-003 ; 1.115974e-002 ];
Tc_error_9  = [ 4.990375e+000 ; 3.881104e+000 ; 3.517483e+000 ];

%-- Image #10:
omc_10 = [ -2.261140e+000 ; -2.059513e+000 ; 2.692300e-001 ];
Tc_10  = [ -1.439904e+002 ; -6.512245e+001 ; 7.504712e+002 ];
omc_error_10 = [ 7.236519e-003 ; 5.804903e-003 ; 1.338382e-002 ];
Tc_error_10  = [ 5.312973e+000 ; 4.154666e+000 ; 3.525737e+000 ];

%-- Image #11:
omc_11 = [ -1.986311e+000 ; -1.982619e+000 ; 3.804426e-001 ];
Tc_11  = [ -3.699273e+001 ; -7.178215e+001 ; 8.314996e+002 ];
omc_error_11 = [ 6.040200e-003 ; 5.948103e-003 ; 1.095233e-002 ];
Tc_error_11  = [ 5.871181e+000 ; 4.559137e+000 ; 3.644638e+000 ];

%-- Image #12:
omc_12 = [ 1.924607e+000 ; 1.649267e+000 ; 5.038332e-001 ];
Tc_12  = [ -1.341821e+002 ; -6.591600e+001 ; 5.327626e+002 ];
omc_error_12 = [ 5.428998e-003 ; 4.534006e-003 ; 8.086092e-003 ];
Tc_error_12  = [ 3.827979e+000 ; 2.972147e+000 ; 2.800624e+000 ];

%-- Image #13:
omc_13 = [ 1.924510e+000 ; 1.680270e+000 ; 3.604319e-001 ];
Tc_13  = [ -1.161658e+002 ; -8.122288e+001 ; 5.796374e+002 ];
omc_error_13 = [ 5.283401e-003 ; 4.871104e-003 ; 8.365753e-003 ];
Tc_error_13  = [ 4.138012e+000 ; 3.208847e+000 ; 2.949486e+000 ];

%-- Image #14:
omc_14 = [ 1.733317e+000 ; 1.499214e+000 ; -5.524867e-001 ];
Tc_14  = [ -1.224757e+002 ; -3.107937e+001 ; 6.711182e+002 ];
omc_error_14 = [ 4.281791e-003 ; 5.804618e-003 ; 7.664578e-003 ];
Tc_error_14  = [ 4.745968e+000 ; 3.704663e+000 ; 3.090376e+000 ];

%-- Image #15:
omc_15 = [ 2.030419e+000 ; 2.324886e+000 ; 2.992897e-001 ];
Tc_15  = [ -7.466612e+001 ; -9.375115e+001 ; 4.739975e+002 ];
omc_error_15 = [ 5.677109e-003 ; 5.451989e-003 ; 1.072657e-002 ];
Tc_error_15  = [ 3.378301e+000 ; 2.627244e+000 ; 2.374253e+000 ];

%-- Image #16:
omc_16 = [ -1.852217e+000 ; -2.087277e+000 ; 4.584815e-001 ];
Tc_16  = [ -6.568945e+001 ; -9.799406e+001 ; 6.513058e+002 ];
omc_error_16 = [ 5.599806e-003 ; 5.224937e-003 ; 9.308110e-003 ];
Tc_error_16  = [ 4.609876e+000 ; 3.578561e+000 ; 2.828760e+000 ];

%-- Image #17:
omc_17 = [ 2.043246e+000 ; 1.721090e+000 ; -5.248453e-001 ];
Tc_17  = [ -1.316722e+002 ; -5.240826e+001 ; 6.516883e+002 ];
omc_error_17 = [ 4.114803e-003 ; 5.643074e-003 ; 9.132102e-003 ];
Tc_error_17  = [ 4.610933e+000 ; 3.605427e+000 ; 3.041140e+000 ];

%-- Image #18:
omc_18 = [ -1.810830e+000 ; -2.016588e+000 ; -9.273578e-001 ];
Tc_18  = [ -6.795982e+001 ; -7.235376e+001 ; 5.982442e+002 ];
omc_error_18 = [ 3.432757e-003 ; 6.535654e-003 ; 9.434380e-003 ];
Tc_error_18  = [ 4.241218e+000 ; 3.296984e+000 ; 3.186461e+000 ];

%-- Image #19:
omc_19 = [ -1.637333e+000 ; -1.825173e+000 ; -1.034933e+000 ];
Tc_19  = [ -5.423013e+001 ; -2.645883e+001 ; 5.633747e+002 ];
omc_error_19 = [ 3.730436e-003 ; 6.727588e-003 ; 8.487225e-003 ];
Tc_error_19  = [ 3.979990e+000 ; 3.097756e+000 ; 2.991463e+000 ];

