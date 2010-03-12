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

#ifndef CARMEN_NAVIGATOR_MESSAGES_H
#define CARMEN_NAVIGATOR_MESSAGES_H

#include <carmen/global.h>
#include <carmen/map.h>

#ifdef __cplusplus
extern "C" {
#endif
  
  typedef struct {
    double timestamp;
    char host[10];
  } carmen_navigator_query_message;
  
#define CARMEN_NAVIGATOR_STATUS_QUERY_NAME       "carmen_navigator_status_query"
#define CARMEN_NAVIGATOR_STATUS_QUERY_FMT        "{double,[char:10]}"
  
#define CARMEN_NAVIGATOR_PLAN_QUERY_NAME         "carmen_navigator_plan_query"
#define CARMEN_NAVIGATOR_PLAN_QUERY_FMT          "{double,[char:10]}"
  
  typedef struct {
    int autonomous;
    int goal_set;
    carmen_point_t goal;
    carmen_traj_point_t robot;
    double timestamp;
    char host[10];
  } carmen_navigator_status_message;
  
#define CARMEN_NAVIGATOR_STATUS_NAME       "carmen_navigator_status"
#define CARMEN_NAVIGATOR_STATUS_FMT        "{int,int,{double, double, double},{double, double, double, double, double},double,[char:10]}"
  
  typedef struct {
    carmen_traj_point_t *path;
    int path_length;
    double timestamp;
    char host[10];
  } carmen_navigator_plan_message;
  
#define      CARMEN_NAVIGATOR_PLAN_NAME       "carmen_navigator_plan"
#define      CARMEN_NAVIGATOR_PLAN_FMT        "{<{double, double, double, double, double}:2>,int,double,[char:10]}"
  
  typedef struct {
    double x, y;
    double timestamp;
    char host[10];
  } carmen_navigator_set_goal_message;
  
#define      CARMEN_NAVIGATOR_SET_GOAL_NAME         "carmen_navigator_set_goal"
#define      CARMEN_NAVIGATOR_SET_GOAL_FMT          "{double,double,double,[char:10]}"
  
  typedef struct {
    carmen_point_t goal;
    double timestamp;
    char host[10];
  } carmen_navigator_set_goal_triplet_message;
  
#define      CARMEN_NAVIGATOR_SET_GOAL_TRIPLET_NAME         "carmen_navigator_set_goal_triplet"
#define      CARMEN_NAVIGATOR_SET_GOAL_TRIPLET_FMT          "{{double,double,double},double,[char:10]}"
  
  typedef struct {
    double timestamp;
    char host[10];
  } carmen_navigator_stop_message;
  
#define      CARMEN_NAVIGATOR_STOP_NAME         "carmen_navigator_stop"
#define      CARMEN_NAVIGATOR_STOP_FMT          "{double,[char:10]}"
  
  typedef struct {
    double timestamp;
    char host[10];
  } carmen_navigator_go_message;
  
#define      CARMEN_NAVIGATOR_GO_NAME       "carmen_navigator_go"
#define      CARMEN_NAVIGATOR_GO_FMT        "{double,[char:10]}"
  
  typedef enum{CARMEN_NAVIGATOR_GOAL_REACHED_v,
		 CARMEN_NAVIGATOR_USER_STOPPED_v,
		 CARMEN_NAVIGATOR_UNKNOWN_v} carmen_navigator_reason_t;
  
  typedef struct {
    double timestamp;
    char host[10];
    carmen_navigator_reason_t reason;
  } carmen_navigator_autonomous_stopped_message;
  
#define      CARMEN_NAVIGATOR_AUTONOMOUS_STOPPED_NAME "carmen_navigator_autonomous_stopped"
#define      CARMEN_NAVIGATOR_AUTONOMOUS_STOPPED_FMT "{double,[char:10],int}"
  
typedef enum {CARMEN_NAVIGATOR_MAP_v, CARMEN_NAVIGATOR_ENTROPY_v, 
	      CARMEN_NAVIGATOR_COST_v, CARMEN_NAVIGATOR_UTILITY_v,
              CARMEN_LOCALIZE_LMAP_v, CARMEN_LOCALIZE_GMAP_v} 
  carmen_navigator_map_t;
  
  typedef struct {
    carmen_navigator_map_t map_type;
    double timestamp;
    char host[10];
  } carmen_navigator_map_request_message;
  
#define      CARMEN_NAVIGATOR_MAP_REQUEST_NAME       "carmen_navigator_map_request"
#define      CARMEN_NAVIGATOR_MAP_REQUEST_FMT        "{int,double,[char:10]}"
  
  typedef struct {
    unsigned char *data;    
    int size;
    int compressed;
    carmen_map_config_t config;
    carmen_navigator_map_t map_type;
    double timestamp;
    char host[10];
  } carmen_navigator_map_message;  
  
#define      CARMEN_NAVIGATOR_MAP_NAME      "carmen_navigator_map"
#define      CARMEN_NAVIGATOR_MAP_FMT       "{<char:2>,int,int,{int,int,double,string},int,double,[char:10]}"
  
  typedef struct {
    char *placename;
    double timestamp;
    char host[10];
  } carmen_navigator_placename_message;
  
#define CARMEN_NAVIGATOR_SET_GOAL_PLACE_NAME "carmen_navigator_set_goal_place"
#define CARMEN_NAVIGATOR_SET_GOAL_PLACE_FMT "{string,double,[char:10]}"
  
  typedef struct {
    int code;
    char *error;
    double timestamp;
    char host[10];
  } carmen_navigator_return_code_message;
  
#define CARMEN_NAVIGATOR_RETURN_CODE_NAME "carmen_navigator_return_code"
#define CARMEN_NAVIGATOR_RETURN_CODE_FMT "{int, string, double, [char:10]}"
  
  /*
    Attribute can be one of:
    "robot colour"
    "goal colour" 
    "path colour" 
    "track robot"
    "draw waypoints"
    "show particles"
    "show gaussians"
    "show laser"
    "show simulator"
    "show people"
    
    The colours are RGB in int form, i.e., R << 16 | G << 8 | B.
    A colour of -1 restores the default colour.
    The other attributes are binary. -1 is restore default, 0 is turn off,
    1 is turn on, other values are ignored.
    
    Setting the reset_all_to_defaults flag causes all attributes
    to be reset to defaults.
    
    The status_message is currently ignored. There will eventually be
    a status window that will get the contents of this field.
  */
  
  typedef struct {
    char *attribute;
    int value;
    char *status_message;
    int reset_all_to_defaults;
    double timestamp;
    char host[10];
  } carmen_navigator_display_config_message;
  
#define CARMEN_NAVIGATOR_DISPLAY_CONFIG_NAME "carmen_navigator_display_config"
#define CARMEN_NAVIGATOR_DISPLAY_CONFIG_FMT "{string, int, string, int, double, [char:10]}"
  
#ifdef __cplusplus
}
#endif

#endif

