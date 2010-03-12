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

#include <carmen/carmen.h>
#include "canon_messages.h"
#include "canon.h"

extern usb_dev_handle *camera_handle;
int publishing_preview = 0;

void publish_preview(void *clientdata __attribute__ ((unused)), 
		     unsigned long t1 __attribute__ ((unused)), 
		     unsigned long t2 __attribute__ ((unused)))
{
  carmen_canon_preview_message preview;
  IPC_RETURN_TYPE err;

  if(canon_rcc_download_preview(camera_handle,
				(unsigned char **)&preview.preview, 
				&preview.preview_length) < 0) {
    fprintf(stderr, "Error: could not download a preview.\n");
    return;
  }
  err = IPC_publishData(CARMEN_CANON_PREVIEW_NAME, &preview);
  carmen_test_ipc_exit(err, "Could not publish", 
		       CARMEN_CANON_PREVIEW_NAME);
  free(preview.preview);
}

void start_preview(MSG_INSTANCE msgRef __attribute__ ((unused)),
		   BYTE_ARRAY callData __attribute__ ((unused)),
		   void *clientData __attribute__ ((unused)))
{
  if(!publishing_preview) {
    IPC_addTimer(60, TRIGGER_FOREVER, publish_preview, NULL);
    publishing_preview = 1;
  }
}

void stop_preview(MSG_INSTANCE msgRef __attribute__ ((unused)),
		  BYTE_ARRAY callData __attribute__ ((unused)),
		  void *clientData __attribute__ ((unused)))
{
  if(publishing_preview) {
    IPC_removeTimer(publish_preview);
    publishing_preview = 0;
  }
}

void snap_preview(MSG_INSTANCE msgRef __attribute__ ((unused)),
		  BYTE_ARRAY callData __attribute__ ((unused)),
		  void *clientData __attribute__ ((unused)))
{
  carmen_canon_preview_message preview;
  IPC_RETURN_TYPE err;

  if(canon_rcc_download_preview(camera_handle,
				(unsigned char **)&preview.preview, 
				&preview.preview_length) < 0) {
    fprintf(stderr, "Error: could not download a preview.\n");
    return;
  }
  err = IPC_publishData(CARMEN_CANON_PREVIEW_NAME, &preview);
  carmen_test_ipc_exit(err, "Could not publish", 
		       CARMEN_CANON_PREVIEW_NAME);
  free(preview.preview);
}

void canon_image_query(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
		       void *clientData __attribute__ ((unused)))
{
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;
  carmen_canon_image_message response;
  carmen_canon_image_request query;
  int transfer_mode;

  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &query, 
                           sizeof(carmen_canon_image_request));
  IPC_freeByteArray(callData);
  carmen_test_ipc(err, "Could not unmarshall", IPC_msgInstanceName(msgRef));  

  transfer_mode = 0;
  if(query.thumbnail_over_ipc)
    transfer_mode |= THUMB_TO_PC;
  if(query.image_over_ipc)
    transfer_mode |= FULL_TO_PC;
  if(query.image_to_drive)
    transfer_mode |= FULL_TO_DRIVE;
  
  if(canon_rcc_set_flash(camera_handle, query.flash_mode) < 0)
    carmen_warn("Warning: Could not set flash mode.\n");
  if(canon_rcc_set_transfer_mode(camera_handle, transfer_mode) < 0)
    carmen_warn("Warning: Could not set transfer mode.\n");
  
  fprintf(stderr, "Starting capture... ");
  if(canon_capture_image(camera_handle,
			 (unsigned char **)&response.thumbnail, 
			 &response.thumbnail_length,
			 (unsigned char **)&response.image, 
			 &response.image_length) < 0)
    fprintf(stderr, "error.\n");
  else
    fprintf(stderr, "done.\n");
  response.timestamp = carmen_get_time_ms();
  strcpy(response.host, carmen_get_tenchar_host_name());
  
  err = IPC_respondData(msgRef, CARMEN_CANON_IMAGE_NAME, &response);
  carmen_test_ipc(err, "Could not respond", CARMEN_CANON_IMAGE_NAME);

  if(response.image != NULL)
    free(response.image);
  if(response.thumbnail != NULL)
    free(response.thumbnail);
}

