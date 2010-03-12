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

#include <carmen/carmen.h>
#include <carmen/map_io.h>

struct mlm_map;

struct mlm_point {
  carmen_point_t pose;
  struct mlm_map *up_map;
  carmen_point_t up_pose;
  struct mlm_map *down_map;
  carmen_point_t down_pose;
};

struct mlm_map {
  char *map_name;
  int map_num;
  struct mlm_point *points;
  int num_points;
};

typedef struct mlm_point mlm_point_t;
typedef struct mlm_map mlm_map_t;

typedef struct {
  mlm_map_t *maps;
  int num_maps;
  int current_map_index;
} mlm_t;

void read_mlm(mlm_t *mlm, char *filename);

mlm_point_t *mlm_get_point(mlm_t *mlm);
