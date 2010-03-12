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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#define MIN(a,b) (((a)<(b))?(a):(b))

#define      USE_CANON          0
#define      USE_RIO            1


#define      MAX_URBS           10000

#define      OUT                1
#define      IN                 2

#define      CONTROL            1
#define      BULK               2
#define      INT                3
#define      UNKNOWN            4
#define      IGNORED            5

typedef struct {
  int num_lines, max_lines;
  char **lines;
} urb_text_t, *urb_text_p;

typedef struct {
  urb_text_t in, out;

  int active;
  int urb_type;
  int direction;
  int length;
  int incomplete;
  int request, value, index;
  unsigned char *buffer;
  double timestamp;
} urb_t, *urb_p;

FILE *fp;

urb_t urb[MAX_URBS];
int highest_urb = 0;
unsigned int bulk_in_hand = 0, bulk_out_hand = 0;
unsigned int int_in_hand = 0;

int isblank (int c);

/* cut off the \n and \r characters at the end of a line of text */

void chomp(char *str)
{
  while(strlen(str) > 0 && 
	(str[strlen(str) - 1] == '\n' || str[strlen(str) - 1] == '\r'))
    str[strlen(str) - 1] = '\0';
}

/* advance to the next word from a string */

char *next_word(char *str)
{
  char *mark = str;

  if(str == NULL)
    return NULL;
  while(mark[0] != '\0' && !isblank(mark[0]))
    mark++;
  if(mark[0] == '\0')
    return NULL;
  while(mark[0] != '\0' && isblank(mark[0]))
    mark++;
  if(mark[0] == '\0')
    return NULL;
  return mark;
}

/* return a pointer to the "nth" word of a string */

char *nth_word(char *str, int n)
{
  char *mark;
  int i;

  mark = str;
  for(i = 0; i < n - 1; i++)
    mark = next_word(mark);
  return mark;
}

void clear_urb_text(urb_text_p text)
{
  text->num_lines = 0;
  text->max_lines = 10;
  text->lines = (char **)calloc(sizeof(char *), text->max_lines);  
}

void clear_urb_list(urb_p urb)
{
  int i;

  for(i = 0; i < MAX_URBS; i++) {
    clear_urb_text(&(urb[i].in));
    clear_urb_text(&(urb[i].out));

    urb[i].active = 0;
    urb[i].buffer = NULL;
    urb[i].request = 0;
    urb[i].value = 0;
    urb[i].index = 0;
    urb[i].incomplete = 0;
  }
}

void add_line_to_urb(urb_text_p urb, char *line)
{
  if(urb->num_lines == urb->max_lines) {
    urb->max_lines += 10;
    urb->lines = realloc(urb->lines, sizeof(char *) * urb->max_lines);
  }
  urb->lines[urb->num_lines] = (char *)malloc(strlen(line) + 1);
  strcpy(urb->lines[urb->num_lines], line);
  (urb->num_lines)++;
}

void cut_logfile_into_urbs(FILE *fp, urb_p urb)
{
  char line[2000], *mark;
  long int line_count;
  double line_timestamp;
  int current_urb = -1;
  int i, out_urb_count = 0, in_urb_count = 0;
  int direction = OUT;

  while(!feof(fp)) {
    if(fgets(line, 2000, fp)) {
      chomp(line);
      sscanf(line, "%ld %lf", &line_count, &line_timestamp);
      mark = next_word(next_word(line));
      if(mark != NULL) {
	if(strncmp(mark, ">>>>", 4) == 0) {
	  sscanf(mark, ">>>>>>> URB %d", &current_urb);
	  direction = OUT;
	}
	else if(strncmp(mark, "<<<<", 4) == 0) {
	  sscanf(mark, "<<<<<<< URB %d", &current_urb);
	  direction = IN;
	}
	if(current_urb >= 0) {
	  if(direction == OUT)
	    add_line_to_urb(&(urb[current_urb].out), line);
	  else 
	    add_line_to_urb(&(urb[current_urb].in), line);
	}
      }
    }
  }
  for(i = 0; i < MAX_URBS; i++) {
    if(urb[i].out.num_lines > 0) {
      out_urb_count++;
      highest_urb = i;
    }
    if(urb[i].in.num_lines > 0) {
      in_urb_count++;
      highest_urb = i;
    }
  }
  printf("URB COUNT:\n  OUTBOUND : %d\n  INBOUND  : %d\n\n", out_urb_count, in_urb_count);
}

