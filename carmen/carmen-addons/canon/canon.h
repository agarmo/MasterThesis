/*********************************************************
 *
 * This source code is part of the Carnegie Mellon Robot
 * Navigation Toolkit (CARMEN)
 *
 * CARMEN Copyright (c) 2002 Michael Montemerlo, Nicholas
 * Roy, and Sebastian Thrun
 *
 * CARMEN is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public 
 * License as published by the Free Software Foundation; 
 * either version 2 of the License, or (at your option)
 * any later version.
 *
 * CARMEN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied 
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more 
 * details.
 *
 * You should have received a copy of the GNU General 
 * Public License along with CARMEN; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, 
 * Suite 330, Boston, MA  02111-1307 USA
 *
 ********************************************************/

/********************************************************
 *
 * The CARMEN Canon module was written by 
 * Mike Montemerlo (mmde+@cs.cmu.edu)
 * Chunks of code were taken from the Canon library that
 * is part of Gphoto2.  http://www.gphoto.com
 *
 ********************************************************/

#ifndef CANON_S40_H
#define CANON_S40_H

#include "usb.h"

/* four different transfer modes, they can be combined as well */
#define    THUMB_TO_PC              0x0001
#define    FULL_TO_PC               0x0002
#define    THUMB_TO_DRIVE           0x0004
#define    FULL_TO_DRIVE            0x0008

#define    FLASH_OFF                0x00
#define    FLASH_ON                 0x01
#define    FLASH_AUTO               0x02

/* open a USB connection the camera */
usb_dev_handle *canon_open_camera(void);

/* initialize USB connection to the camera */
int canon_initialize_camera(usb_dev_handle *camera_handle);

/* identify camera */
int canon_identify_camera(usb_dev_handle *camera_handle);

/* get camera capabilities */
int canon_get_picture_abilities(usb_dev_handle *camera_handle);

/* turn off the LCD display and lock the keys */
int canon_lock_keys(usb_dev_handle *camera_handle);

/* get the power supply status */
int canon_get_power_status(usb_dev_handle *camera_handle);

/* initialize remote camera control mode */
int canon_rcc_init(usb_dev_handle *camera_handle);

/* unknown remote camera control command */
int canon_rcc_unknown(usb_dev_handle *camera_handle, int transfer_mode);

/* turn off flash */
int canon_rcc_set_flash(usb_dev_handle *camera_handle, int flash_mode);

/* get release parameters */
int canon_rcc_get_release_params(usb_dev_handle *camera_handle);

/* take a picture */
int canon_rcc_release_shutter(usb_dev_handle *camera_handle);

/* set the capture transfer mode */
int canon_rcc_set_transfer_mode(usb_dev_handle *camera_handle, 
				int transfer_mode);

/* download the captured thumbnail image */
int canon_rcc_download_thumbnail(usb_dev_handle *camera_handle,
				 unsigned char **thumbnail, 
				 int *thumbnail_length);

/* dowload the captured full image */
int canon_rcc_download_full_image(usb_dev_handle *camera_handle,
				  unsigned char **image,
				  int *image_length);

/* exit remote capture control mode */
int canon_rcc_exit(usb_dev_handle *camera_handle);

/* star the viewfinder */
int canon_rcc_start_viewfinder(usb_dev_handle *camera_handle);

/* select camera output ??? */
int canon_rcc_select_camera_output(usb_dev_handle *camera_handle);

/* download preview image */
int canon_rcc_download_preview(usb_dev_handle *camera_handle,
			       unsigned char **preview,
			       int *preview_length);

/* initialize remote capture control */
int canon_initialize_capture(usb_dev_handle *camera_handle, int transfer_mode,
			     int use_flash);

int canon_initialize_preview(usb_dev_handle *camera_handle);

/* capture image wrapper */
int canon_capture_image(usb_dev_handle *camera_handle, 
			unsigned char **thumbnail, int *thumbnail_length,
			unsigned char **image, int *image_length);

/* stop capturing */
int canon_stop_capture(usb_dev_handle *camera_handle);

/* close USB connection to camera */
void canon_close_camera(usb_dev_handle *camera_handle);

#endif
