
#ifndef ROOMNAV_TYPES_H
#define ROOMNAV_TYPES_H

#include <carmen/carmen.h>

#ifdef __cplusplus
extern "C" {
#endif


/*
 * (pos.x, pos.y) is the center of the door.
 * pose.theta is between 0 and 2*pi and indicates the 
 * angle of the vector from room 1 to room 2
 * normal to the door at the center point.
 * points.num_places == 0 and width > 0 => door is a ceiling.
 * points.num_places == 0 and width < 0 => door is a floor.
 * if door is a floor/ceiling, it is not shared between two rooms.
 * thus, room1 is the room the door is in, and room2 is the room
 * (on another floor, either up or down) that the door goes to.
 */
struct carmen_door {
  int room1;
  int room2;
  carmen_map_placelist_t points;
  carmen_point_t pose;  // in world coordinates
  double width;  // in meters
  int num;
  int is_real_door;  // or just a border between rooms
};

struct carmen_room {
  int num;
  char *name;
  int *doors;
  int num_doors;
  int type;
  int num_cells;
  double ux;   // mean x
  double uy;   // mean y
  double vx;   // var(x)
  double vy;   // var(y)
  double vxy;  // cov(x,y)
  double w1;   // major eigenvalue
  double w2;   // minor eigenvalue
  double theta;  // orientation of bivariate normal
  carmen_point_t e1;  // endpoint 1 (for hallways only)
  carmen_point_t e2;  // endpoint 2 (for hallways only)
};

typedef struct carmen_door carmen_door_t, *carmen_door_p;
typedef struct carmen_room carmen_room_t, *carmen_room_p;

typedef struct {
  carmen_room_p rooms;
  int num_rooms;
  carmen_door_p doors;
  int num_doors;
} carmen_rooms_topology_t, *carmen_rooms_topology_p;

#define GRID_NONE     -1
#define GRID_UNKNOWN  -2
#define GRID_WALL     -3
#define GRID_DOOR     -4
#define GRID_PROBE    -5

#define CARMEN_ROOM_TYPE_UNKNOWN -1
#define CARMEN_ROOM_TYPE_ROOM     0
#define CARMEN_ROOM_TYPE_HALLWAY  1

#ifdef __cplusplus
}
#endif

#endif