void process_configuration_urb(urb_p urb)
{
  int i, interface_num, pipe_num, pipe_type, ep_address, pipe_handle;
  
  urb->urb_type = IGNORED;
  for(i = 1; i < urb->in.num_lines; i++) {
    if(strncmp(nth_word(urb->in.lines[i], 3), "Interface", 9) == 0 &&
       strncmp(nth_word(urb->in.lines[i], 4), "Pipes", 5) == 0) {
      sscanf(nth_word(urb->in.lines[i], 3), "Interface[%d", &interface_num);
      sscanf(nth_word(urb->in.lines[i], 3), "Pipes[%d", &pipe_num);
      if(strncmp(nth_word(urb->in.lines[i], 6), "PipeType", 8) == 0)
	sscanf(nth_word(urb->in.lines[i], 8), "%x", &pipe_type);
      else if(strncmp(nth_word(urb->in.lines[i], 6), "EndpointAddress", 15)==0)
	sscanf(nth_word(urb->in.lines[i], 8), "%x", &ep_address);
      else if(strncmp(nth_word(urb->in.lines[i], 6), "PipeHandle", 10) == 0) {
	sscanf(nth_word(urb->in.lines[i], 8), "%x", &pipe_handle);
	switch(pipe_type) {
	case 0x02:
	  if(ep_address & 0xf0)
	    bulk_in_hand = pipe_handle;
	  else
	    bulk_out_hand = pipe_handle;
	  break;
	case 0x03:
	  int_in_hand = pipe_handle;
	  break;
	}
      }
    }
  }
  printf("ENDPOINT HANDLES:\n  BULK IN  : 0x%x\n  BULK OUT : 0x%x\n"
	 "  INT IN   : 0x%x\n\n", bulk_in_hand, bulk_out_hand, int_in_hand);
}

void process_control_urb(urb_p urb)
{
  int temp, current_line, count;
  char *mark;

  urb->urb_type = CONTROL;
  
  /* get message direction */
  if(strncmp(nth_word(urb->in.lines[3], 6) + 1, 
	     "USBD_TRANSFER_DIRECTION_IN", 26) == 0)
    urb->direction = IN;
  else
    urb->direction = OUT;
  
  /* get message length */
  sscanf(nth_word(urb->in.lines[4], 5), "%x", &(urb->length));
  
  /* get message contents */
  count = 0;
  urb->buffer = (char *)calloc(1, urb->length);
  if(urb->direction == IN) {

    current_line = 8;
    mark = nth_word(urb->in.lines[current_line], 3);
    count = 0;

    while(count < urb->length) {
      if(mark == NULL) {
	urb->incomplete = 1;
	break;
      }
      
      sscanf(mark, "%x", &temp);
      urb->buffer[count] = (unsigned char)temp;

      count++;
      if(count % 16 == 0) {
	current_line += 2;
	if(current_line >= urb->in.num_lines) {
	  urb->incomplete = 1;
	  break;
	}
	mark = nth_word(urb->in.lines[current_line], 3);
      }
      else
	mark = next_word(mark);
    }

    /* get request, value, & index */
    sscanf(nth_word(urb->out.lines[8], 5), "%x", &urb->request);
    sscanf(nth_word(urb->out.lines[9], 5), "%x", &urb->value);
    sscanf(nth_word(urb->out.lines[10], 5), "%x", &urb->index);
  }
  else if(urb->direction == OUT) {
    current_line = 7;
    while(count < urb->length) {
      if(current_line >= urb->out.num_lines) {
	urb->incomplete = 1;
	break;
      }
      sscanf(nth_word(urb->out.lines[current_line], 3), "%x", &temp);
      urb->buffer[count] = (unsigned char)temp;
      count++;
      current_line++;
      if(count > 0 && count % 16 == 0 && count != urb->length)
	current_line++;
    }
    current_line += 2;
    /* get request, value, & index */
    if(!urb->incomplete) {
      sscanf(nth_word(urb->out.lines[current_line], 5), "%x", &urb->request);
      sscanf(nth_word(urb->out.lines[current_line + 1], 5), "%x", &urb->value);
      sscanf(nth_word(urb->out.lines[current_line + 2], 5), "%x", &urb->index);
    }
  }
}

