General Information
------------------------

This is a driver for Hokuyo URG-04LX and URG-30LX laser range scanners.
See hokuyoExampleSingleRequest.m
    hokuyoExampleStreaming.m
    
    testHokuyo.cc:
    testHokuyoReader.cc

for sample usage with Matlab and C++. Also, C++ documentation is a bit incomplete, so take a look at
the Matlab scripts for more examples.

Requirements
------------------------
If you are using a Hokuyo URG-04LX, it must be upgraded to the latest firmware, which supports SCIP2.0 mode and this mode must be enabled by default.
The old protocol is not supported.


File description
----------------
README.txt                       This file

Hokuyo.cc:  									   Low-level communication driver
Hokuyo.hh:                       Header file for Hokuyo.cc
testHokuyo.cc:                   Sample C++ program that shows how to use Hokuyo class
hokuyoAPI.cc:  								   Mex interface to Hokuyo class
hokuyoExampleSingleRequest.m:    Matlab script, showing how to use hokuyoAPI

HokuyoReader.cc:                 Threaded "reader" that allows to use the sensor in the most efficient 
                                 manner- it sets the scanner to streaming mode and constantly reads off 
                                 the measurement, always returning the latest data, upon user's request.
HokuyoReader.hh                  Header file for HokuyoReader.cc
testHokuyoReader.cc              Sample C++ program that shows how to use HokuyoReader class
hokuyoReaderAPI.cc               Mex interface to HokuyoReader class
hokuyoExampleStreaming.m         Matlab script, showing how to use hokuyoReaderAPI

SerialDevice.cc                  Serial port driver
SerialDevice.hh                  Header file for SerialDevice.cc

Makefile                         Build the executables
mexHokuyoAPI.m                   build hokuyoAPI from Matlab
mexHokuyoReaderAPI.m             build hokuyoReaderAPI from Matlab

HokuyoDriverNew.cc               Player 2.1 wrapper for the hokuyo driver
hokuyodrivernew.cfg              sample player config file



Compilation instructions
-----------------------

If you use the Makefile to compile mex files, please make sure that MATLAB_DIR is defined and points to the actual matlab directory, for example:
export MATLAB_DIR=/usr/local/matlab-7.1.0

If you compile from Matlab, you should not have to do anything.

In order to compile the player driver, you must have Player 2.1 installed, then type:
make hokuyodrivernew - this will compile the Player plugin (.so file).
Take a look at hokuyodrivernew.cfg for usage under player
in order to start up driver, run "player hokuyodrivernew.cfg". See Player manual for more details.

Note that the driver supports intensity mode for both 04LX and 30LX scanners. It is implemented differently
on hardware, so the behavior is different: 04-LX will only return every 3rd point if intensity is enabled -
this is in order to save bandwidth (sounds silly to me, but what can you do). 30LX returns full resolution
scans with range and intensity, but if you request full range, then frame rate drops, since there is not enough time to transfer all the data. If you want full rate (40hz), then limit the range (experiment with it).


Known Issues
-----------------------

-incorrect version of gcc: instead of trying to install an old version of gcc, add
CC='gcc-4.1' to mex command, then move  libstdc++.so.6, libstdc++.so.6.0.8, and
libgcc_s.so.1 out of the ... matlab/sys/os/glnx86/ folder so that matlab uses
system libs, then restart matlab.

-if you are getting checksum errors while connecting to the sensor, make sure you have the latest firmware, which supports SCIP2.0

Version History:

v0.16 - Initial Release (summer 2008)

v0.17 - Added Player wrapper that supports all scanning modes of 04-LX and 30LX with intensity




Notes:

Computing number of points that will be returned, given scan_start, scan_end, scan_skip:

*	# of points = ceil ( (scan_end - scan_start + 1) / skip )
	
	or in terms of angles,
*	# of points = ceil ( (angle_end - angle_start + angle_res) / (angle_res * skip) )


Computing the angle, given the angle_count:

*	angle = min_angle + angle_count*angle_res;

Computing angle_count from angle

*	angle_count = floor( angle / angle_res * scan_skip + count_zero + 0.49 )

	where count_zero is the angle_count of the zero angle (forward direction)
	0.49 is added to avoid numerical accuracy issues and to round the value


Limiting the angles to sensor's capabilities

	//first limit the angles to the sensor capabilities
	min_angle = min_angle >= laser_min_angle ? min_angle : laser_min_angle;
	max_angle = max_angle <= laser_max_angle ? max_angle : laser_max_angle;

	//add 0.49 to avoid accuracy issues during casting + round
	min_angle = ((int)(min_angle / laser_res - 0.49*laser_res)) * laser_res;
	max_angle = ((int)(max_angle / laser_res + 0.49*laser_res)) * laser_res;

The above information was experimentally verified agains the actual sensor readins


