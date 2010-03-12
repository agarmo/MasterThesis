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

#include "usb.h"

#include "canon.h"

#define    USB_TIMEOUT              5000

#define    CAMERA_BULK_READ_EP      0x81
#define    CAMERA_BULK_WRITE_EP     0x02
#define    CAMERA_INT_READ_EP       0x83

#define    CANON_S40_VENDOR         0x04a9
#define    CANON_S40_PRODUCT        0x3056

#define htole32a(a,x) (a)[3]=(unsigned char)((x)>>24), \
                      (a)[2]=(unsigned char)((x)>>16), \
                      (a)[1]=(unsigned char)((x)>>8), \
                      (a)[0]=(unsigned char)(x)

int verbose = 0;
int global_transfer_mode = -1;
int global_image_count = 1;
int global_flash_mode = FLASH_AUTO;

struct usb_device *find_usb_device(int vendor, int product)
{
  struct usb_bus *bus;
  struct usb_device *device;
  
  bus = usb_busses;
  while(bus) {
    device = bus->devices;
    while(device) {
      if(device->descriptor.idVendor == vendor &&
	 device->descriptor.idProduct == product)
	return device;
      device = device->next;
    }
    bus = bus->next;
  }
  return NULL;
}

int camera_control_read(usb_dev_handle *handle, int request, int value,
			int index, char *buffer, int length)
{
  if(verbose)
    fprintf(stderr, "CAMERA CONTROL READ %x\n", length);
  return usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | 
			 USB_ENDPOINT_IN, request, value, index, 
			 buffer, length, USB_TIMEOUT);
}

int camera_control_write(usb_dev_handle *handle, int request, int value,
			 int index, char *buffer, int length)
{
  if(verbose)
    fprintf(stderr, "CAMERA CONTROL WRITE %x\n", length);
  return usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE,
			 request, value, index, buffer, length, USB_TIMEOUT);
}

int camera_bulk_read(usb_dev_handle *handle, char *buffer, int length)
{
  int bytes_read = 0, read_chunk, actually_read;

  if(verbose)
    fprintf(stderr, "CAMERA BULK TOTAL READ %x\n", length);

  bytes_read = 0;
  do {
    if(length - bytes_read > 0x5000)
      read_chunk = 0x5000;
    else if(length - bytes_read > 0x40)
      read_chunk = (length - bytes_read) / 0x40 * 0x40;
    else
      read_chunk = length - bytes_read;
    if(verbose)
      fprintf(stderr, "CAMERA BULK SUB READ %x\n", read_chunk);
    actually_read = usb_bulk_read(handle, CAMERA_BULK_READ_EP, 
				  buffer + bytes_read, 
				  read_chunk, USB_TIMEOUT);
    if(actually_read != read_chunk)
      return -1;
    bytes_read += read_chunk;
  } while(bytes_read < length);
  return length;
}

int camera_bulk_write(usb_dev_handle *handle, char *buffer, int length)
{
  if(verbose)
    fprintf(stderr, "CAMERA BULK WRITE %x\n", length);
  return usb_bulk_write(handle, CAMERA_BULK_WRITE_EP, buffer, length,
			USB_TIMEOUT);
}

int camera_int_read(usb_dev_handle *handle, char *buffer, int length)
{
  int l;
  if(verbose)
    fprintf(stderr, "CAMERA INTERRUPT READ %x\n", length);
  while((l = usb_bulk_read(handle, CAMERA_INT_READ_EP, 
			   buffer, length, 50)) < length);
  return l;
}

void canon_print_command(unsigned char *command, int length, char *name)
{
  int i;

  printf("%s:\n", name);
  for(i = 0; i < length; i++) {
    printf("%02x ", command[i]);
    if(i > 0 && (i + 1) % 16 == 0)
      printf("\n");
  }
  printf("\n");
}

unsigned char *canon_fill_command(int length, int cmd1, unsigned char cmd2, 
				  unsigned char cmd3, unsigned char *payload,
				  int payload_length)
{
  unsigned char *buffer;
  
  buffer = (unsigned char *)calloc(length, 1);
  memset(buffer, 0, length);
  htole32a(buffer, 0x10 + payload_length);
  htole32a(buffer + 0x04, cmd1);
  buffer[0x40] = 2;
  buffer[0x44] = cmd2;
  buffer[0x47] = cmd3;
  htole32a(buffer + 0x48, 0x10 + payload_length);
  htole32a(buffer + 0x4c, 0x12345678);
  if(payload != NULL)
    memcpy(buffer + 0x50, payload, payload_length);
  return buffer;
}

