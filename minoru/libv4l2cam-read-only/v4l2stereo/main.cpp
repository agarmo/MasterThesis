/*
    v4l2stereo
    A command line utility for stereoscopic vision
    Copyright (C) 2009 Bob Mottram and Giacomo Spigler
    fuzzgun@gmail.com

    Requires packages:
		libgstreamer-plugins-base0.10-dev
		libgst-dev

    sudo apt-get install libcv4 libhighgui4 libcvaux4 libcv-dev libcvaux-dev libhighgui-dev libgstreamer-plugins-base0.10-dev libgst-dev

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <cv.h>
#include <highgui.h>
#include <stdio.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappbuffer.h>
#include <sstream>

#include "anyoption.h"
#include "drawing.h"
#include "stereo.h"
#include "stereodense.h"
#include "fast.h"
#include "libcam.h"

#define VERSION 1.046

using namespace std;

int main(int argc, char* argv[]) {
  int ww = 320;
  int hh = 240;
  int skip_frames = 1;
  int prev_matches = 0;
  bool show_features = false;
  bool show_matches = false;
  bool show_regions = false;
  bool show_depthmap = false;
  bool show_anaglyph = false;
  bool show_histogram = false;
  bool show_lines = false;
  bool show_disparity_map = false;
  bool rectify_images = false;
  bool show_FAST = false;
  int use_priors = 1;
  int matches;
  int FOV_degrees = 50;

  int disparity_histogram[3][SVS_MAX_IMAGE_WIDTH];

  // Port to start streaming from - second video will be on this + 1
  int start_port = 5000;

  AnyOption *opt = new AnyOption();
  assert(opt != NULL);

  // help
  opt->addUsage( "Example: " );
  opt->addUsage( "  v4l2stereo -0 /dev/video1 -1 /dev/video0 -w 320 -h 240 --features" );
  opt->addUsage( " " );
  opt->addUsage( "Usage: " );
  opt->addUsage( "" );
  opt->addUsage( " -0  --dev0                     Video device number of the left camera");
  opt->addUsage( " -1  --dev1                     Video device number of the right camera");
  opt->addUsage( " -w  --width                    Image width in pixels");
  opt->addUsage( " -h  --height                   Image height in pixels");
  opt->addUsage( " -x  --offsetx                  Calibration x offset in pixels");
  opt->addUsage( " -y  --offsety                  Calibration y offset in pixels");
  opt->addUsage( " -d  --disparity                Max disparity as a percent of image width");
  opt->addUsage( "     --baseline                 Baseline distance in millimetres");
  opt->addUsage( "     --equal                    Perform histogram equalisation");
  opt->addUsage( "     --ground                   y coordinate of the ground plane as percent of image height");
  opt->addUsage( "     --features                 Show stereo features");
  opt->addUsage( "     --disparitymap             Show dense disparity map");
  opt->addUsage( "     --disparitystep            Disparity step size in pixels for dense stereo");
  opt->addUsage( "     --disparitythreshold       Threshold applied to the disparity map as a percentage of max disparity");
  opt->addUsage( "     --smoothing                Smoothing radius in pixels for dense stereo");
  opt->addUsage( "     --patchsize                Correlation patch radius in pixels for dense stereo");
  opt->addUsage( "     --crosscheck               Threshold used for dense stereo pixel cross checking");
  opt->addUsage( "     --zoom                     Zoom level given as a percentage");
  opt->addUsage( "     --matches                  Show stereo matches");
  opt->addUsage( "     --regions                  Show regions");
  opt->addUsage( "     --depth                    Show depth map");
  opt->addUsage( "     --lines                    Show lines");
  opt->addUsage( "     --anaglyph                 Show anaglyph");
  opt->addUsage( "     --histogram                Show disparity histogram");
  opt->addUsage( "     --calibrate                Calibrate offsets");
  opt->addUsage( "     --cd0x                     Centre of distortion x coord for the left camera");
  opt->addUsage( "     --cd0y                     Centre of distortion y coord for the left camera");
  opt->addUsage( "     --cd1x                     Centre of distortion x coord for the right camera");
  opt->addUsage( "     --cd1y                     Centre of distortion y coord for the right camera");
  opt->addUsage( "     --coeff00                  Distortion coefficient 0 for the left camera");
  opt->addUsage( "     --coeff01                  Distortion coefficient 1 for the left camera");
  opt->addUsage( "     --coeff02                  Distortion coefficient 2 for the left camera");
  opt->addUsage( "     --coeff10                  Distortion coefficient 0 for the right camera");
  opt->addUsage( "     --coeff11                  Distortion coefficient 1 for the right camera");
  opt->addUsage( "     --coeff12                  Distortion coefficient 2 for the right camera");
  opt->addUsage( "     --rot0                     Calibration rotation of the left camera in radians");
  opt->addUsage( "     --rot1                     Calibration rotation of the right camera in radians");
  opt->addUsage( "     --scale0                   Calibration scale of the left camera");
  opt->addUsage( "     --scale1                   Calibration scale of the right camera");
  opt->addUsage( "     --fast                     Show FAST corners");
  opt->addUsage( "     --descriptors              Saves feature descriptor for each FAST corner");
  opt->addUsage( "     --fov                      Field of view in degrees");
  opt->addUsage( " -f  --fps                      Frames per second");
  opt->addUsage( " -s  --skip                     Skip this number of frames");
  opt->addUsage( " -i  --input                    Loads stereo matches from the given output file");
  opt->addUsage( " -o  --output                   Saves stereo matches to the given output file");
  opt->addUsage( "     --log                      Logs stereo matches to the given output file (only when no file exists)");
  opt->addUsage( " -V  --version                  Show version number");
  opt->addUsage( "     --save                     Save raw images");
  opt->addUsage( "     --flipright                Flip the right image");
  opt->addUsage( "     --flipleft                 Flip the left image");
  opt->addUsage( "     --stream                   Stream output using gstreamer");
  opt->addUsage( "     --headless                 Disable video output (for use with --stream)");
  opt->addUsage( "     --help                     Show help");
  opt->addUsage( "" );

  opt->setOption(  "ground" );
  opt->setOption(  "cd0x" );
  opt->setOption(  "cd0y" );
  opt->setOption(  "cd1x" );
  opt->setOption(  "cd1y" );
  opt->setOption(  "coeff00" );
  opt->setOption(  "coeff01" );
  opt->setOption(  "coeff02" );
  opt->setOption(  "coeff10" );
  opt->setOption(  "coeff11" );
  opt->setOption(  "coeff12" );
  opt->setOption(  "fast" );
  opt->setOption(  "descriptors" );
  opt->setOption(  "rot0" );
  opt->setOption(  "rot1" );
  opt->setOption(  "save" );
  opt->setOption(  "scale0" );
  opt->setOption(  "scale1" );
  opt->setOption(  "fps", 'f' );
  opt->setOption(  "dev0", '0' );
  opt->setOption(  "dev1", '1' );
  opt->setOption(  "width", 'w' );
  opt->setOption(  "height", 'h' );
  opt->setOption(  "offsetx", 'x' );
  opt->setOption(  "offsety", 'y' );
  opt->setOption(  "disparity", 'd' );
  opt->setOption(  "input", 'i' );
  opt->setOption(  "output", 'o' );
  opt->setOption(  "log" );
  opt->setOption(  "skip", 's' );
  opt->setOption(  "fov" );
  opt->setOption(  "disparitystep" );
  opt->setOption(  "smoothing" );
  opt->setOption(  "patchsize" );
  opt->setOption(  "disparitythreshold" );
  opt->setOption(  "crosscheck" );
  opt->setOption(  "zoom" );
  opt->setOption(  "baseline" );
  opt->setFlag(  "help" );
  opt->setFlag(  "flipleft" );
  opt->setFlag(  "flipright" );
  opt->setFlag(  "features" );
  opt->setFlag(  "regions" );
  opt->setFlag(  "matches" );
  opt->setFlag(  "depth" );
  opt->setFlag(  "lines" );
  opt->setFlag(  "anaglyph" );
  opt->setFlag(  "histogram" );
  opt->setFlag(  "calibrate" );
  opt->setFlag(  "version", 'V' );
  opt->setFlag(  "stream"  );
  opt->setFlag(  "headless"  );
  opt->setFlag(  "disparitymap"  );
  opt->setFlag(  "equal"  );

  opt->processCommandArgs(argc, argv);

  if( ! opt->hasOptions())
  {
      // print usage if no options
      opt->printUsage();
      delete opt;
      return(0);
  }

  if( opt->getFlag( "version" ) || opt->getFlag( 'V' ) )
  {
      printf("Version %f\n", VERSION);
      delete opt;
      return(0);
  }

  bool stream = false;
  if( opt->getFlag( "stream" ) ) {
	  stream = true;
  }

  bool headless = false;
  if( opt->getFlag( "headless" ) ) {
      headless = true;
  }

  bool flip_left_image = false;
  if( opt->getFlag( "flipleft" ) )
  {
	  flip_left_image = true;
  }

  bool flip_right_image = false;
  if( opt->getFlag( "flipright" ) )
  {
	  flip_right_image = true;
  }

  bool histogram_equalisation = false;
  if( opt->getFlag( "equal" ) )
  {
	  histogram_equalisation = true;
  }

  bool save_images = false;
  std::string save_filename = "";
  if( opt->getValue( "save" ) != NULL  ) {
  	  save_filename = opt->getValue("save");
  	  if (save_filename == "") save_filename = "image_";
  	  save_images = true;
  }

  if( opt->getFlag( "help" ) ) {
      opt->printUsage();
      delete opt;
      return(0);
  }

  if( opt->getFlag( "disparitymap" ) ) {
	  show_regions = false;
	  show_features = false;
	  show_matches = false;
	  show_depthmap = false;
	  show_anaglyph = false;
	  show_histogram = false;
	  show_lines = false;
	  show_FAST = false;
	  show_disparity_map = true;
  }

  if( opt->getFlag( "features" ) ) {
	  show_regions = false;
	  show_features = true;
	  show_matches = false;
	  show_depthmap = false;
	  show_anaglyph = false;
	  show_histogram = false;
	  show_lines = false;
	  show_FAST = false;
	  show_disparity_map = false;
  }

  if( opt->getFlag( "histogram" ) ) {
	  show_regions = false;
	  show_features = false;
	  show_matches = false;
	  show_depthmap = false;
	  show_anaglyph = false;
	  show_histogram = true;
	  show_lines = false;
	  show_FAST = false;
	  show_disparity_map = false;
  }

  if( opt->getFlag( "matches" ) ) {
	  show_regions = false;
	  show_features = false;
	  show_matches = true;
	  show_depthmap = false;
	  show_anaglyph = false;
	  show_histogram = false;
	  show_lines = false;
	  show_FAST = false;
	  show_disparity_map = false;
  }

  if( opt->getFlag( "regions" ) ) {
	  show_regions = true;
	  show_features = false;
	  show_matches = false;
	  show_depthmap = false;
	  show_anaglyph = false;
	  show_histogram = false;
	  show_lines = false;
	  show_FAST = false;
	  show_disparity_map = false;
  }

  if( opt->getFlag( "depth" ) ) {
	  show_regions = false;
	  show_features = false;
	  show_matches = false;
	  show_depthmap = true;
	  show_anaglyph = false;
	  show_histogram = false;
	  show_lines = false;
	  show_FAST = false;
	  show_disparity_map = false;
  }

  if( opt->getFlag( "lines" ) ) {
	  show_regions = false;
	  show_features = false;
	  show_matches = false;
	  show_depthmap = false;
	  show_anaglyph = false;
	  show_histogram = false;
	  show_lines = true;
	  show_FAST = false;
	  show_disparity_map = false;
  }

  if( opt->getFlag( "anaglyph" ) ) {
	  show_regions = false;
	  show_features = false;
	  show_matches = false;
	  show_depthmap = false;
	  show_anaglyph = true;
	  show_histogram = false;
	  show_lines = false;
	  show_FAST = false;
	  show_disparity_map = false;
  }

  bool calibrate_offsets = false;
  if( opt->getFlag( "calibrate" ) ) {
	  show_regions = false;
	  calibrate_offsets = true;
	  show_features = false;
	  show_matches = false;
	  show_depthmap = false;
	  show_anaglyph = false;
	  show_histogram = false;
	  show_lines = false;
	  show_FAST = false;
	  show_disparity_map = false;
  }

  int desired_corner_features = 70;
  if( opt->getValue( "fast" ) != NULL  ) {
	  show_regions = false;
	  calibrate_offsets = false;
	  show_features = false;
	  show_matches = false;
	  show_depthmap = false;
	  show_anaglyph = false;
	  show_histogram = false;
	  show_lines = false;
	  show_FAST = true;
	  show_disparity_map = false;
	  desired_corner_features = atoi(opt->getValue("fast"));
	  if (desired_corner_features > 150) desired_corner_features=150;
	  if (desired_corner_features < 50) desired_corner_features=50;
  }

  int enable_ground_priors = 0;
  int ground_y_percent = 50;
  if( opt->getValue( "ground" ) != NULL  ) {
	  enable_ground_priors = 1;
	  ground_y_percent = atoi(opt->getValue("ground"));
  }

  if( opt->getValue( "fov" ) != NULL  ) {
  	  FOV_degrees = atoi(opt->getValue("fov"));
  }

  std::string dev0 = "/dev/video1";
  if( opt->getValue( '0' ) != NULL  || opt->getValue( "dev0" ) != NULL  ) {
  	  dev0 = opt->getValue("dev0");
  }

  std::string dev1 = "/dev/video2";
  if( opt->getValue( '1' ) != NULL  || opt->getValue( "dev1" ) != NULL  ) {
  	  dev1 = opt->getValue("dev1");
  }

  if( opt->getValue( 'w' ) != NULL  || opt->getValue( "width" ) != NULL  ) {
  	  ww = atoi(opt->getValue("width"));
  }

  if( opt->getValue( 'h' ) != NULL  || opt->getValue( "height" ) != NULL  ) {
  	  hh = atoi(opt->getValue("height"));
  }

  int calibration_offset_x = -16;
  if( opt->getValue( 'x' ) != NULL  || opt->getValue( "offsetx" ) != NULL  ) {
  	  calibration_offset_x = atoi(opt->getValue("offsetx"));
  }

  int calibration_offset_y = 2;
  if( opt->getValue( 'y' ) != NULL  || opt->getValue( "offsety" ) != NULL  ) {
  	  calibration_offset_y = atoi(opt->getValue("offsety"));
  }

  int max_disparity_percent = 40;
  if( opt->getValue( 'd' ) != NULL  || opt->getValue( "disparity" ) != NULL  ) {
	  max_disparity_percent = atoi(opt->getValue("disparity"));
	  if (max_disparity_percent < 2) max_disparity_percent = 2;
	  if (max_disparity_percent > 90) max_disparity_percent = 90;
  }

  float centre_of_distortion_x0 = ww/2;
  if( opt->getValue( "cd0x" ) != NULL  ) {
	  centre_of_distortion_x0 = atof(opt->getValue("cd0x"));
	  rectify_images = true;
  }

  float centre_of_distortion_y0 = hh/2;
  if( opt->getValue( "cd0y" ) != NULL  ) {
	  centre_of_distortion_y0 = atof(opt->getValue("cd0y"));
	  rectify_images = true;
  }

  float centre_of_distortion_x1 = ww/2;
  if( opt->getValue( "cd1x" ) != NULL  ) {
	  centre_of_distortion_x1 = atof(opt->getValue("cd1x"));
	  rectify_images = true;
  }

  float centre_of_distortion_y1 = hh/2;
  if( opt->getValue( "cd1y" ) != NULL  ) {
	  centre_of_distortion_y1 = atof(opt->getValue("cd1y"));
	  rectify_images = true;
  }

  float coeff[2][3];

  /* surveyor svs */
  coeff[0][0] = (float)1.03159213066101;
  coeff[0][1] = (float)-1.04955497590709E-05;
  coeff[0][2] = (float)-4.3939662646153E-06;
  coeff[1][0] = (float)1.03159213066101;
  coeff[1][1] = (float)-1.04955497590709E-05;
  coeff[1][2] = (float)-4.3939662646153E-06;

  /* minoru */
  coeff[0][0] = (float)1.14295991712303;
  coeff[0][1] = (float)-0.00178321008445744;
  coeff[0][2] = (float)2.97387766992807E-06;
  coeff[1][0] = (float)1.14295991712303;
  coeff[1][1] = (float)-0.00178321008445744;
  coeff[1][2] = (float)2.97387766992807E-06;

  if( opt->getValue( "coeff00" ) != NULL  ) {
	  coeff[0][0] = atof(opt->getValue("coeff00"));
	  rectify_images = true;
  }
  if( opt->getValue( "coeff01" ) != NULL  ) {
	  coeff[0][1] = atof(opt->getValue("coeff01"));
	  rectify_images = true;
  }
  if( opt->getValue( "coeff02" ) != NULL  ) {
      coeff[0][2] = atof(opt->getValue("coeff02"));
      rectify_images = true;
  }
  if( opt->getValue( "coeff10" ) != NULL  ) {
	  coeff[1][0] = atof(opt->getValue("coeff10"));
	  rectify_images = true;
  }
  if( opt->getValue( "coeff11" ) != NULL  ) {
	  coeff[1][1] = atof(opt->getValue("coeff11"));
	  rectify_images = true;
  }
  if( opt->getValue( "coeff12" ) != NULL  ) {
	  coeff[1][2] = atof(opt->getValue("coeff12"));
	  rectify_images = true;
  }

  float rotation0 = 0;
  if( opt->getValue( "rot0" ) != NULL  ) {
	  rotation0 = atof(opt->getValue("rot0"));
	  rectify_images = true;
  }
  float rotation1 = 0;
  if( opt->getValue( "rot1" ) != NULL  ) {
	  rotation1 = atof(opt->getValue("rot1"));
	  rectify_images = true;
  }

  float scale0 = 1;
  if( opt->getValue( "scale0" ) != NULL  ) {
	  scale0 = atof(opt->getValue("scale0"));
	  rectify_images = true;
  }
  float scale1 = 1;
  if( opt->getValue( "scale1" ) != NULL  ) {
	  scale1 = atof(opt->getValue("scale1"));
	  rectify_images = true;
  }

  int fps = 30;
  if( opt->getValue( 'f' ) != NULL  || opt->getValue( "fps" ) != NULL  ) {
	  fps = atoi(opt->getValue("fps"));
  }

  std::string descriptors_filename = "";
  if( opt->getValue( "descriptors" ) != NULL  ) {
	  descriptors_filename = opt->getValue("descriptors");
  }

  std::string stereo_matches_filename = "";
  if( opt->getValue( 'o' ) != NULL  || opt->getValue( "output" ) != NULL  ) {
	  stereo_matches_filename = opt->getValue("output");
	  skip_frames = 6;
  }

  std::string stereo_matches_input_filename = "";
  if( opt->getValue( 'i' ) != NULL  || opt->getValue( "input" ) != NULL  ) {
	  stereo_matches_input_filename = opt->getValue("input");
  }

  std::string log_stereo_matches_filename = "";
  if( opt->getValue( "log" ) != NULL  ) {
	  log_stereo_matches_filename = opt->getValue("log");
  }

  if( opt->getValue( 's' ) != NULL  || opt->getValue( "skip" ) != NULL  ) {
	  skip_frames = atoi(opt->getValue("skip"));
  }

  // disparity step size in pixels for dense stereo
  int disparity_step = 8;
  if( opt->getValue( "disparitystep" ) != NULL  ) {
	  disparity_step = atoi(opt->getValue("disparitystep"));
	  if (disparity_step < 1) disparity_step = 1;
	  if (disparity_step > 20) disparity_step = 20;
  }

  // radius used for patch matching in dense stereo
  int disparity_map_correlation_radius = 1;
  if( opt->getValue( "patchsize" ) != NULL  ) {
	  disparity_map_correlation_radius = atoi(opt->getValue("patchsize"));
	  if (disparity_map_correlation_radius < 1) disparity_map_correlation_radius = 1;
	  if (disparity_map_correlation_radius > 10) disparity_map_correlation_radius = 10;
  }

  // radius for disparity space smoothing in dense stereo
  int disparity_map_smoothing_radius = 2;
  if( opt->getValue( "smoothing" ) != NULL  ) {
	  disparity_map_smoothing_radius = atoi(opt->getValue("smoothing"));
	  if (disparity_map_smoothing_radius < 1) disparity_map_smoothing_radius = 1;
	  if (disparity_map_smoothing_radius > 10) disparity_map_smoothing_radius = 10;
  }

  // disparity map threshold as a percentage
  int disparity_threshold_percent = 0;
  if( opt->getValue( "disparitythreshold" ) != NULL  ) {
	  disparity_threshold_percent = atoi(opt->getValue("disparitythreshold"));
	  if (disparity_threshold_percent < 0) disparity_threshold_percent = 0;
	  if (disparity_threshold_percent > 100) disparity_threshold_percent = 100;
  }

  // cross checking threshold
  int cross_checking_threshold = 50;
  if( opt->getValue( "crosscheck" ) != NULL  ) {
	  cross_checking_threshold = atoi(opt->getValue("crosscheck"));
	  if (cross_checking_threshold < 2) cross_checking_threshold = 2;
	  if (cross_checking_threshold > 100) cross_checking_threshold = 100;
  }

  // baseline distance
  int baseline_mm = 60;
  if( opt->getValue( "baseline" ) != NULL  ) {
	  baseline_mm = atoi(opt->getValue("baseline"));
	  if (baseline_mm < 10) baseline_mm = 10;
  }

  // zoom percentage
  int zoom = 0;
  if( opt->getValue( "zoom" ) != NULL  ) {
	  zoom = atoi(opt->getValue("zoom"));
	  if (zoom < 0) zoom = 0;
	  if (zoom > 100) zoom = 100;
  }
  int zoom_tx = zoom * ((ww/2)*80/100) / 100;
  int zoom_ty = zoom * ((hh/2)*80/100) / 100;
  int zoom_bx = ww - zoom_tx;
  int zoom_by = hh - zoom_ty;

  // adjust offsets to compensate for the zoom
  calibration_offset_x = calibration_offset_x * ww / (zoom_bx - zoom_tx);
  calibration_offset_y = calibration_offset_y * hh / (zoom_by - zoom_ty);

  delete opt;

  Camera c(dev0.c_str(), ww, hh, fps);
  Camera c2(dev1.c_str(), ww, hh, fps);

  std::string left_image_title = "Left image";
  std::string right_image_title = "Right image";

  if (show_features) {
	  left_image_title = "Left image features";
	  right_image_title = "Right image features";
  }
  if (show_regions) {
	  left_image_title = "Left image regions";
	  right_image_title = "Right image regions";
  }
  if (show_FAST) left_image_title = "FAST corners";
  if (show_matches) left_image_title = "Stereo matches";
  if (show_depthmap) left_image_title = "Depth map";
  if (show_histogram) right_image_title = "Disparity histograms (L/R/All)";
  if (show_anaglyph) left_image_title = "Anaglyph";
  if (show_disparity_map) left_image_title = "Disparity map";

