
#ifndef VALET_MESSAGES_H
#define VALET_MESSAGES_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
  double timestamp;
  char host[10];
} carmen_valet_msg;

#define CARMEN_VALET_PARK_MSG_NAME "carmen_valet_park_msg"
#define CARMEN_VALET_PARK_MSG_FMT "{double, [char:10]}"

#define CARMEN_VALET_RETURN_MSG_NAME "carmen_valet_return_msg"
#define CARMEN_VALET_RETURN_MSG_FMT "{double, [char:10]}"


#ifdef __cplusplus
}
#endif

#endif
