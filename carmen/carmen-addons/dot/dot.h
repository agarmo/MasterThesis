#ifndef DOT_H
#define DOT_H

#include <carmen/carmen.h>

#ifdef __cplusplus
extern "C" {
#endif


#define CARMEN_DOT_PERSON  0
#define CARMEN_DOT_TRASH   1
#define CARMEN_DOT_DOOR    2

typedef enum {carmen_dot_person, carmen_dot_trash, carmen_dot_door} carmen_dot_enum_t;

typedef struct {
  int id;
  double x;
  double y;
  double r;
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
  double *xhull;
  double *yhull;
  int hull_size;
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

typedef struct {
  int dot_type;
  union {
    carmen_dot_door_t door;
    carmen_dot_person_t person;
    carmen_dot_trash_t trash;
  } data;
} carmen_dot_t;

#ifdef __cplusplus
}
#endif

#endif
