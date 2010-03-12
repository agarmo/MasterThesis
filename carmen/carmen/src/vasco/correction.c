/*********************************************************
 *
 * This source code is part of the Carnegie Mellon Robot
 * Navigation Toolkit (CARMEN)
 *
 * CARMEN Copyright (c) 2002 Michael Montemerlo, Nicholas
 * Roy, Sebastian Thrun, Dirk Haehnel, Cyrill Stachniss,
 * and Jared Glover
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

#include "bd-utils.h"

void 
compute_forward_correction( RPOS2 pos, RPOS2 corr, RPOS2 *cpos )
{
  double corr_angle_2pi;

  cpos->o = pos.o - corr.o;
  for (; cpos->o <= -180.0;) cpos->o += 360.0;
  for (; cpos->o >   180.0;) cpos->o -= 360.0;
  
  corr_angle_2pi = corr.o * M_PI / 180.0;

  cpos->x = ((pos.x - corr.x) * cos(corr_angle_2pi))
    + ((pos.y - corr.y) * sin(corr_angle_2pi));

  cpos->y = ((pos.y - corr.y) * cos(corr_angle_2pi))
    - ((pos.x - corr.x) * sin(corr_angle_2pi));
}

void 
compute_backward_correction( RPOS2 cpos, RPOS2 corr, RPOS2 *pos )
{
  double corr_angle_2pi;
  
  pos->o = corr.o + cpos.o;
  for (; pos->o <= -180.0;) pos->o += 360.0;
  for (; pos->o  >  180.0;) pos->o -= 360.0;

  corr_angle_2pi = corr.o * M_PI / 180.0;

  pos->x = corr.x + (cpos.x * cos(corr_angle_2pi))
    - (cpos.y * sin(corr_angle_2pi));
  
  pos->y = corr.y + (corr.y * cos(corr_angle_2pi))
    + (cpos.x * sin(corr_angle_2pi));
}  

void 
compute_correction_parameters( RPOS2 pos, RPOS2 cpos, RPOS2 *corr )
{
  double corr_angle_2pi;

  corr->o = pos.o - cpos.o;
  for (; corr->o <= -180.0;) corr->o += 360.0;
  for (; corr->o  >  180.0;) corr->o -= 360.0;

  corr_angle_2pi = corr->o * M_PI / 180.0;

  corr->x = pos.x - (cpos.x * cos(corr_angle_2pi))
    + (cpos.y * sin(corr_angle_2pi));

  corr->y = pos.y - (cpos.y * cos(corr_angle_2pi))
    - (cpos.x * sin(corr_angle_2pi));
}

void 
update_correction_parameters( RPOS2 cpos, RPOS2 delta, RPOS2 *corr )
{
  RPOS2 pos, cpos2;

  compute_backward_correction( cpos, *corr, &pos );
  
  cpos2.x = cpos.x + delta.x;
  cpos2.y = cpos.y + delta.y;
  cpos2.o = cpos.o + delta.o;

  compute_correction_parameters( pos, cpos2, corr );
}

void
robot2map( RPOS2 pos, RPOS2 corr, RPOS2 *map )
{
  RPOS2 rpos = pos;
  rpos.o = 90.0 - pos.o;
  
  compute_forward_correction( rpos, corr, map );
  map->o = deg2rad( map->o );
}


void
map2robot( RPOS2 map, RPOS2 corr, RPOS2 *pos )
{
  RPOS2 map2 = map;
  map2.o = rad2deg( map.o );
  
  compute_backward_correction( map2, corr, pos );
  pos->o = 90.0 - pos->o;
}


void
computeCorr( RPOS2 pos, RPOS2 map, RPOS2 *corr )
{
  RPOS2 map2, pos2;
  
  map2   = map;
  map2.o = rad2deg( map.o );

  pos2   = pos;
  pos2.o = 90.0 - pos.o;

  compute_correction_parameters( pos2, map2, corr );
}









