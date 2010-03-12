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
#include "canon.h"
#include "canon_messages.h"
#include "canon_interface.h"

static unsigned int timeout = 15000;

static carmen_canon_preview_message *preview_message_pointer_external = NULL;
static carmen_handler_t preview_message_handler_external = NULL;
static carmen_handler_t alive_message_handler_external = NULL;

carmen_canon_image_message *carmen_canon_get_image(int thumbnail_over_ipc,
						   int image_over_ipc,
						   int image_to_drive,
						   int flash_mode)
{
  IPC_RETURN_TYPE err;
  carmen_canon_image_message *response;
  carmen_canon_image_request query;
  
  if(!thumbnail_over_ipc && !image_over_ipc && !image_to_drive) {
    carmen_warn("Error: need to set at least one location for the image.\n");
    return NULL;
  }
  query.timestamp = carmen_get_time_ms();
  strcpy(query.host, carmen_get_tenchar_host_name());
  query.thumbnail_over_ipc = thumbnail_over_ipc;
  query.image_over_ipc = image_over_ipc;
  query.image_to_drive = image_to_drive;
  query.flash_mode = flash_mode;
  err = IPC_queryResponseData(CARMEN_CANON_IMAGE_REQUEST_NAME, &query,
                              (void **)&response, timeout);
  carmen_test_ipc(err, "Could not query image",
		  CARMEN_CANON_IMAGE_REQUEST_NAME);
  if(IPC_Error || err == IPC_Timeout)
    return NULL;
  return response;
}

void carmen_canon_free_image(carmen_canon_image_message **message)
{
  if(message != NULL) 
    if(*message != NULL) {
      if((*message)->thumbnail != NULL)
	free((*message)->thumbnail);
      if((*message)->image != NULL)
	free((*message)->image);
      free(*message);
      *message = NULL;
    }
}

static void
preview_interface_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
			  void *clientData __attribute__ ((unused)))
{
  IPC_RETURN_TYPE err = IPC_OK;
  FORMATTER_PTR formatter;
  
  formatter = IPC_msgInstanceFormatter(msgRef);
  if(preview_message_pointer_external) {
    if(preview_message_pointer_external->preview != NULL)
      free(preview_message_pointer_external->preview);
    
    err = IPC_unmarshallData(formatter, callData, 
                             preview_message_pointer_external,
                             sizeof(carmen_canon_preview_message));
  }
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall", 
                         IPC_msgInstanceName(msgRef));
  if(preview_message_handler_external)
    preview_message_handler_external(preview_message_pointer_external);
}

void
carmen_canon_subscribe_preview_message(carmen_canon_preview_message *preview,
				       carmen_handler_t handler,
				       carmen_subscribe_t subscribe_how)
{
  IPC_RETURN_TYPE err = IPC_OK;
  
  if(subscribe_how == CARMEN_UNSUBSCRIBE) {
    IPC_unsubscribe(CARMEN_CANON_PREVIEW_NAME, preview_interface_handler);
    return;
  }
  if(preview) {
    preview_message_pointer_external = preview;
    memset(preview_message_pointer_external, 0, 
           sizeof(carmen_canon_preview_message));
  }
  else if(preview_message_pointer_external == NULL) {
    preview_message_pointer_external = (carmen_canon_preview_message *)
      calloc(1, sizeof(carmen_canon_preview_message));
    carmen_test_alloc(preview_message_pointer_external);
  }
  
  preview_message_handler_external = handler;
  err = IPC_subscribe(CARMEN_CANON_PREVIEW_NAME, 
                      preview_interface_handler, NULL);
  if (subscribe_how == CARMEN_SUBSCRIBE_LATEST)
    IPC_setMsgQueueLength(CARMEN_CANON_PREVIEW_NAME, 1);
  else
    IPC_setMsgQueueLength(CARMEN_CANON_PREVIEW_NAME, 100);
  carmen_test_ipc(err, "Could not subscribe", CARMEN_CANON_PREVIEW_NAME);
}