unsigned char *canon_fill_payload(int length, int word1, int word2, int word3, 
				  int word4)
{
  unsigned char *buffer;
  
  buffer = (unsigned char *)calloc(length, 1);
  htole32a(buffer, word1);
  if(length >= 8)
    htole32a(buffer + 4, word2);
  if(length >= 12)
    htole32a(buffer + 8, word3);
  if(length >= 16)
    htole32a(buffer + 12, word4);
  return buffer;
}

int canon_query_response(usb_dev_handle *camera_handle, int cmd1,
			 unsigned char cmd2, unsigned char cmd3,
			 unsigned char value, unsigned char *payload, 
			 int payload_length, char *response, 
			 int response_length)
{
  unsigned char *command;
  int l, total_length = 0x50 + payload_length;

  /* create the command data */
  command = canon_fill_command(total_length, cmd1, cmd2, cmd3,
			       payload, payload_length);
  /* send the command */
  l = camera_control_write(camera_handle, 0x04, value, 0x00, command,
			   total_length);
  if(l != total_length) {
    free(command);
    return -1;
  }

  /* read the response */
  l = camera_bulk_read(camera_handle, response, response_length);
  if(l != response_length) {
    free(command);
    return -1;
  }

  free(command);
  return 0;
}
			 
usb_dev_handle *canon_open_camera(void)
{
  struct usb_device *camera;
  usb_dev_handle *camera_handle;
  
  /* intialize USB library */
  usb_init();
  usb_find_busses();
  usb_find_devices();

  /* search the USB bus for the appropriate camera */
  fprintf(stderr, "Searching for Canon Powershot S40 camera... ");
  camera = find_usb_device(CANON_S40_VENDOR, CANON_S40_PRODUCT);
  if(camera == NULL) {
    fprintf(stderr, "not found.\n");
    return NULL;
  }
  else
    fprintf(stderr, "found.\n");

  /* claim the camera on the USB bus */
  fprintf(stderr, "Opening connection to camera... ");
  camera_handle = usb_open(camera);
  if(camera_handle == NULL) {
    fprintf(stderr, "failed.\n");
    return NULL;
  }
  else
    fprintf(stderr, "done.\n");
  if(usb_claim_interface(camera_handle, 0) < 0) {
    fprintf(stderr, "Error: could not claim USB interface to camera.\n");
    return NULL;
  }
  if(usb_set_configuration(camera_handle, 1) < 0) {
    fprintf(stderr, "Error: could not set configuration.\n");
    return NULL;
  }
  if(usb_set_altinterface(camera_handle, 0) < 0) {
    fprintf(stderr, "Error: could not set altinterface.\n");
    return NULL;
  }
  return camera_handle;
}

void canon_close_camera(usb_dev_handle *camera_handle)
{
  /* close connection to the camera */
  fprintf(stderr, "Closing connection to camera... ");
  usb_release_interface(camera_handle, 0);
  usb_close(camera_handle);
  fprintf(stderr, "done.\n");
}

int canon_identify_camera(usb_dev_handle *camera_handle)
{
  unsigned char response[0x9c];

  if(canon_query_response(camera_handle, 0x201, 0x01, 0x12, 0x10, 
			  NULL, 0, response, 0x9c) < 0)
    return -1;
  return 0;
}

int canon_get_picture_abilities(usb_dev_handle *camera_handle)
{
  unsigned char response[0x384];

  if(canon_query_response(camera_handle, 0x201, 0x1f, 0x12, 0x10, 
			  NULL, 0, response, 0x384) < 0) {
    fprintf(stderr, "Error: get picture abilities command failed.\n");
    return -1;
  }
  return 0;
}

int canon_lock_keys(usb_dev_handle *camera_handle)
{
  unsigned char response[0x54];

  if(canon_query_response(camera_handle, 0x201, 0x20, 0x12, 0x10, 
			  NULL, 0, response, 0x54) < 0) {
    fprintf(stderr, "Error: lock keys command failed.\n");
    return -1;
  }
  return 0;
}

int canon_get_power_status(usb_dev_handle *camera_handle)
{
  unsigned char response[0x58];

  if(canon_query_response(camera_handle, 0x201, 0x0a, 0x12, 0x10, 
			  NULL, 0, response, 0x58) < 0) {
    fprintf(stderr, "Error: get power status command failed.\n");
    return -1;
  }
  fprintf(stderr, "Power status is %s  Source = %s\n", 
	  (response[0x54] == 0x06) ? "GOOD" : "BAD",
	  ((response[0x57] & 0x20) == 0) ? "AC-ADAPTER" : "BATTERY");
  return 0;
}