void process_int_urb(urb_p urb)
{
  int temp, count, current_line;

  urb->urb_type = INT;
  urb->direction = IN;
  /* get message length */
  sscanf(nth_word(urb->in.lines[4], 5), "%x", &(urb->length));

  count = 0;
  urb->buffer = (char *)calloc(1, urb->length);

  current_line = 8;
  while(count < urb->length) {
    sscanf(nth_word(urb->in.lines[current_line], 3), "%x", &temp);
    urb->buffer[count] = (unsigned char)temp;
    count++;
    current_line++;
    if(current_line >= urb->in.num_lines) {
      urb->incomplete = 1;
      break;
    }
    if(count > 0 && count % 16 == 0)
      current_line++;
  }
}

void process_bulk_urb(urb_p urb)
{
  unsigned int hand;
  int current_line, count, temp;
  char *mark;
  
  sscanf(nth_word(urb->in.lines[2], 5), "%x", &hand);
  if(hand == bulk_in_hand) {
    urb->urb_type = BULK;
    urb->direction = IN;
    /* get message length */
    sscanf(nth_word(urb->in.lines[4], 5), "%x", &(urb->length));
    urb->buffer = (char *)calloc(1, urb->length);

    current_line = 8;
    mark = nth_word(urb->in.lines[current_line], 3);
    count = 0;
    while(count < urb->length) {
      if(mark == NULL) {
	urb->incomplete = 1;
	break;
      }

      sscanf(mark, "%x", &temp);
      urb->buffer[count] = (unsigned char)temp;

      count++;
      if(count % 16 == 0) {
	current_line += 2;
	if(current_line >= urb->in.num_lines) {
	  urb->incomplete = 1;
	  break;
	}
	mark = nth_word(urb->in.lines[current_line], 3);
      }
      else
	mark = next_word(mark);
    }
  }
  else if(hand == bulk_out_hand) {
    urb->urb_type = BULK;
    urb->direction = OUT;
  }
}

void process_urbs(void)
{
  int i;
  unsigned int hand;

  for(i = 1; i < highest_urb; i++) {
    fprintf(stderr, "%d ", i);
    if(urb[i].out.num_lines > 0 && urb[i].in.num_lines > 0) {
      urb[i].active = 1;
      
      /* get timestamp */
      sscanf(nth_word(urb[i].out.lines[0], 2), "%lf", &urb[i].timestamp);
      
      /* process control messages */
      if(strncmp(nth_word(urb[i].in.lines[1], 4),
		 "URB_FUNCTION_SELECT_CONFIGURATION", 33) == 0)
	process_configuration_urb(&(urb[i]));
      else if(strncmp(nth_word(urb[i].in.lines[1], 4),
		 "URB_FUNCTION_CONTROL", 20) == 0 &&
	 strncmp(nth_word(urb[i].out.lines[1], 4),
		 "URB_FUNCTION_VENDOR", 19) == 0)
	process_control_urb(&(urb[i]));
      /* process bulk and interrupt messages */
      else if(strncmp(nth_word(urb[i].in.lines[1], 4),
		      "URB_FUNCTION_BULK", 17) == 0) {
	sscanf(nth_word(urb[i].in.lines[2], 5), "%x", &hand);
	if(hand == int_in_hand)
	  process_int_urb(&(urb[i]));
	else 
	  process_bulk_urb(&(urb[i]));
      }
      else if(strncmp(nth_word(urb[i].in.lines[1], 4),
		      "URB_FUNCTION_CONTROL", 20) == 0 ||
	      strncmp(nth_word(urb[i].in.lines[1], 4),
		      "URB_FUNCTION_SELECT", 19) == 0)
	urb[i].urb_type = IGNORED;
      else
	urb[i].urb_type = UNKNOWN;
    }
    else if(urb[i].out.num_lines != 0 || urb[i].in.num_lines != 0)
      urb[i].incomplete = 1;
  }
  fprintf(stderr, "\n");
}

