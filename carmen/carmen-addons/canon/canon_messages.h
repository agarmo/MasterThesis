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

#ifndef CANON_MESSAGES_H
#define CANON_MESSAGES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int thumbnail_over_ipc;
  int image_over_ipc;
  int image_to_drive;
  int flash_mode;
  double timestamp;
  char host[10];
} carmen_canon_image_request;

#define      CARMEN_CANON_IMAGE_REQUEST_NAME   "carmen_canon_image_request"
#define      CARMEN_CANON_IMAGE_REQUEST_FMT    "{int,int,int,int,double,[char:10]}"

typedef struct {
  int thumbnail_length;
  char *thumbnail;
  int image_length;
  char *image;
  double timestamp;
  char host[10];
} carmen_canon_image_message;

#define      CARMEN_CANON_IMAGE_NAME       "carmen_canon_image"
#define      CARMEN_CANON_IMAGE_FMT        "{int,<char:1>,int,<char:3>,double,[char:10]}"

#define      CARMEN_CANON_PREVIEW_START_NAME   "canon_preview_start"
#define      CARMEN_CANON_PREVIEW_START_FMT    ""

#define      CARMEN_CANON_PREVIEW_STOP_NAME    "canon_preview_stop"
#define      CARMEN_CANON_PREVIEW_STOP_FMT     ""

#define      CARMEN_CANON_PREVIEW_SNAP_NAME    "canon_preview_stop"
#define      CARMEN_CANON_PREVIEW_SNAP_FMT     ""

typedef struct {
  int preview_length;
  char *preview;
  double timestamp;
  char host[10];
} carmen_canon_preview_message;

#define      CARMEN_CANON_PREVIEW_NAME     "carmen_canon_preview"
#define      CARMEN_CANON_PREVIEW_FMT      "{int,<char:1>,double,[char:10]}"

#define      CARMEN_CANON_ALIVE_NAME       "carmen_canon_alive"
#define      CARMEN_CANON_ALIVE_FMT        ""

#ifdef __cplusplus
}
#endif

#endif
