#include <stdio.h>
#include <stdlib.h>
#include "canon.h"

usb_dev_handle *camera_handle;

void die(char *str)
{
  fprintf(stderr, str);
  exit(1);
}

int main(int argc __attribute__ ((unused)), char **argv)
{
  char *thumbnail, *image;
  int thumbnail_length, image_length;
  FILE *fp;

  /* initialize the digital camera */
  camera_handle = canon_open_camera();
  if(camera_handle == NULL)
    die("Erorr: could not open connection to camera.\n");

  if(canon_initialize_camera(camera_handle) < 0)
    die("Erorr: could not open initialize camera.\n");
  if(canon_initialize_capture(camera_handle, FULL_TO_PC | THUMB_TO_PC,
			      FLASH_AUTO) < 0)
    die("Error: could not start image capture.\n");
  
  fprintf(stderr, "Starting capture... ");
  if(canon_capture_image(camera_handle,
			 (unsigned char **)&thumbnail, &thumbnail_length,
			 (unsigned char **)&image, &image_length) < 0)
    fprintf(stderr, "error.\n");
  else {
    fprintf(stderr, "done.\n");
    fp = fopen("test.jpg", "w");
    if(fp == NULL)
      die("Error: could not open file test.jpg for writing.\n");
    fwrite(image, image_length, 1, fp);
    fclose(fp);
  }
  canon_stop_capture(camera_handle);
  canon_close_camera(camera_handle);
  return 0;
}