void print_buffer(unsigned char *buffer, int length)
{
  int i, j;
  int new_length = (int)ceil(length / 16.0) * 16;

  for(i = 0; i < new_length; i++) {
    if(i < length)
      printf("%02x ", buffer[i]);
    else
      printf("   ");
    if((i + 1) % 16 == 0) {
      printf("    ");
      for(j = i - 15; j <= MIN(i, length - 1); j++)
	if(isprint(buffer[j]))
	  printf("%c", buffer[j]);
	else
	  printf(".");
      if(i < new_length - 1)
	printf("\n");
    }
  }
}

char *interpret_rio_control_msg(int request, 
				int value __attribute__ ((unused)), 
				int index __attribute__ ((unused)), 
				unsigned char *buffer __attribute__ ((unused)),
				int length __attribute__ ((unused)))
{
  switch(request) {
  case 0x60:
    return "RIO MESSAGE : PACKET START";
  case 0x61:
    return "RIO MESSAGE : HEARTBEAT";
  case 0x62:
    return "RIO MESSAGE : DEVICE INFO PACKET";
  case 0x65:
    return "RIO MESSAGE : MESSAGE START";
  case 0x66:
    return "RIO MESSAGE : MESSAGE TERMINATOR";
  case 0x68:
    return "RIO MESSAGE : MEMORY INFO PACKET";
  case 0x69:
    return "RIO MESSAGE : SONG INFO PACKET";
  default:
    return "UNINTERPRETED";
  }
}

char *interpret_canon_msg(unsigned char *buffer, int length)
{
  unsigned int cmd1, cmd2, cmd3, rcc_cmd = 0;
  static char err_str[200];

  if(length < 0x50)
    return "CANON MESSAGE : broken message";
  else {
    cmd1 = (buffer[5] << 8) | buffer[4];
    cmd2 = buffer[0x44];
    cmd3 = buffer[0x47];
    if(length > 0x50)
      rcc_cmd = buffer[0x50];
    sprintf(err_str, "CANON MESSAGE : unknown 0x%04x 0x%02x 0x%02x", 
	    cmd1, cmd2, cmd3);

    if(cmd1 == 0x201) {
      if(cmd3 == 0x11) {
	switch(cmd2) {
	case 0x05:
	  return "CANON MESSAGE : Make directory";
	case 0x06:
	  return "CANON MESSAGE : Remove directory";
	case 0x09:
	  return "CANON MESSAGE : Disk info request";
	case 0x0d:
	  return "CANON MESSAGE : Delete file";
	case 0x0e:
	  return "CANON MESSAGE : Set file attribute";
	}
      }
      else if(cmd3 == 0x12) {
	switch(cmd2) {
	case 0x01:
	  return "CANON MESSAGE : Identify camera";
	case 0x03:
	  return "CANON MESSAGE : Get time";
	case 0x04:
	  return "CANON MESSAGE : Set time";
	case 0x05:
	  return "CANON MESSAGE : Change owner";
	case 0x0a:
	  return "CANON MESSAGE : Power supply status";
	case 0x13:
	  if(length > 0x50)
	    switch(rcc_cmd) {
	    case 0x00:
	      return "CANON MESSAGE : RCC - init";
	    case 0x04:
	      return "CANON MESSAGE : RCC - release shutter";
	    case 0x07:
	      return "CANON MESSAGE : RCC - set release params";
	    case 0x09:
	      return "CANON MESSAGE : RCC - set transfer mode";
	    case 0x0a:
	      return "CANON MESSAGE : RCC - get release params";
	    case 0x0b:
	      return "CANON MESSAGE : RCC - get zoom position";
	    case 0x0c:
	      return "CANON MESSAGE : RCC - set zoom position";
	    case 0x10:
	      return "CANON MESSAGE : RCC - get extended release params size";
	    case 0x12:
	      return "CANON MESSAGE : RCC - get extended release params";
	    case 0x01:
	      return "CANON MESSAGE : RCC - exit release control";
	    case 0x02:
	      return "CANON MESSAGE : RCC - start viewfinder";
	    case 0x03:
	      return "CANON MESSAGE : RCC - stop viewfinder";
	    case 0x0d:
	      return "CANON MESSAGE : RCC - get available shots";
	    case 0x0e:
	      return "CANON MESSAGE : RCC - set custom func.";
	    case 0x0f:
	      return "CANON MESSAGE : RCC - get custom func.";
	    case 0x11:
	      return "CANON MESSAGE : RCC - get extended params version";
	    case 0x13:
	      return "CANON MESSAGE : RCC - set extended params";
	    case 0x14:
	      return "CANON MESSAGE : RCC - select camera output";
	    case 0x15:
	      return "CANON MESSAGE : RCC - Do AE, AF and AWB";
	    }
	  return "CANON MESSAGE : Remote camera control - unknown subcommand";
	case 0x1b:
	  return "CANON MESSAGE : EOS lock keys";
	case 0x1c:
	  return "CANON MESSAGE : EOS unlock keys";
	case 0x1f:
	  return "CANON MESSAGE : Get picture abilities";
	case 0x20:
	  return "CANON MESSAGE : Lock keys and turn off LCD";
	}
      }
    }
    else if(cmd1 == 0x202) {
      if(cmd2 == 0x01 && cmd3 == 0x11)
	return "CANON MESSAGE : Get file";
      else if(cmd2 == 0x0a && cmd3 == 0x11)
	return "CANON MESSAGE : Flash device identify";
      else if(cmd2 == 0x0b && cmd3 == 0x11)
	return "CANON MESSAGE : Get directory entries";
      else if(cmd2 == 0x17 && cmd3 == 0x12)
	return "CANON MESSAGE : RCC - Download captured image";
      else if(cmd2 == 0x18 && cmd3 == 0x12)
	return "CANON MESSAGE : RCC - Download a captured preview";
    }

    return err_str;
  }
}

