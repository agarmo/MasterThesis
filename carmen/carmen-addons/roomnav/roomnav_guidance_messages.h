
#ifndef ROOMNAV_GUIDANCE_MESSAGES_H
#define ROOMNAV_GUIDANCE_MESSAGES_H

#include "roomnav_guidance.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
  int cmd;
  char *text;
  double theta;
  double dist;
  double timestamp;
  char host[10];
} carmen_roomnav_guidance_msg;

#define CARMEN_ROOMNAV_GUIDANCE_MSG_NAME "carmen_roomnav_guidance_msg"
#define CARMEN_ROOMNAV_GUIDANCE_MSG_FMT  "{int, string, double, double, double, [char:10]}"


typedef struct {
  int text;
  int audio;
  double timestamp;
  char host[10];
} carmen_roomnav_guidance_config_msg;

#define CARMEN_ROOMNAV_GUIDANCE_CONFIG_MSG_NAME "carmen_roomnav_guidance_config_msg"
#define CARMEN_ROOMNAV_GUIDANCE_CONFIG_MSG_FMT  "{int, int, double, [char:10]}"


#ifdef __cplusplus
}
#endif

#endif
