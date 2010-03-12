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

#ifndef CARMEN_HOKUYO_MESSAGES_H
#define CARMEN_HOKUYO_MESSAGES_H

#include <carmen/laser_messages.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef carmen_laser_laser_message carmen_hokuyo_pb9_message;

#define      CARMEN_HOKUYO_PB9_NAME       "carmen_hokuyo_bp9"
#define      CARMEN_HOKUYO_PB9_FMT        CARMEN_LASER_FRONTLASER_FMT

typedef struct {
  int stalled;
} carmen_hokuyo_alive_message;

#define      CARMEN_HOKUYO_ALIVE_NAME            "carmen_hokuyo_alive"
#define      CARMEN_HOKUYO_ALIVE_FMT             "{int}"


#ifdef __cplusplus
}
#endif

#endif

