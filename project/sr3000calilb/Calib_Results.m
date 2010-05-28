% Intrinsic and Extrinsic Camera Parameters
%
% This script file can be directly excecuted under Matlab to recover the camera intrinsic and extrinsic parameters.
% IMPORTANT: This file contains neither the structure of the calibration objects nor the image coordinates of the calibration points.
%            All those complementary variables are saved in the complete matlab data file Calib_Results.mat.
% For more information regarding the calibration model visit http://www.vision.caltech.edu/bouguetj/calib_doc/


%-- Focal length:
fc = [ 213.936283990629990 ; 212.521769376317170 ];

%-- Principal point:
cc = [ 62.962768163220730 ; 88.637651453676298 ];

%-- Skew coefficient:
alpha_c = 0.000000000000000;

%-- Distortion coefficients:
kc = [ -0.813839891193129 ; 6.772205829641939 ; 0.004444996852290 ; 0.000245752556863 ; 0.000000000000000 ];

%-- Focal length uncertainty:
fc_error = [ 14.955979882436655 ; 14.914218731581343 ];

%-- Principal point uncertainty:
cc_error = [ 5.673812468572526 ; 4.993470452256117 ];

%-- Skew coefficient uncertainty:
alpha_c_error = 0.000000000000000;

%-- Distortion coefficients uncertainty:
kc_error = [ 0.371149395239943 ; 4.354742662269850 ; 0.011017625836467 ; 0.013497320236552 ; 0.000000000000000 ];

%-- Image size:
nx = 144;
ny = 176;


%-- Various other variables (may be ignored if you do not use the Matlab Calibration Toolbox):
%-- Those variables are used to control which intrinsic parameters should be optimized

n_ima = 20;						% Number of calibration images
est_fc = [ 1 ; 1 ];					% Estimation indicator of the two focal variables
est_aspect_ratio = 1;				% Estimation indicator of the aspect ratio fc(2)/fc(1)
center_optim = 1;					% Estimation indicator of the principal point
est_alpha = 0;						% Estimation indicator of the skew coefficient
est_dist = [ 1 ; 1 ; 1 ; 1 ; 0 ];	% Estimation indicator of the distortion coefficients


%-- Extrinsic parameters:
%-- The rotation (omc_kk) and the translation (Tc_kk) vectors for every calibration image and their uncertainties

%-- Image #1:
omc_1 = [ -1.865982e+000 ; -2.131075e+000 ; 3.022072e-001 ];
Tc_1  = [ -2.568914e+001 ; -8.685827e+001 ; 6.190673e+002 ];
omc_error_1 = [ 4.349048e-002 ; 4.689250e-002 ; 8.507252e-002 ];
Tc_error_1  = [ 1.657581e+001 ; 1.480590e+001 ; 4.410644e+001 ];

%-- Image #2:
omc_2 = [ NaN ; NaN ; NaN ];
Tc_2  = [ NaN ; NaN ; NaN ];
omc_error_2 = [ NaN ; NaN ; NaN ];
Tc_error_2  = [ NaN ; NaN ; NaN ];

%-- Image #3:
omc_3 = [ NaN ; NaN ; NaN ];
Tc_3  = [ NaN ; NaN ; NaN ];
omc_error_3 = [ NaN ; NaN ; NaN ];
Tc_error_3  = [ NaN ; NaN ; NaN ];

%-- Image #4:
omc_4 = [ -1.942105e+000 ; -2.137403e+000 ; 5.165901e-001 ];
Tc_4  = [ -3.073323e+001 ; -9.408179e+001 ; 5.605848e+002 ];
omc_error_4 = [ 3.795126e-002 ; 3.871764e-002 ; 6.891948e-002 ];
Tc_error_4  = [ 1.503517e+001 ; 1.342491e+001 ; 3.816916e+001 ];

%-- Image #5:
omc_5 = [ -1.898838e+000 ; -2.180729e+000 ; 4.660801e-001 ];
Tc_5  = [ -4.600006e+001 ; -9.326126e+001 ; 5.610148e+002 ];
omc_error_5 = [ 3.833613e-002 ; 3.979587e-002 ; 6.979507e-002 ];
Tc_error_5  = [ 1.509807e+001 ; 1.345662e+001 ; 3.832357e+001 ];

%-- Image #6:
omc_6 = [ 2.892902e-001 ; -2.534582e+000 ; 2.182443e-001 ];
Tc_6  = [ 1.271333e+002 ; -8.013553e+001 ; 1.017696e+003 ];
omc_error_6 = [ 2.604975e-002 ; 6.337387e-002 ; 8.548641e-002 ];
Tc_error_6  = [ 2.784514e+001 ; 2.432719e+001 ; 7.821675e+001 ];

%-- Image #7:
omc_7 = [ NaN ; NaN ; NaN ];
Tc_7  = [ NaN ; NaN ; NaN ];
omc_error_7 = [ NaN ; NaN ; NaN ];
Tc_error_7  = [ NaN ; NaN ; NaN ];

%-- Image #8:
omc_8 = [ -1.863696e+000 ; -2.291225e+000 ; 4.277683e-001 ];
Tc_8  = [ -9.430285e+001 ; -1.000523e+002 ; 6.350530e+002 ];
omc_error_8 = [ 4.252906e-002 ; 4.483142e-002 ; 8.119063e-002 ];
Tc_error_8  = [ 1.721317e+001 ; 1.531903e+001 ; 4.342012e+001 ];

