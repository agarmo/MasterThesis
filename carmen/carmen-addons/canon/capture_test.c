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
#include "canon_interface.h"

#define GET_THUMB 1

int main(int argc, char **argv)
{
  carmen_canon_image_message *image = NULL;
  int image_over_ipc, image_to_drive, use_flash;
  FILE *fp;
  
  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);

  if(argc < 4)
    carmen_die("Error: not enough arguments.\n"
	       "Usage: %s image-over-ipc image-to-drive use-flash\n", argv[0]);
  image_over_ipc = atoi(argv[1]);
  image_to_drive = atoi(argv[2]);
  use_flash = atoi(argv[3]);

  /* get an image from the canon server */
  if((image = carmen_canon_get_image(GET_THUMB, image_over_ipc, image_to_drive,
				     use_flash ? 1 : 0))== NULL)
    carmen_die("Error: could not get image from server.\n");
  fprintf(stderr, "Received image.\n");

  /* write images to file */
  if(image->image_length > 0) {
    fp = fopen("image.jpg", "w");
    if(fp == NULL)
      carmen_die("Error: could not open file image.jpg for writing.\n");
    fprintf(stderr, "Writing %d bytes to image.jpg.\n", image->image_length);
    fwrite(image->image, image->image_length, 1, fp);
    fclose(fp);
  }
  if(image->thumbnail_length > 0) {
    fp = fopen("thumbnail.jpg", "w");
    if(fp == NULL)
      carmen_die("Error: could not open file thumbnail.jpg for writing.\n");
    fprintf(stderr, "Writing %d bytes to thumbnail.jpg.\n",
	    image->thumbnail_length);
    fwrite(image->thumbnail, image->thumbnail_length, 1, fp);
    fclose(fp);
  }

  /* free image */
  carmen_canon_free_image(&image);
  return 0;
}