//cout<<c.setSharpness(3)<<"   "<<c.minSharpness()<<"  "<<c.maxSharpness()<<" "<<c.defaultSharpness()<<endl;

  if ((!save_images) &&
	  (!calibrate_offsets) && (!headless) &&
	  (stereo_matches_filename == "")) {

      cvNamedWindow(left_image_title.c_str(), CV_WINDOW_AUTOSIZE);
      if ((!show_matches) &&
    	  (!show_FAST) &&
    	  (!show_depthmap) &&
    	  (!show_anaglyph) &&
    	  (!show_disparity_map)) {
          cvNamedWindow(right_image_title.c_str(), CV_WINDOW_AUTOSIZE);
      }
  }

  IplImage *l=cvCreateImage(cvSize(ww, hh), 8, 3);
  unsigned char *l_=(unsigned char *)l->imageData;

  IplImage *r=cvCreateImage(cvSize(ww, hh), 8, 3);
  unsigned char *r_=(unsigned char *)r->imageData;

  /* feature detection params */
  int inhibition_radius = 6;
  unsigned int minimum_response = 250;

  /* matching params */
  int ideal_no_of_matches = 400;

  /* These weights are used during matching of stereo features.
   * You can adjust them if you wish */
  int learnDesc = 18*5;  /* weight associated with feature descriptor match */
  int learnLuma = 7*5;   /* weight associated with luminance match */
  int learnDisp = 1;   /* weight associated with disparity (bias towards smaller disparities) */
  int learnGrad = 4;  /* weight associated with horizontal gradient */
  int groundPrior = 200; /* weight for ground plane prior */

  svs* lcam = new svs(ww, hh);
  svs* rcam = new svs(ww, hh);
  //motionmodel* motion = new motionmodel();
  fast* corners_left = new fast();

  unsigned char* rectification_buffer = NULL;
  unsigned char* depthmap_buffer = NULL;

  linefit *lines = new linefit();

  /*
   * Send the video over a network for use in embedded applications
   * using the gstreamer library.
   */
  GstElement* l_source = NULL;
  GstElement* r_source = NULL;
  GstBuffer* l_app_buffer = NULL;
  GstBuffer* r_app_buffer = NULL;
  GstFlowReturn ret;

  // Yuck
  std::stringstream lp_str;
  lp_str << start_port;
  std::stringstream rp_str;
  rp_str << start_port + 1;

  std::string caps;

  if( stream ) {
	// Initialise gstreamer and glib
      	gst_init( NULL, NULL );
	GError* l_error = 0;
	GError* r_error = 0;
	GstElement* l_pipeline = 0;
	GstElement* r_pipeline = 0;

        caps = "image/jpeg";

	// Can replace this pipeline with anything you like (udpsink, videowriters etc)
	std::string l_pipetext = "appsrc name=appsource caps="+ caps +
	    " ! jpegdec ! ffmpegcolorspace ! queue ! jpegenc ! multipartmux ! tcpserversink port=" + lp_str.str();
	std::string r_pipetext = "appsrc name=appsource caps="+ caps +
	    " ! jpegdec ! ffmpegcolorspace ! queue ! jpegenc ! multipartmux ! tcpserversink port=" + rp_str.str();

	// Create the left image pipeline
	l_pipeline = gst_parse_launch( l_pipetext.c_str(), &l_error );

	// If needed, create right image pipeline
	if ((!show_matches) &&
		(!show_FAST) &&
		(!show_depthmap) &&
		(!show_anaglyph) &&
		(!show_disparity_map)) {
	    r_pipeline = gst_parse_launch( r_pipetext.c_str(), &r_error );
	}

	// Seperate errors in case of port clash
	if( l_error == NULL ) {
	    l_source = gst_bin_get_by_name( GST_BIN( l_pipeline ), "appsource" );
	    gst_app_src_set_caps( (GstAppSrc*) l_source, gst_caps_from_string( caps.c_str() ) );
	    gst_element_set_state( l_pipeline, GST_STATE_PLAYING );
	    cout << "Streaming started on port " << start_port << endl;
	    cout << "Watch stream with the command:" << endl;
	    cout << "gst-launch tcpclientsrc host=[ip] port=" << start_port << " ! multipartdemux ! jpegdec ! autovideosink" << endl;
	} else {
	    cout << "A gstreamer error occurred: " << l_error->message << endl;
	}

	// Cannot rely on pipeline, as there maybe a situation where the pipeline is null
	if( (!show_matches) &&
		(!show_FAST) &&
		(!show_depthmap) &&
		(!show_anaglyph) &&
		(!show_disparity_map)) {
	    if( r_error == NULL ) {
		r_source = gst_bin_get_by_name( GST_BIN( r_pipeline ), "appsource" );
		gst_app_src_set_caps( (GstAppSrc*) r_source, gst_caps_from_string( caps.c_str() ) );
		gst_element_set_state( r_pipeline, GST_STATE_PLAYING );
		cout << "Streaming started on port " << start_port + 1 << endl;
		cout << "Watch stream with the command:" << endl;
		cout << "gst-launch tcpclientsrc host=[ip] port=" << start_port + 1 << " ! multipartdemux ! jpegdec ! autovideosink" << endl;
	    } else {
		cout << "A gstreamer error occurred: " << r_error->message << endl;
	    }
	}
  }

  // dense disparity
  unsigned int* disparity_space = NULL;
  unsigned int* disparity_map = NULL;

  IplImage* hist_image0 = NULL;
  IplImage* hist_image1 = NULL;

  while(1){

    while(c.Get()==0 || c2.Get()==0) usleep(100);

    c.toIplImage(l);
    c2.toIplImage(r);

    if (flip_right_image) {
    	if (rectification_buffer == NULL) {
    		rectification_buffer = new unsigned char[ww * hh * 3];
    	}
    	rcam->flip(r_, rectification_buffer);
    }

    if (flip_left_image) {
    	if (rectification_buffer == NULL) {
    		rectification_buffer = new unsigned char[ww * hh * 3];
    	}
    	lcam->flip(l_, rectification_buffer);
    }

    if (rectify_images) {
    	if (rectification_buffer == NULL) {
    		rectification_buffer = new unsigned char[ww * hh * 3];

    		/* create mappings for image rectification */
            lcam->make_map(centre_of_distortion_x0, centre_of_distortion_y0, coeff[0][0], coeff[0][1], coeff[0][2], rotation0, scale0);
            rcam->make_map(centre_of_distortion_x1, centre_of_distortion_y1, coeff[1][0], coeff[1][1], coeff[1][2], rotation1, scale1);
    	}
        lcam->rectify(l_, rectification_buffer);
        memcpy(l_, rectification_buffer, ww * hh * 3 * sizeof(unsigned char));
        rcam->rectify(r_, rectification_buffer);
        memcpy(r_, rectification_buffer, ww * hh * 3 * sizeof(unsigned char));
    }

	if (zoom > 0) {

		unsigned char l2_[ww*hh*3];
		unsigned char r2_[ww*hh*3];
		memcpy((void*)l2_,l_,ww*hh*3);
		memcpy((void*)r2_,r_,ww*hh*3);

		stereodense::expand(l2_,ww,hh,zoom_tx,zoom_ty,zoom_bx,zoom_by,l_);
		stereodense::expand(r2_,ww,hh,zoom_tx,zoom_ty,zoom_bx,zoom_by,r_);
	}

	if (histogram_equalisation) {

		if (hist_image0 == NULL) {
			hist_image0 = cvCreateImage( cvGetSize(l), IPL_DEPTH_8U, 1 );
			hist_image1 = cvCreateImage( cvGetSize(l), IPL_DEPTH_8U, 1 );
		}

        #pragma omp parallel for
		for (int i = 0; i < 2; i++) {
			unsigned char *img = l_;
			IplImage* hist_image = hist_image0;
			if (i > 0) {
				img = r_;
				hist_image = hist_image1;
			}
			svs::histogram_equalise(
				hist_image,
				img, ww, hh);
		}

	}

    int calib_offset_x = calibration_offset_x;
    int calib_offset_y = calibration_offset_y;
    unsigned char* rectified_frame_buf = NULL;
	for (int cam = 1; cam >= 0; cam--) {

		int no_of_feats = 0;
		int no_of_feats_horizontal = 0;
		svs* stereocam = NULL;
		if (cam == 0) {
			rectified_frame_buf = l_;
			stereocam = lcam;
		}
		else {
			rectified_frame_buf = r_;
			stereocam = rcam;
		}

		no_of_feats = stereocam->get_features_vertical(
	        rectified_frame_buf,
	        inhibition_radius,
	        minimum_response,
	        calib_offset_x,
	        calib_offset_y,
	        1-cam);

		if ((cam == 0) || (show_features) || (show_lines)) {
		    no_of_feats_horizontal = stereocam->get_features_horizontal(
	            rectified_frame_buf,
	            inhibition_radius,
	            minimum_response,
	            calib_offset_x,
	            calib_offset_y,
	            1-cam);
		}

		if (show_lines) {
            lines->vertically_oriented(
                no_of_feats,
        	    stereocam->feature_x,
        	    stereocam->features_per_row,
        	    SVS_VERTICAL_SAMPLING,
        	    10*320/SVS_MAX_IMAGE_WIDTH);
            lines->horizontally_oriented(
            	no_of_feats_horizontal,
        	    stereocam->feature_y,
        	    stereocam->features_per_col,
        	    SVS_HORIZONTAL_SAMPLING,
        	    6*320/SVS_MAX_IMAGE_WIDTH);
            for (int line = 0; line < lines->line_vertical[0]; line++) {
                drawing::drawLine(rectified_frame_buf,ww,hh,
                	lines->line_vertical[line*5 + 1] - calib_offset_x,
                	lines->line_vertical[line*5 + 2] - calib_offset_y,
                	lines->line_vertical[line*5 + 3] - calib_offset_x,
                	lines->line_vertical[line*5 + 4] - calib_offset_y,
                	255,0,0,
                	0,false);
            }
            for (int line = 0; line < lines->line_horizontal[0]; line++) {
                drawing::drawLine(rectified_frame_buf,ww,hh,
                	lines->line_horizontal[line*5 + 1] - calib_offset_x,
                	lines->line_horizontal[line*5 + 2] - calib_offset_y,
                	lines->line_horizontal[line*5 + 3] - calib_offset_x,
                	lines->line_horizontal[line*5 + 4] - calib_offset_y,
                	0,255,0,
                	0,false);
            }
		}

		//printf("cam %d:  %d\n", cam, no_of_feats);

		/* display the features */
		if (show_features) {

			/* vertically oriented features */
			int row = 0;
			int feats_remaining = stereocam->features_per_row[row];

			for (int f = 0; f < no_of_feats; f++, feats_remaining--) {

				int x = (int)stereocam->feature_x[f];
				int y = 4 + (row * SVS_VERTICAL_SAMPLING);

				if (cam == 0) {
				    drawing::drawCross(rectified_frame_buf, ww, hh, x, y, 2, 0, 0, 255, 0);
				}
				else {
					x -= calibration_offset_x;
					y += calibration_offset_y;
				    drawing::drawCross(rectified_frame_buf, ww, hh, x, y, 2, 255, 0, 0, 0);
				}

				/* move to the next row */
				if (feats_remaining <= 0) {
					row++;
					feats_remaining = stereocam->features_per_row[row];
				}
			}

			/* horizontally oriented features */
			int col = 0;
			feats_remaining = stereocam->features_per_col[col];

			for (int f = 0; f < no_of_feats_horizontal; f++, feats_remaining--) {

				int y = (int)stereocam->feature_y[f];
				int x = 4 + (col * SVS_HORIZONTAL_SAMPLING);

				if (cam == 0) {
				    drawing::drawCross(rectified_frame_buf, ww, hh, x, y, 2, 0, 255, 0, 0);
				}
				else {
					x += calibration_offset_x;
					y -= calibration_offset_y;
				    drawing::drawCross(rectified_frame_buf, ww, hh, x, y, 2, 0, 255, 0, 0);
				}

				/* move to the next column */
				if (feats_remaining <= 0) {
					col++;
					feats_remaining = stereocam->features_per_col[col];
				}
			}

		}

		calib_offset_x = 0;
		calib_offset_y = 0;
	}

	/* set ground plane parameters */
	lcam->enable_ground_priors = enable_ground_priors;
	lcam->ground_y_percent = ground_y_percent;

	matches = lcam->match(
		rcam,
		ideal_no_of_matches,
		max_disparity_percent,
		learnDesc,
		learnLuma,
		learnDisp,
		learnGrad,
		groundPrior,
		use_priors);


	if (show_regions) {
		lcam->enable_segmentation = 1;
		if (lcam->low_contrast != NULL) {
			lcam->segment(l_, matches);
			memset((void*)l_, '\0', ww*hh*3);
			int min_vol = ww*hh/500;
			int r=255, g=0, b=0;
			int i = 0;
			for (int y = 0; y < hh; y++) {
				for (int x = 0; x < ww; x++, i++) {
					int ID = lcam->low_contrast[i];
					if ((ID > 0) && (ID < lcam->no_of_regions )) {
						if ((int)lcam->region_volume[ID] > min_vol) {
							int disp = lcam->region_disparity[ID*3];
							int slope_x = (int)lcam->region_disparity[ID*3+1] - 127;
							int slope_y = (int)lcam->region_disparity[ID*3+2] - 127;
							if (disp != 255) {
								if (!((slope_x == 0) && (slope_y == 0))) {
									int region_tx = lcam->region_bounding_box[ID*4];
									int region_ty = lcam->region_bounding_box[ID*4+1];
									int region_bx = lcam->region_bounding_box[ID*4+2];
									int region_by = lcam->region_bounding_box[ID*4+3];
								    int disp_horizontal = 0;
								    if (region_bx > region_tx) {
								        disp_horizontal =
								    	    -(slope_x/2) + ((x - region_tx) * slope_x /
								             (region_bx - region_tx));
								    }
								    int disp_vertical = 0;
								    if (region_by > region_ty) {
								        disp_vertical =
								    	    -(slope_y/2) + ((y - region_ty) * slope_y /
								             (region_by - region_ty));
								    }
								    disp += disp_horizontal + disp_vertical;
								    if (disp < 0) disp = 0;
								}
								r = 20+disp*5;
								if (r > 255) r = 255;
								g = r;
								b = r;
								l_[i*3] = b;
								l_[i*3+1] = g;
								l_[i*3+2] = r;
							}
							/*
							r = lcam->region_colour[ID*3+2];
							g = lcam->region_colour[ID*3+1];
							b = lcam->region_colour[ID*3];
							l_[i*3] = b;
							l_[i*3+1] = g;
							l_[i*3+2] = r;
							*/
						}
					}
			    }
		    }

			/*
			for (int i = 0; i < lcam->no_of_regions; i++) {
				if ((int)lcam->region_volume[i] > min_vol) {
					drawing::drawCross(
							l_, ww, hh,
							(int)lcam->region_centre[i*2],
							(int)lcam->region_centre[i*2+1],
							4, 255,0,0, 1);
				}
			} */

			if (lcam->region_history_index > -1) {
				for (i = 0; i < lcam->prev_region_centre[lcam->region_history_index][0]; i++) {
					int ctr = lcam->region_history_index;
					int j0 = lcam->prev_region_centre[ctr][i*4+3];
					int j = j0;
					int k = lcam->prev_region_centre[ctr][i*4+4];
					int prev_x = lcam->prev_region_centre[ctr][i*4+1];
					int prev_y = lcam->prev_region_centre[ctr][i*4+2];

					int n = 0;
					while ((j != 65535) && (n < SVS_REGION_HISTORY-1)) {
						int x = lcam->prev_region_centre[j][k*4+1];
						int y = lcam->prev_region_centre[j][k*4+2];
						int j2 = lcam->prev_region_centre[j][k*4+3];
						k = lcam->prev_region_centre[j][k*4+4];
						j = j2;
						if (j == lcam->region_history_index) break;
						drawing::drawLine(l_,ww,hh,prev_x,prev_y,x,y,0,255,0,1,false);
						prev_x = x;
						prev_y = y;
						n++;
					}
				}
			}
		}
	}

	/* show disparity histogram */
	if (show_histogram) {
	    memset(disparity_histogram[0], 0, SVS_MAX_IMAGE_WIDTH * sizeof(int));
	    memset(disparity_histogram[1], 0, SVS_MAX_IMAGE_WIDTH * sizeof(int));
	    memset(disparity_histogram[2], 0, SVS_MAX_IMAGE_WIDTH * sizeof(int));
	    memset(r_, 0, ww * hh * 3 * sizeof(unsigned char));
	    int hist_max[3];
	    hist_max[0] = 0;
	    hist_max[1] = 0;
	    hist_max[2] = 0;

		for (int i = 0; i < matches; i++) {
			int x = lcam->svs_matches[i*5 + 1]/SVS_SUB_PIXEL;
			int disp = lcam->svs_matches[i*5 + 3]/SVS_SUB_PIXEL;
			disparity_histogram[2][disp]++;
			if (x < ww/2)
				disparity_histogram[0][disp]++;
			else
				disparity_histogram[1][disp]++;
			if (disparity_histogram[0][disp] > hist_max[0]) hist_max[0] = disparity_histogram[0][disp];
			if (disparity_histogram[1][disp] > hist_max[1]) hist_max[1] = disparity_histogram[1][disp];
			if (disparity_histogram[2][disp] > hist_max[2]) hist_max[2] = disparity_histogram[2][disp];
		}
		int max_disparity_pixels = max_disparity_percent * ww / 100;

		int mass[3];
		mass[0] = 0;
		mass[1] = 0;
		mass[2] = 0;
		int disp2[3];
		disp2[0] = 0;
		disp2[1] = 0;
		disp2[2] = 0;
		int hist_thresh[3];
		hist_thresh[0] = hist_max[0] / 4;
		hist_thresh[1] = hist_max[1] / 4;
		hist_thresh[2] = hist_max[2] / 4;
		for (int d = 3; d < max_disparity_pixels-1; d++) {
			for (int i = 0; i < 3; i++) {
				if (disparity_histogram[i][d] > hist_thresh[i]) {
					int m = disparity_histogram[i][d] + disparity_histogram[i][d-1] + disparity_histogram[i][d+1];
					mass[i] += m;
					disp2[i] += m * d;
				}
			}
		}
		for (int i = 0; i < 3; i++) {
		    if (mass[i] > 0) disp2[i] /= mass[i];
		}

		int tx=0,ty=0,bx=0,by=0;
		for (int i = 0; i < 3; i++) {
			if (hist_max[i] > 0) {
				switch(i) {
					case 0: {
						tx = 0;
						ty = 0;
						bx = ww/2;
						by = hh/2;
						break;
					}
					case 1: {
						tx = ww/2;
						ty = 0;
						bx = ww;
						by = hh/2;
						break;
					}
					case 2: {
						tx = 0;
						ty = hh/2;
						bx = ww;
						by = hh;
						break;
					}
				}

				for (int x = tx; x < bx; x++) {
					int disp = (x-tx) * max_disparity_pixels / (bx-tx);
					int h2 = disparity_histogram[i][disp] * (by-ty) / hist_max[i];
					for (int y = by-1; y > by-1-h2; y--) {
						int n = ((y * ww) + x) * 3;
						r_[n] = 255;
						r_[n+1] = 255;
						r_[n+2] = 255;
					}
				}

				int xx = tx + (disp2[i] * (bx-tx) / max_disparity_pixels);
				drawing::drawLine(r_, ww, hh, xx, ty, xx, by-1, 255,0,0,0,false);
			}
		}

		drawing::drawLine(r_, ww, hh, ww/2, 0, ww/2, hh/2, 0,255,0,1,false);
		drawing::drawLine(r_, ww, hh, 0, hh/2, ww-1, hh/2, 0,255,0,1,false);
	}

    /* show disparity as spots */
    if (show_matches) {
        float baseline = 60.0f;
        float focal = 5.44f;
        for (int i = 0; i < matches; i++) {
            if ((lcam->svs_matches[i*5] > 0) &&
                    (lcam->svs_matches[i*5+4] != 9999)) {
                int x = lcam->svs_matches[i*5 + 1]/SVS_SUB_PIXEL;
                int y = lcam->svs_matches[i*5 + 2];
                int disp = lcam->svs_matches[i*5 + 3]/SVS_SUB_PIXEL;
                if (disp < ww/2) drawing::drawBlendedSpot(l_, ww, hh, x, y, 1 + (disp/6), 0, 255, 0);

                /* Calculate the cooridnates of all the matches */
                if (1) {
                    float distance = 0;
                    if (disp != 0) {
                        distance = focal*(baseline/(float)disp -1.0);
                    } else{
                         distance = 0.0f;
                    }

                    std::cout << "Distance of feature nr " << i << " at pos (x,y) " << x << "," << y << " is " << distance << std::endl;

                }

            }
        }
    }
    //For each mathc calculate the distance, dependant on the disparity
    

	if (show_disparity_map) {

		if (disparity_space == NULL) {
			int max_disparity_pixels = SVS_MAX_IMAGE_WIDTH * max_disparity_percent / 100;
			int disparity_space_length = (max_disparity_pixels / disparity_step) * SVS_MAX_IMAGE_WIDTH * ((SVS_MAX_IMAGE_HEIGHT/SVS_VERTICAL_SAMPLING)/disparity_map_smoothing_radius) * 2;
			int disparity_map_length = SVS_MAX_IMAGE_WIDTH * ((SVS_MAX_IMAGE_HEIGHT/SVS_VERTICAL_SAMPLING)/disparity_map_smoothing_radius) * 2;
		    disparity_space = new unsigned int[disparity_space_length];
		    disparity_map = new unsigned int[disparity_map_length];
		}

        stereodense::update_disparity_map(
                l_,r_,ww,hh,
                calibration_offset_x, calibration_offset_y,
                SVS_VERTICAL_SAMPLING,
                max_disparity_percent,
                disparity_map_correlation_radius,
                disparity_map_smoothing_radius,
                disparity_step,
                disparity_threshold_percent,
                true,
                cross_checking_threshold,
                disparity_space,
                disparity_map);

        stereodense::show(
                l_,ww,hh,
                SVS_VERTICAL_SAMPLING,
                disparity_map_smoothing_radius,
                max_disparity_percent,
                disparity_map);

        //cvSmooth( l, l, CV_GAUSSIAN, 9, 9 );
	}

	/* show depth map */
	if (show_depthmap) {
		if (depthmap_buffer == NULL) {
			depthmap_buffer = new unsigned char[ww*hh*3];
			memset(depthmap_buffer, 0, ww*hh*3*sizeof(unsigned char));
		}
		memset(l_, 0, ww*hh*3*sizeof(unsigned char));
		if (matches == 0) matches = prev_matches;
		for (int i = 0; i < matches; i++) {
			int x = lcam->svs_matches[i*5 + 1]/SVS_SUB_PIXEL;
			int y = lcam->svs_matches[i*5 + 2];
			int disp = lcam->svs_matches[i*5 + 3]/SVS_SUB_PIXEL;
			int max_disparity_pixels = max_disparity_percent * ww / 100;
			int disp_intensity = 50 + (disp * 300 / max_disparity_pixels);
			if (disp_intensity > 255) disp_intensity = 255;
			int radius = 10 + (disp/8);
			if (use_priors != 0) {
			    int n = (y*ww+x)*3;
			    int disp_intensity2 = disp_intensity;
			    disp_intensity = (disp_intensity + depthmap_buffer[n]) / 2;
		        drawing::drawBlendedSpot(depthmap_buffer, ww, hh, x, y, radius, disp_intensity2, disp_intensity2, disp_intensity2);
			}
		    drawing::drawBlendedSpot(l_, ww, hh, x, y, radius, disp_intensity, disp_intensity, disp_intensity);
		}
		prev_matches = matches;
	}

	if (show_anaglyph) {
		int n = 0;
		int max = (ww * hh * 3) - 3;
		for (int y = 0; y < hh; y++) {
			int y2 = y + calibration_offset_y;
			for (int x = 0; x < ww; x++, n += 3) {
				int x2 = x + calibration_offset_x;
				int n2 = ((y2 * ww) + x2) * 3;
				if ((n2 > -1) && (n2 < max)) {
					l_[n] = 0;
					l_[n+1] = l_[n+2];
					l_[n+2] = r_[n2+2];
				}
			}
		}
	}

	/* log stereo matches */
	if ((log_stereo_matches_filename != "")) {
		if (lcam->log_matches(log_stereo_matches_filename, l_, matches, true)) {
		    printf("%d stereo matches logged to %s\n", matches, log_stereo_matches_filename.c_str());
		}
	}

	if (skip_frames == 0) {

		/* save left and right images to file, then quit */
		if (save_images) {
			std::string filename = save_filename + "0.jpg";
			cvSaveImage(filename.c_str(), l);
			filename = save_filename + "1.jpg";
			if ((!show_matches) &&
				(!show_FAST) &&
				(!show_depthmap) &&
				(!show_anaglyph) &&
				(!show_disparity_map))
				cvSaveImage(filename.c_str(), r);

			/* save stereo matches */
			if ((stereo_matches_filename != "") && (!show_FAST) &&
			    ((skip_frames == 0) || (matches > 5))) {
				lcam->save_matches(stereo_matches_filename, l_, matches, true);
				printf("%d stereo matches saved to %s\n", matches, stereo_matches_filename.c_str());
			}

			break;
		}

		/* compute calibration offsets, display the results, then quit */
		if (calibrate_offsets) {
			int x_range = 25;
			int y_range = 25;
			lcam->calibrate_offsets(l_, r_, x_range, y_range, calibration_offset_x, calibration_offset_y);
			printf("Offset x: %d\n", calibration_offset_x);
			printf("Offset y: %d\n", calibration_offset_y);
			break;
		}
	}

	/* save stereo matches to a file, then quit */
	if ((stereo_matches_filename != "") && (!save_images) && (!show_FAST) &&
	    ((skip_frames == 0) || (matches > 5))) {
		lcam->save_matches(stereo_matches_filename, l_, matches, false);
		printf("%d stereo matches saved to %s\n", matches, stereo_matches_filename.c_str());
		break;
	}

	//motion->update(l_,ww,hh);
	//motion->show(l_,ww,hh);

	if (show_FAST) {
		/* load previous matches from file */
		if (stereo_matches_input_filename != "") {
			corners_left->load_matches(stereo_matches_input_filename, true);
			stereo_matches_input_filename = "";
		}

		/* locate corner features in the left image */
		corners_left->update(l_,ww,hh, desired_corner_features,1);

		/* assign disparity values to corner features */
		corners_left->match_interocular(
			ww, hh,
			matches, lcam->svs_matches);

		/* save stereo matches to a file, then quit */
		if ((stereo_matches_filename != "") && (!save_images) &&
			((skip_frames == 0) || (corners_left->get_no_of_disparities() > 50))) {
			/* save the matches */
			corners_left->save_matches(stereo_matches_filename, l_, ww, true);
			break;
		}

		/* save stereo matches to a file, then quit */
		if ((descriptors_filename != "") && (!save_images) &&
			((skip_frames == 0) || (corners_left->get_no_of_disparities() > 50))) {
			if (corners_left->save_descriptors(descriptors_filename, l_, ww, hh) > 40) {
			    break;
			}
		}

		corners_left->show(l_,ww,hh,1);
	}

    /*
     * The streaming bit - seems a bit hacky, someone else can try
     * and convert an IPLImage directly to something GStreamer can handle.
     * My bitbanging abilities just aren't up to the task.
     */
/*    if (stream) {
	    CvMat* l_buf;
	    l_buf = cvEncodeImage(".jpg", l);

	    l_app_buffer = gst_app_buffer_new( l_buf->data.ptr, l_buf->step, NULL, l_buf->data.ptr );
	    g_signal_emit_by_name( l_source, "push-buffer", l_app_buffer, &ret );

	    if ((!show_matches) &&
	    	(!show_FAST) &&
	    	(!show_depthmap) &&
	    	(!show_anaglyph) &&
	    	(!show_disparity_map)) {
		    CvMat* r_buf;
		    r_buf = cvEncodeImage(".jpg", r);

		    r_app_buffer = gst_app_buffer_new( r_buf->data.ptr, r_buf->step, NULL, r_buf->data.ptr );
		    g_signal_emit_by_name( r_source, "push-buffer", r_app_buffer, &ret );
	    }
    }
    */

	/* display the left and right images */
	if ((!save_images) && (!calibrate_offsets) && (!headless) && (stereo_matches_filename == "")) {
	    cvShowImage(left_image_title.c_str(), l);
	    if ((!show_matches) &&
	    	(!show_FAST) &&
	    	(!show_depthmap) &&
	    	(!show_anaglyph) &&
	    	(!show_disparity_map)) {
		    cvShowImage(right_image_title.c_str(), r);
	    }
	}

    skip_frames--;
    if (skip_frames < 0) skip_frames = 0;

    int wait = cvWaitKey(10) & 255;
    if( wait == 27 ) break;
  }

  /* destroy the left and right images */
  if ((!save_images) &&
	  (!calibrate_offsets) &&
	  (stereo_matches_filename == "")) {

	  cvDestroyWindow(left_image_title.c_str());
	  if ((!show_matches) &&
		  (!show_FAST) &&
		  (!show_depthmap) &&
		  (!show_anaglyph) &&
		  (!show_disparity_map)) {
	      cvDestroyWindow(right_image_title.c_str());
	  }
  }
  cvReleaseImage(&l);
  cvReleaseImage(&r);
  if (hist_image0 != NULL) cvReleaseImage(&hist_image0);
  if (hist_image1 != NULL) cvReleaseImage(&hist_image1);

  delete lcam;
  delete rcam;
  delete corners_left;
  delete lines;
  if (rectification_buffer != NULL) delete[] rectification_buffer;
  if (depthmap_buffer != NULL) delete[] depthmap_buffer;
  if (disparity_space != NULL) delete[] disparity_space;
  if (disparity_map != NULL) delete[] disparity_map;

  return 0;
}