void initialize_ipc_messages(void)
{
  IPC_RETURN_TYPE err;
  
  /* register image request message */
  err = IPC_defineMsg(CARMEN_CANON_IMAGE_REQUEST_NAME, IPC_VARIABLE_LENGTH, 
                      CARMEN_CANON_IMAGE_REQUEST_FMT);
  carmen_test_ipc_exit(err, "Could not define", 
		       CARMEN_CANON_IMAGE_REQUEST_NAME);
  
  /* register image message */
  err = IPC_defineMsg(CARMEN_CANON_IMAGE_NAME, IPC_VARIABLE_LENGTH, 
                      CARMEN_CANON_IMAGE_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_CANON_IMAGE_NAME);

  /* register preview message */
  err = IPC_defineMsg(CARMEN_CANON_PREVIEW_NAME, IPC_VARIABLE_LENGTH, 
                      CARMEN_CANON_PREVIEW_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_CANON_PREVIEW_NAME);

  /* register preview message */
  err = IPC_defineMsg(CARMEN_CANON_PREVIEW_START_NAME, IPC_VARIABLE_LENGTH, 
                      CARMEN_CANON_PREVIEW_START_FMT);
  carmen_test_ipc_exit(err, "Could not define", 
		       CARMEN_CANON_PREVIEW_START_NAME);

  /* register preview message */
  err = IPC_defineMsg(CARMEN_CANON_PREVIEW_STOP_NAME, IPC_VARIABLE_LENGTH, 
                      CARMEN_CANON_PREVIEW_STOP_FMT);
  carmen_test_ipc_exit(err, "Could not define", 
		       CARMEN_CANON_PREVIEW_STOP_NAME);

  /* register preview snap message */
  err = IPC_defineMsg(CARMEN_CANON_PREVIEW_SNAP_NAME, IPC_VARIABLE_LENGTH, 
                      CARMEN_CANON_PREVIEW_SNAP_FMT);
  carmen_test_ipc_exit(err, "Could not define", 
		       CARMEN_CANON_PREVIEW_SNAP_NAME);

  /* heartbeat message */
  err = IPC_defineMsg(CARMEN_CANON_ALIVE_NAME, IPC_VARIABLE_LENGTH, 
                      CARMEN_CANON_ALIVE_FMT);
  carmen_test_ipc_exit(err, "Could not define", 
		       CARMEN_CANON_ALIVE_NAME);

  /* subscribe to image requests */
  err = IPC_subscribe(CARMEN_CANON_IMAGE_REQUEST_NAME, 
		      canon_image_query, NULL);
  carmen_test_ipc_exit(err, "Could not subscribe to", 
                       CARMEN_CANON_IMAGE_REQUEST_NAME);
  IPC_setMsgQueueLength(CARMEN_CANON_IMAGE_REQUEST_NAME, 100);

  /* subscribe to preview starts and stops */
  err = IPC_subscribe(CARMEN_CANON_PREVIEW_START_NAME,
		      start_preview, NULL);
  carmen_test_ipc_exit(err, "Could not subscribe to", 
                       CARMEN_CANON_PREVIEW_START_NAME);
  IPC_setMsgQueueLength(CARMEN_CANON_PREVIEW_START_NAME, 100);

  err = IPC_subscribe(CARMEN_CANON_PREVIEW_STOP_NAME,
		      stop_preview, NULL);
  carmen_test_ipc_exit(err, "Could not subscribe to", 
                       CARMEN_CANON_PREVIEW_STOP_NAME);
  IPC_setMsgQueueLength(CARMEN_CANON_PREVIEW_STOP_NAME, 100);

  err = IPC_subscribe(CARMEN_CANON_PREVIEW_SNAP_NAME,
		      snap_preview, NULL);
  carmen_test_ipc_exit(err, "Could not subscribe to", 
                       CARMEN_CANON_PREVIEW_SNAP_NAME);
  IPC_setMsgQueueLength(CARMEN_CANON_PREVIEW_SNAP_NAME, 100);
}

