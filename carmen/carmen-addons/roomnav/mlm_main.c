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
#include <carmen/localize_interface.h>

static mlm_t mlm = {NULL, 0, 0};
carmen_localize_globalpos_message global_pos;

static void set_robot_pose(carmen_point_t pose) {

  carmen_point_t std = {0.2, 0.2, carmen_degrees_to_radians(4.0)};
  carmen_localize_initialize_gaussian_command(pose, std);
}

static carmen_point_t transform_robot_pose(carmen_point_t pose0,
					   carmen_point_t pose1) {
  carmen_point_t robot_pose;
  carmen_point_t offset;

  offset.x = pose1.x - pose0.x;
  offset.y = pose1.y - pose0.y;
  offset.theta = pose1.theta - pose0.theta;

  robot_pose = global_pos.globalpos;
  robot_pose.x -= pose0.x;
  robot_pose.y -= pose0.y;
  robot_pose.theta -= pose0.theta;
  robot_pose.x = cos(offset.theta)*robot_pose.x - sin(offset.theta)*robot_pose.y;
  robot_pose.y = sin(offset.theta)*robot_pose.x + cos(offset.theta)*robot_pose.y;
  robot_pose.x += pose0.x;
  robot_pose.y += pose0.y;
  robot_pose.theta += pose0.theta;
  robot_pose.x += offset.x;
  robot_pose.y += offset.y;
  robot_pose.theta += offset.theta;
  robot_pose.theta = carmen_normalize_theta(robot_pose.theta);

  return robot_pose;
}

static void level_up() {

  mlm_point_t *point;

  printf("\nlevel_up()\n");

  point = mlm_get_point(&mlm);

  if (point == NULL || point->up_map == NULL)
    return;

  carmen_map_set_filename(point->up_map->map_name);
  mlm.current_map_index = point->up_map->map_num;
  set_robot_pose(transform_robot_pose(point->pose, point->up_pose));
}

static void level_down() {

  mlm_point_t *point;

  printf("\nlevel_down()\n");

  point = mlm_get_point(&mlm);

  if (point == NULL || point->down_map == NULL)
    return;

  carmen_map_set_filename(point->down_map->map_name);
  mlm.current_map_index = point->down_map->map_num;
  set_robot_pose(transform_robot_pose(point->pose, point->down_pose));
}

static void ipc_init() {

  carmen_localize_subscribe_globalpos_message(&global_pos, NULL,
					      CARMEN_SUBSCRIBE_LATEST);
}

int main(int argc, char** argv) {

  int c;

  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);

  ipc_init();

  if (argc != 2)
    carmen_die("Usage: %s <multi-level map filename>\n", argv[0]);

  read_mlm(&mlm, argv[1]);

  carmen_terminal_cbreak(0);

  while(1) {
    c = getchar();
    /*
    if (c == EOF)
      break;
    */
    if (c != 27)
      continue;
    c = getchar();
    /*
    if (c == EOF)
      break;
    */
    if (c != 91)
      continue;
    c = getchar();
    /*
    if (c == EOF)
      break;
    */
    switch(c) {
    case 65: level_up(); break;
    case 66: level_down();
    }
    sleep_ipc(0.01);
  }
   
  return 0;
}
