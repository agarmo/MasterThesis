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

#include "mlm.h"
#include "roomnav_interface.h"

static mlm_map_t *mlm_get_map_by_name(mlm_t *mlm, char *map_name) {

  int i;

  for (i = 0; i < mlm->num_maps; i++)
    if (!strcmp(mlm->maps[i].map_name, map_name))
      return &mlm->maps[i];

  return NULL;
}

mlm_point_t *mlm_get_point(mlm_t *mlm) {

  mlm_map_t map;
  carmen_map_t gridmap;
  carmen_map_point_t map_point;
  carmen_world_point_t world_point;
  int i, r1, r2;

  map = mlm->maps[mlm->current_map_index];
  
  carmen_map_get_gridmap(&gridmap);
  map_point.map = world_point.map = &gridmap;

  for (i = 0; i < map.num_points; i++) {
    world_point.pose = map.points[i].pose;
    carmen_world_to_map(&world_point, &map_point);
    r1 = carmen_roomnav_get_room_from_point(map_point);
    r2 = carmen_roomnav_get_room();
    printf("map point %d (%.2f, %.2f, %.2f) is in room %d\n", i,
	   world_point.pose.x, world_point.pose.y, world_point.pose.theta, r1);
    printf("robot is in room %d\n", r2);
    if (carmen_roomnav_get_room_from_point(map_point) ==
	carmen_roomnav_get_room())
      break;
  }

  if (i < map.num_points)
    return &map.points[i];

  return NULL;
}

/*
static mlm_point_t *mlm_get_point_at(mlm_t *mlm, carmen_point_t pose) {

  mlm_map_t map;
  carmen_map_t gridmap;
  carmen_map_point_t map_point_i, map_point;
  carmen_world_point_t world_point_i, world_point;
  int i;

  printf("mlm_get_point_at()\n");

  map = mlm->maps[mlm->current_map_index];

  printf("setting map: %s\n", map.map_name);

  carmen_map_set_filename(map.map_name);

  printf("getting map...\n");

  carmen_map_get_gridmap(&gridmap);
  map_point_i.map = map_point.map = world_point_i.map = world_point.map = &gridmap;

  for (i = 0; i < map.num_points; i++) {
    world_point_i.pose = map.points[i].pose;
    world_point.pose = pose;
    carmen_world_to_map(&world_point_i, &map_point_i);
    carmen_world_to_map(&world_point, &map_point);
    if (carmen_roomnav_get_room_from_point(map_point_i) ==
	carmen_roomnav_get_room_from_point(map_point))
      break;
  }

  printf("end mlm_get_point_at()\n");

  if (i < map.num_points)
    return &map.points[i];

  return NULL;
}
*/

static mlm_point_t *mlm_add_point(mlm_t *mlm, mlm_map_t *map, carmen_point_t *pose,
				  mlm_map_t *up_map, carmen_point_t *up_pose,
				  mlm_map_t *down_map, carmen_point_t *down_pose) {
  int i;
  mlm_point_t *point;

  printf("mlm_add_point()\n");

  mlm = NULL; //dbug

  for (i = 0; i < map->num_points; i++)
    if (!memcmp(&map->points[i].pose, &pose, sizeof(carmen_point_t)))
      break;

  if (i == map->num_points) {
    /*
      if (mlm_get_point_at(mlm, *pose) != NULL)
        carmen_die("Only one level-switching point allowed per room");
    */
    map->points = (mlm_point_t *) realloc(map->points, (map->num_points + 1) *
					  sizeof(mlm_point_t));
    carmen_test_alloc(map->points);
    map->num_points++;
    memset(&map->points[i], 0, sizeof(mlm_point_t));
  }

  point = &map->points[i];

  point->pose = *pose;

  if (up_map) {
    printf("up_pose = (%.2f, %.2f, %.2f)\n",
	   up_pose->x, up_pose->y, up_pose->theta);
    point->up_map = up_map;
    point->up_pose = *up_pose;
    printf("point->up_pose = (%.2f, %.2f, %.2f)\n",
	   point->up_pose.x, point->up_pose.y, point->up_pose.theta);
  }
  if (down_map) {
    printf("down_pose = (%.2f, %.2f, %.2f)\n",
	   down_pose->x, down_pose->y, down_pose->theta);
    point->down_map = down_map;
    point->down_pose = *down_pose;
    printf("point->down_pose = (%.2f, %.2f, %.2f)\n",
	   point->down_pose.x, point->down_pose.y, point->down_pose.theta);
  }

  printf("end mlm_add_point()\n");

  return point;
}

static mlm_map_t *mlm_add_map(mlm_t *mlm, char *map_name) {

  mlm_map_t *map;

  if (!carmen_file_exists(map_name))
    carmen_die("Could not find file %s\n", map_name);  

  printf("mlm_add_map()\n");

  map = mlm_get_map_by_name(mlm, map_name);

  if (map != NULL)
    return map;

  mlm->maps = (mlm_map_t *)
    realloc(mlm->maps, (mlm->num_maps + 1)*sizeof(mlm_map_t));
  carmen_test_alloc(mlm->maps);
  
  map = &mlm->maps[mlm->num_maps];
  memset(map, 0, sizeof(mlm_map_t));

  map->map_num = mlm->num_maps;
  map->map_name = (char *) calloc(strlen(map_name) + 1, sizeof(char));
  carmen_test_alloc(map->map_name);
  strcpy(map->map_name, map_name);

  mlm->num_maps++;

  return map;
}

static char *next_word(char *str) {

  char *mark = str;

  mark += strcspn(mark, " \t");
  mark += strspn(mark, " \t");

  return mark;
}

void read_mlm(mlm_t *mlm, char *filename) {

  FILE *fp;
  char token[100], map_name1[100], map_name2[100];
  char line[2000], *mark;
  mlm_map_t *map1, *map2;
  carmen_point_t pose1, pose2;
  int n;

  if (carmen_file_exists(filename) == 0)
    carmen_die("Could not find file %s\n", filename);

  if (strcmp(carmen_file_extension(filename), ".mlm"))
    carmen_die("%s is not a valid mlm file.\n", filename);

  fp = fopen(filename, "r");

  memset(mlm, 0, sizeof(mlm_t));

  fgets(line, 2000, fp);
  for (n = 1; !feof(fp); n++) {
    printf("---> n = %d, line = %s\n", n, line); 
    if (sscanf(line, "%s", token) == 1) {
      if (!strcmp(token, "MAPS")) {
	mark = next_word(line);
	while (strlen(mark) > 0) {
	  sscanf(mark, "%s", token);
	  mlm_add_map(mlm, token);
	  mark = next_word(mark);
	}
      }
      else if (!strcmp(token, "POINT")) {
	mark = next_word(line);
	if (sscanf(mark, "%s %s %lf %lf %lf %lf %lf %lf", map_name1, map_name2,
		   &pose1.x, &pose1.y, &pose1.theta,
		   &pose2.x, &pose2.y, &pose2.theta) < 8)
	  carmen_die("<line %d> expected format: POINT <map filename 1> "
		     "<map filename 2> <x1> <y1> <theta1> <x2> <y2> <theta2>", n);
	map1 = mlm_add_map(mlm, map_name1);
	map2 = mlm_add_map(mlm, map_name2);
	mlm_add_point(mlm, map1, &pose1, map2, &pose2, NULL, NULL);
	mlm_add_point(mlm, map2, &pose2, NULL, NULL, map1, &pose1);
      }
    }

    fgets(line, 2000, fp);
  }
}

