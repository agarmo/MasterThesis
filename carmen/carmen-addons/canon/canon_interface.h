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

#ifndef CANON_INTERFACE_H
#define CANON_INTERFACE_H

#include "canon_messages.h"

/* gets an image from the server.  Parameters specify where the image should
   be delivered */

carmen_canon_image_message *carmen_canon_get_image(int thumbnail_over_ipc,
						   int image_over_ipc,
						   int image_to_disk,
						   int flash_mode);

/* frees a canon image */

void carmen_canon_free_image(carmen_canon_image_message **message);

/* subscribe to the preview message */

void
carmen_canon_subscribe_preview_message(carmen_canon_preview_message *preview,
				       carmen_handler_t handler,
				       carmen_subscribe_t subscribe_how);

/* start publishing previews */

int carmen_canon_start_preview_command(void);

/* stop publishing previews */

int carmen_canon_stop_preview_command(void);

/* snap a single image from the previewer */

int carmen_canon_snap_preview_command(void);

/* subscribe to the heartbeat message */

void
carmen_canon_subscribe_alive_message(carmen_handler_t handler,
                                     carmen_subscribe_t subscribe_how);

#endif