int canon_initialize_camera(usb_dev_handle *camera_handle)
{
  unsigned char msg[0x58], buffer[0x44];
  unsigned char camstat;
  int i, err = 0;

  memset(msg, 0, sizeof(msg));
  memset(buffer, 0, sizeof(buffer));
  i = camera_control_read(camera_handle, 0x0c, 0x55, 0x00, msg, 0x01);
  camstat = msg[0];
  if(i != 1 || (camstat != 'A' && camstat != 'C'))
    return -1;
  i = camera_control_read(camera_handle, 0x04, 0x01, 0x00, msg, 0x58);
  if(i != 0x58)
    return -1;

  if(camstat == 'A') {
    i = camera_control_read(camera_handle, 0x04, 0x04, 0x00, msg, 0x50);
    if(i != 0x50)
      return -1;
  }
  else {
    msg[0] = 0x10;
    memmove(msg + 0x40, msg + 0x48, 0x10);
    i = camera_control_write(camera_handle, 0x04, 0x11, 0x00, msg, 0x50);
    if(i != 0x50)
      return -1;
    i = camera_bulk_read(camera_handle, buffer, 0x40);
    if((i >= 4) && (buffer[i - 4] == 0x54) && (buffer[i - 3] == 0x78) &&
       (buffer[i - 2] == 0x00) && (buffer[i - 1] == 0x00))
      return 0;
    if(i != 0x40)
      return -1;
    i = camera_bulk_read(camera_handle, buffer, 0x04);
    if(i != 4)
      return -1;
    /* read 0x10 bytes from the interrupt endpoint - this is important */
    i = camera_int_read(camera_handle, buffer, 0x10);
    if(i != 0x10)
      return -1;
  }
  
  fprintf(stderr, "Identifying camera... ");
  for(i = 0; i < 4; i++) {
    err = canon_identify_camera(camera_handle);
    if(err == 0)
      break;
  }
  if(err == 0)
    fprintf(stderr, "done.\n");
  else
    fprintf(stderr, "failed.\n");

  if(canon_get_picture_abilities(camera_handle) < 0)
    fprintf(stderr, "get camera abilites failed.\n");
  if(canon_lock_keys(camera_handle) < 0)
    fprintf(stderr, "lock keys failed.\n");
  if(canon_get_power_status(camera_handle) < 0)
    fprintf(stderr, "get power status failed.\n");
  sleep(2);
  return 0;
}

int canon_rcc_init(usb_dev_handle *camera_handle)
{
  unsigned char *payload, response[0x5c];

  payload = canon_fill_payload(0x08, 0, 0, 0, 0);
  if(canon_query_response(camera_handle, 0x201, 0x13, 0x12, 0x10, 
			  payload, 0x08, response, 0x5c) < 0) {
    fprintf(stderr, "Error: rcc init command failed.\n");
    free(payload);
    return -1;
  }
  free(payload);
  return 0;
}

int canon_rcc_unknown(usb_dev_handle *camera_handle, int transfer_mode)
{
  unsigned char *payload, response[0x5c];

  payload = canon_fill_payload(0x0c, 9, 4, (1 << 24) | transfer_mode, 0);
  if(canon_query_response(camera_handle, 0x201, 0x13, 0x12, 0x10, 
			  payload, 0x0c, response, 0x5c) < 0) {
    fprintf(stderr, "Error: rcc unknown command failed.\n");
    free(payload);
    return -1;
  }
  free(payload);
  return 0;
}

int canon_rcc_get_release_params(usb_dev_handle *camera_handle)
{
  unsigned char *payload, response[0x8c];

  payload = canon_fill_payload(0x08, 0x0a, 0, 0, 0);
  if(canon_query_response(camera_handle, 0x201, 0x13, 0x12, 0x10, 
			  payload, 0x08, response, 0x8c) < 0) {
    fprintf(stderr, "Error: rcc get release command failed.\n");
    free(payload);
    return -1;
  }
  free(payload);
  return 0;
}

int canon_rcc_set_transfer_mode(usb_dev_handle *camera_handle, 
				int transfer_mode)
{
  unsigned char *payload, response[0x5c];

  payload = canon_fill_payload(0x0c, 9, 4, transfer_mode, 0);
  if(canon_query_response(camera_handle, 0x201, 0x13, 0x12, 0x10, 
			  payload, 0x0c, response, 0x5c) < 0) {
    fprintf(stderr, "Error: rcc set transfer mode command failed.\n");
    free(payload);
    return -1;
  }
  free(payload);
  global_transfer_mode = transfer_mode;
  return 0;
}

