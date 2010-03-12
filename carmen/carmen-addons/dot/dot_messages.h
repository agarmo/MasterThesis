#ifndef DOT_MESSAGES_H
#define DOT_MESSAGES_H

#include <carmen/carmen.h>
#include "dot.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
  double timestamp;
  char host[10];
} carmen_dot_reset_msg;

#define CARMEN_DOT_RESET_MSG_NAME "carmen_dot_reset_msg"
#define CARMEN_DOT_RESET_MSG_FMT  "{double, [char:10]}"


typedef struct {
  int type;
  double timestamp;
  char host[10];
} carmen_dot_query;

#define CARMEN_DOT_QUERY_NAME "carmen_dot_query"
#define CARMEN_DOT_QUERY_FMT  "{int, double, [char:10]}"


typedef struct {
  carmen_dot_person_t person;
  int delete;
  double timestamp;
  char host[10];
} carmen_dot_person_msg;

#define CARMEN_DOT_PERSON_MSG_NAME "carmen_dot_person_msg"
#define CARMEN_DOT_PERSON_MSG_FMT  "{{int, double, double, double, double, double, double}, int, double, [char:10]}"

typedef struct {
  carmen_dot_person_p people;
  int num_people;
  double timestamp;
  char host[10];
} carmen_dot_all_people_msg;

#define CARMEN_DOT_ALL_PEOPLE_MSG_NAME "carmen_dot_all_people_msg"
#define CARMEN_DOT_ALL_PEOPLE_MSG_FMT  "{<{int, double, double, double, double, double, double}:2>, int, double, [char:10]}"

typedef struct {
  carmen_dot_trash_t trash;
  int delete;
  double timestamp;
  char host[10];
} carmen_dot_trash_msg;

#define CARMEN_DOT_TRASH_MSG_NAME "carmen_dot_trash_msg"
#define CARMEN_DOT_TRASH_MSG_FMT  "{{int, double, double, double, double, double, <double:9>, <double:9>, int}, int, double, [char:10]}"

typedef struct {
  carmen_dot_trash_p trash;
  int num_trash;
  double timestamp;
  char host[10];
} carmen_dot_all_trash_msg;

#define CARMEN_DOT_ALL_TRASH_MSG_NAME "carmen_dot_all_trash_msg"
#define CARMEN_DOT_ALL_TRASH_MSG_FMT  "{<{int, double, double, double, double, double, <double:9>, <double:9>, int}:2>, int, double, [char:10]}"

typedef struct {
  carmen_dot_door_t door;
  int delete;
  double timestamp;
  char host[10];
} carmen_dot_door_msg;

#define CARMEN_DOT_DOOR_MSG_NAME "carmen_dot_door_msg"
#define CARMEN_DOT_DOOR_MSG_FMT  "{{int, double, double, double, double, double, double, double}, int, double, [char:10]}"

typedef struct {
  carmen_dot_door_p doors;
  int num_doors;
  double timestamp;
  char host[10];
} carmen_dot_all_doors_msg;

#define CARMEN_DOT_ALL_DOORS_MSG_NAME "carmen_dot_all_doors_msg"
#define CARMEN_DOT_ALL_DOORS_MSG_FMT  "{<{int, double, double, double, double, double, double, double}:2>, int, double, [char:10]}"


#ifdef __cplusplus
}
#endif

#endif