int carmen_canon_start_preview_command(void)
{
  IPC_RETURN_TYPE err;
  static int first = 1;

  if(first) {
    err = IPC_defineMsg(CARMEN_CANON_PREVIEW_START_NAME, IPC_VARIABLE_LENGTH, 
                        CARMEN_CANON_PREVIEW_START_FMT);
    carmen_test_ipc_exit(err, "Could not define message", 
                         CARMEN_CANON_PREVIEW_START_NAME);
    first = 0;
  }
  err = IPC_publishData(CARMEN_CANON_PREVIEW_START_NAME, NULL);
  carmen_test_ipc_return_int(err, "Could not publish", 
			     CARMEN_CANON_PREVIEW_START_NAME);
  return 0;
}

int carmen_canon_stop_preview_command(void)
{
  IPC_RETURN_TYPE err;
  static int first = 1;

  if(first) {
    err = IPC_defineMsg(CARMEN_CANON_PREVIEW_STOP_NAME, IPC_VARIABLE_LENGTH, 
                        CARMEN_CANON_PREVIEW_STOP_FMT);
    carmen_test_ipc_exit(err, "Could not define message", 
                         CARMEN_CANON_PREVIEW_STOP_NAME);
    first = 0;
  }
  err = IPC_publishData(CARMEN_CANON_PREVIEW_STOP_NAME, NULL);
  carmen_test_ipc_return_int(err, "Could not publish", 
			     CARMEN_CANON_PREVIEW_STOP_NAME);
  return 0;
}

int carmen_canon_snap_preview_command(void)
{
  IPC_RETURN_TYPE err;
  static int first = 1;

  if(first) {
    err = IPC_defineMsg(CARMEN_CANON_PREVIEW_SNAP_NAME, IPC_VARIABLE_LENGTH, 
                        CARMEN_CANON_PREVIEW_SNAP_FMT);
    carmen_test_ipc_exit(err, "Could not define message", 
                         CARMEN_CANON_PREVIEW_SNAP_NAME);
    first = 0;
  }
  err = IPC_publishData(CARMEN_CANON_PREVIEW_SNAP_NAME, NULL);
  carmen_test_ipc_return_int(err, "Could not publish", 
			     CARMEN_CANON_PREVIEW_SNAP_NAME);
  return 0;
}

static void 
alive_interface_handler(MSG_INSTANCE msgRef __attribute__ ((unused)),
                         BYTE_ARRAY callData __attribute__ ((unused)),
                         void *clientData __attribute__ ((unused)))
{
  if(alive_message_handler_external)
    alive_message_handler_external(NULL);
}

void
carmen_canon_subscribe_alive_message(carmen_handler_t handler,
                                     carmen_subscribe_t subscribe_how)
{
  IPC_RETURN_TYPE err = IPC_OK;
  
  err = IPC_defineMsg(CARMEN_CANON_ALIVE_NAME, IPC_VARIABLE_LENGTH, 
                      CARMEN_CANON_ALIVE_FMT);
  carmen_test_ipc_exit(err, "Could not define message", 
                       CARMEN_CANON_ALIVE_NAME);
  if(subscribe_how == CARMEN_UNSUBSCRIBE) {
    IPC_unsubscribe(CARMEN_CANON_ALIVE_NAME, 
                    alive_interface_handler);
    return;
  }
  
  alive_message_handler_external = handler;
  err = IPC_subscribe(CARMEN_CANON_ALIVE_NAME, 
                      alive_interface_handler, NULL);
  if (subscribe_how == CARMEN_SUBSCRIBE_LATEST)
    IPC_setMsgQueueLength(CARMEN_CANON_ALIVE_NAME, 1);
  else
    IPC_setMsgQueueLength(CARMEN_CANON_ALIVE_NAME, 100);
  carmen_test_ipc(err, "Could not subscribe", CARMEN_CANON_ALIVE_NAME);
}