void print_processed_urbs(urb_p urb)
{
  int i;

  for(i = 0; i <= highest_urb; i++)
    if(urb[i].active) {
      switch(urb[i].urb_type) {
      case IGNORED:
	printf("URB %5d : IGNORED CONTROL TRANSFER\n\n", i);
	break;
      case CONTROL:
	printf("URB %5d : %.3f : CONT %s len = 0x%04x  req = 0x%02x  val = 0x%04x  ind = 0x%04x %s\n", 
	       i, urb[i].timestamp, (urb[i].direction == IN) ? "IN " : "OUT", 
	       urb[i].length, urb[i].request, urb[i].value, urb[i].index,
	       urb[i].incomplete ? "- INCOMPLETE" : "");
	if(urb[i].buffer != NULL)
	  print_buffer(urb[i].buffer, urb[i].length);
	printf("\n");
	if(urb[i].direction == OUT && USE_CANON)
	  printf("%s\n", interpret_canon_msg(urb[i].buffer, urb[i].length));
	else if(urb[i].direction == IN && USE_RIO)
	  printf("%s\n", interpret_rio_control_msg(urb[i].request, 
						   urb[i].value, urb[i].index,
						   urb[i].buffer, 
						   urb[i].length));
	printf("\n");
	break;
      case BULK:
	if(urb[i].direction == IN)
	  printf("URB %5d : %.3f : BULK IN  len = 0x%04x\n",
		 i, urb[i].timestamp, urb[i].length);
	else
	  printf("URB %5d : %.3f : BULK OUT ???\n", i, urb[i].timestamp);
	if(urb[i].buffer != NULL)
	  print_buffer(urb[i].buffer, urb[i].length);
	printf("\n");
	printf("\n");
	break;
      case INT:
	if(urb[i].length != 0) {
	  printf("URB %5d : %.3f : INT IN   len = 0x%04x\n", i, 
		 urb[i].timestamp, urb[i].length);
	  if(urb[i].buffer != NULL)
	    print_buffer(urb[i].buffer, urb[i].length);
	  printf("\n\n");
	}
	break;
      case UNKNOWN:
	printf("URB %5d : unknown\n", i);
	break;
      }
    }
    else if(urb[i].incomplete)
      printf("URB %5d : INCOMPLETE URB\n\n", i);
}

int main(int argc, char **argv)
{
  if(argc < 2) {
    fprintf(stderr, "Error: not enough arguments.\n");
    fprintf(stderr, "Usage: %s filename.\n", argv[0]);
    exit(1);
  }
  fp = fopen(argv[1], "r");
  if(fp == NULL) {
    fprintf(stderr, "Error: could not open file %s for reading.\n", argv[1]);
    exit(1);
  }

  clear_urb_list(urb);
  cut_logfile_into_urbs(fp, urb);
  process_urbs();
  print_processed_urbs(urb);
  return 0;
}
