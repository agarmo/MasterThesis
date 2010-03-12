#ifndef DOT_H
#define DOT_H

#include <carmen/carmen.h>

#ifdef __cplusplus
extern "C" {
#endif


#define CARMEN_DOT_PERSON  0
#define CARMEN_DOT_TRASH   1
#define CARMEN_DOT_DOOR    2


typedef struct {
  int id;
  double x;
  double y;
  double vx;
  double vy;
  double vxy;
} carmen_dot_person_t, *carmen_dot_person_p;

typedef struct {
  int id;
  double x;
  double y;
  double vx;
  double vy;
  double vxy;
} carmen_dot_trash_t, *carmen_dot_trash_p;

typedef struct {
  int id;
  double x;
  double y;
  double theta;
  double vx;
  double vy;
  double vxy;
  double vtheta;
} carmen_dot_door_t, *carmen_dot_door_p;


#ifdef __cplusplus
}
#endif

#endif