int canon_rcc_release_shutter(usb_dev_handle *camera_handle)
{
  unsigned char *payload, response[0x5c];
  int l;

  payload = canon_fill_payload(0x08, 4, 0, 0, 0);
  if(canon_query_response(camera_handle, 0x201, 0x13, 0x12, 0x10, 
			  payload, 0x08, response, 0x5c) < 0) {
    fprintf(stderr, "Error: rcc release shutter command failed.\n");
    free(payload);
    return -1;
  }
  free(payload);

  l = camera_int_read(camera_handle, response, 0x10);
  if(global_transfer_mode & THUMB_TO_PC)
    l = camera_int_read(camera_handle, response, 0x17);
  if(global_transfer_mode & FULL_TO_PC)
    l = camera_int_read(camera_handle, response, 0x17);
  l = camera_int_read(camera_handle, response, 0x10);
  return 0;
}

int canon_rcc_download_thumbnail(usb_dev_handle *camera_handle,
				 unsigned char **thumbnail, 
				 int *thumbnail_length)
{
  unsigned char *payload, response[0x40];
  int l;

  payload = canon_fill_payload(0x10, 0, 0x50 << 8, 1, global_image_count);
  if(canon_query_response(camera_handle, 0x202, 0x17, 0x12, 0x10, 
			  payload, 0x10, response, 0x40) < 0) {
    fprintf(stderr, "Error: rcc download thumbnail command failed.\n");
    free(payload);
    return -1;
  }
  free(payload);

  *thumbnail_length = (response[6] | (response[7] << 8) |
		       (response[8] << 16) | (response[9] << 24));
  *thumbnail = (unsigned char *)calloc(*thumbnail_length, 1);
  l = camera_bulk_read(camera_handle, *thumbnail, *thumbnail_length);
  if(l != *thumbnail_length) {
    free(*thumbnail);
    return -1;
  }
  return 0;
}

int canon_rcc_download_full_image(usb_dev_handle *camera_handle,
				  unsigned char **image,
				  int *image_length)
{
  unsigned char *payload, response[0x40];
  int l;

  payload = canon_fill_payload(0x10, 0, 0x50 << 8, 2, global_image_count);
  if(canon_query_response(camera_handle, 0x202, 0x17, 0x12, 0x10, 
			  payload, 0x10, response, 0x40) < 0) {
    fprintf(stderr, "Error: rcc download full image command failed.\n");
    free(payload);
    return -1;
  }
  free(payload);

  *image_length = (response[6] | (response[7] << 8) |
		   (response[8] << 16) | (response[9] << 24));
  *image = (unsigned char *)calloc(*image_length, 1);
  l = camera_bulk_read(camera_handle, *image, *image_length);
  if(l != *image_length) {
    free(*image);
    return -1;
  }
  return 0;
}

int canon_rcc_set_flash(usb_dev_handle *camera_handle, int flash_mode)
{
  unsigned char payload[0x38] = 
  {0x07, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 
   0x20, 0x03, 0x01, 0x00, 0x64, 0x00, 0x00, 0x01,
   0x00, 0x00, 0x00, 0x00, 0x03, 0x01, 0x01, 0x30,
   0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0x00, 0x7f,
   0xff, 0xff, 0x40, 0x00, 0x28, 0x00, 0x70, 0x00,
   0x18, 0x18, 0xff, 0xff, 0x20, 0x00, 0x38, 0x00, 
   0xe3, 0x00, 0xaa, 0x02, 0xe3, 0x00, 0x20, 0x00};
  unsigned char response[0x5c];

  payload[14] = (unsigned char)flash_mode;
  if(canon_query_response(camera_handle, 0x201, 0x13, 0x12, 0x10, 
			  payload, 0x38, response, 0x5c) < 0) {
    fprintf(stderr, "Error: rcc set flash failed.\n");
    return -1;
  }
  global_flash_mode = flash_mode;
  return 0;
}

int canon_rcc_start_viewfinder(usb_dev_handle *camera_handle)
{
  unsigned char *payload, response[0x5c];

  payload = canon_fill_payload(0x08, 2, 0, 0, 0);
  if(canon_query_response(camera_handle, 0x201, 0x13, 0x12, 0x10, 
			  payload, 0x08, response, 0x5c) < 0) {
    fprintf(stderr, "Error: rcc start viewfinder failed.\n");
    free(payload);
    return -1;
  }
  free(payload);
  return 0;
}

int canon_rcc_select_camera_output(usb_dev_handle *camera_handle)
{
  unsigned char payload[9] = 
    {0x14, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03};
  unsigned char response[0x5c];

  if(canon_query_response(camera_handle, 0x201, 0x13, 0x12, 0x10, 
			  payload, 0x09, response, 0x5c) < 0) {
    fprintf(stderr, "Error: rcc select camera output failed.\n");
    return -1;
  }
  return 0;
}