%-- Image #9:
omc_9 = [ -1.877477e+000 ; -2.059319e+000 ; 8.225670e-002 ];
Tc_9  = [ -1.047324e+002 ; -1.264904e+002 ; 7.399945e+002 ];
omc_error_9 = [ 5.208232e-002 ; 4.657405e-002 ; 9.416167e-002 ];
Tc_error_9  = [ 2.006639e+001 ; 1.774239e+001 ; 5.362872e+001 ];

%-- Image #10:
omc_10 = [ -2.039566e+000 ; -2.185228e+000 ; 5.457721e-002 ];
Tc_10  = [ -8.071356e+001 ; -1.148510e+002 ; 5.129411e+002 ];
omc_error_10 = [ 5.021725e-002 ; 4.890306e-002 ; 1.040207e-001 ];
Tc_error_10  = [ 1.397067e+001 ; 1.236260e+001 ; 3.713125e+001 ];

%-- Image #11:
omc_11 = [ 1.862920e+000 ; 2.004032e+000 ; -6.527023e-001 ];
Tc_11  = [ -1.120252e+002 ; -1.066551e+002 ; 6.056372e+002 ];
omc_error_11 = [ 2.709960e-002 ; 3.707401e-002 ; 5.518003e-002 ];
Tc_error_11  = [ 1.652191e+001 ; 1.482956e+001 ; 3.792369e+001 ];

%-- Image #12:
omc_12 = [ -1.818796e+000 ; -1.809165e+000 ; -5.757113e-001 ];
Tc_12  = [ -1.178766e+002 ; -5.842666e+001 ; 5.501731e+002 ];
omc_error_12 = [ 3.020807e-002 ; 3.293680e-002 ; 4.879684e-002 ];
Tc_error_12  = [ 1.481777e+001 ; 1.336268e+001 ; 4.215649e+001 ];

%-- Image #13:
omc_13 = [ -1.962138e+000 ; -2.242987e+000 ; 2.830021e-001 ];
Tc_13  = [ -1.530769e+002 ; -1.679791e+002 ; 7.522600e+002 ];
omc_error_13 = [ 5.642868e-002 ; 4.925907e-002 ; 9.732795e-002 ];
Tc_error_13  = [ 2.060194e+001 ; 1.807487e+001 ; 5.371306e+001 ];

%-- Image #14:
omc_14 = [ -2.075225e+000 ; -2.118163e+000 ; 2.981093e-001 ];
Tc_14  = [ -9.995594e+001 ; -1.149068e+002 ; 5.483302e+002 ];
omc_error_14 = [ 4.546571e-002 ; 4.033611e-002 ; 8.057965e-002 ];
Tc_error_14  = [ 1.493524e+001 ; 1.318138e+001 ; 3.824875e+001 ];

%-- Image #15:
omc_15 = [ 2.193534e+000 ; 2.091614e+000 ; 3.586091e-001 ];
Tc_15  = [ -1.236631e+002 ; -8.185382e+001 ; 6.226484e+002 ];
omc_error_15 = [ 6.630796e-002 ; 6.420280e-002 ; 1.271779e-001 ];
Tc_error_15  = [ 1.709559e+001 ; 1.510559e+001 ; 4.708079e+001 ];

%-- Image #16:
omc_16 = [ 2.210220e+000 ; 2.002318e+000 ; 1.904390e-001 ];
Tc_16  = [ -9.806872e+001 ; -7.718415e+001 ; 5.445604e+002 ];
omc_error_16 = [ 6.122041e-002 ; 5.231832e-002 ; 1.148889e-001 ];
Tc_error_16  = [ 1.476136e+001 ; 1.311497e+001 ; 4.027858e+001 ];

%-- Image #17:
omc_17 = [ -1.618490e+000 ; -2.004034e+000 ; -4.829064e-001 ];
Tc_17  = [ -1.071089e+002 ; -1.450965e+002 ; 5.665362e+002 ];
omc_error_17 = [ 3.006449e-002 ; 3.345128e-002 ; 5.622692e-002 ];
Tc_error_17  = [ 1.550415e+001 ; 1.436072e+001 ; 4.404205e+001 ];

%-- Image #18:
omc_18 = [ 2.110868e+000 ; 1.882535e+000 ; -4.344059e-001 ];
Tc_18  = [ -7.304516e+001 ; -3.952048e+001 ; 6.057872e+002 ];
omc_error_18 = [ 4.611704e-002 ; 3.865106e-002 ; 7.906962e-002 ];
Tc_error_18  = [ 1.627864e+001 ; 1.442062e+001 ; 4.061272e+001 ];

%-- Image #19:
omc_19 = [ -1.861884e+000 ; -2.078802e+000 ; -8.207883e-001 ];
Tc_19  = [ -8.489309e+001 ; -9.084221e+001 ; 5.647341e+002 ];
omc_error_19 = [ 3.098186e-002 ; 3.948265e-002 ; 5.714040e-002 ];
Tc_error_19  = [ 1.543087e+001 ; 1.386100e+001 ; 4.497902e+001 ];

%-- Image #20:
omc_20 = [ 1.775596e+000 ; 1.826022e+000 ; -5.604988e-001 ];
Tc_20  = [ -1.099573e+002 ; -6.051273e+001 ; 6.136856e+002 ];
omc_error_20 = [ 2.704793e-002 ; 3.252428e-002 ; 5.073561e-002 ];
Tc_error_20  = [ 1.653432e+001 ; 1.476522e+001 ; 3.806228e+001 ];