int canon_rcc_download_preview(usb_dev_handle *camera_handle,
			       unsigned char **preview,
			       int *preview_length)
{
  unsigned char *payload, response[0x40];
  int l;
  
  payload = canon_fill_payload(0x08, 0, 0x50 << 8, 0, 0);
  if(canon_query_response(camera_handle, 0x202, 0x18, 0x12, 0x10, 
			  payload, 0x10, response, 0x40) < 0) {
    fprintf(stderr, "Error: rcc download preview command failed.\n");
    free(payload);
    return -1;
  }
  free(payload);

  *preview_length = (response[6] | (response[7] << 8) |
		     (response[8] << 16) | (response[9] << 24));
  *preview = (unsigned char *)calloc(*preview_length, 1);
  l = camera_bulk_read(camera_handle, *preview, *preview_length);
  if(l != *preview_length) {
    free(*preview);
    return -1;
  }
  return 0;
}

int canon_rcc_exit(usb_dev_handle *camera_handle)
{
  unsigned char *payload, response[0x5c];

  payload = canon_fill_payload(0x08, 1, 0, 0, 0);
  if(canon_query_response(camera_handle, 0x201, 0x13, 0x12, 0x10, 
			  payload, 0x08, response, 0x5c) < 0) {
    fprintf(stderr, "Error: rcc set transfer mode command failed.\n");
    free(payload);
    return -1;
  }
  free(payload);
  return 0;
}

int canon_initialize_preview(usb_dev_handle *camera_handle)
{
  if(canon_rcc_start_viewfinder(camera_handle) < 0) {
    fprintf(stderr, "rcc start viewfinder failed.\n");
    return -1;
  }
  if(canon_rcc_select_camera_output(camera_handle) < 0) {
    fprintf(stderr, "rcc select camera output failed.\n");
    return -1;
  }
  return 0;
}

int canon_initialize_capture(usb_dev_handle *camera_handle, int transfer_mode,
			     int flash_mode)
{
  if(canon_rcc_init(camera_handle) < 0) {
    fprintf(stderr, "rcc init failed.\n");
    return -1;
  }
  sleep(1);
  if(canon_rcc_unknown(camera_handle, transfer_mode) < 0) {
    fprintf(stderr, "rcc unknown failed.\n");
    return -1;
  }
  if(canon_rcc_set_transfer_mode(camera_handle, transfer_mode) < 0) {
    fprintf(stderr, "rcc set transfer mode failed.\n");
    return -1;
  }
  if(canon_rcc_set_flash(camera_handle, flash_mode) < 0) {
    fprintf(stderr, "rcc turnoff flash failed.\n");
    return -1;
  }
  return 0;
}

int canon_capture_image(usb_dev_handle *camera_handle, 
			unsigned char **thumbnail, int *thumbnail_length,
			unsigned char **image, int *image_length)
{
  if(canon_rcc_get_release_params(camera_handle) < 0) {
    fprintf(stderr, "rcc get release params failed.\n");
    return -1;
  }
  if(canon_rcc_get_release_params(camera_handle) < 0) {
    fprintf(stderr, "rcc get release params failed.\n");
    return -1;
  }
  if(canon_rcc_set_transfer_mode(camera_handle, global_transfer_mode) < 0) {
    fprintf(stderr, "rcc set transfer mode failed.\n");
    return -1;
  }
  if(canon_rcc_release_shutter(camera_handle) < 0) {
    fprintf(stderr, "rcc release shutter failed.\n");
    return -1;
  }
  if(global_transfer_mode & THUMB_TO_PC) {
    if(canon_rcc_download_thumbnail(camera_handle, thumbnail, 
				    thumbnail_length) < 0) {
      fprintf(stderr, "download thumbnail failed.\n");
      return -1;
    }
  }
  else {
    *thumbnail = NULL;
    *thumbnail_length = 0;
  }
  if(global_transfer_mode & FULL_TO_PC) {
    if(canon_rcc_download_full_image(camera_handle, image, image_length) < 0) {
      fprintf(stderr, "download image failed.\n");
      return -1;
    }
  }
  else {
    *image = NULL;
    *image_length = 0;
  }
  global_image_count++;
  return 0;
}

int canon_stop_capture(usb_dev_handle *camera_handle)
{
  if(canon_rcc_exit(camera_handle) < 0) {
    fprintf(stderr, "rcc exit failed.\n");
    return -1;
  }
  return 0;
}
