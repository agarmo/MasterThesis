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

#ifndef CARMEN_CEREBELLUM_COM_H
#define CARMEN_CEREBELLUM_COM_H

#ifdef __cplusplus
extern "C" {
#endif

  int carmen_cerebellum_connect_robot(char *dev);
  int carmen_cerebellum_reconnect_robot();
  int carmen_cerebellum_ac(int acc);
  int carmen_cerebellum_set_velocity(int command_vl, int command_vr);
  int carmen_cerebellum_get_state(int *left_tics, int *right_tics,
				  int *left_vel, int *right_vel);
  int carmen_cerebellum_get_voltage(double *batt_voltage);
  int carmen_cerebellum_get_temperatures(int *fault, int *temp_l, int *temp_r);
  
  int carmen_cerebellum_limp(void);
  int carmen_cerebellum_engage(void);
  int carmen_cerebellum_disconnect_robot(void);
  int carmen_cerebellum_heartbeat(void);
  
  int carmen_cerebellum_fire(void);
  int carmen_cerebellum_tilt(int value);
  int carmen_cerebellum_get_shroud(char *hit, char *where);
  int carmen_cerebellum_flash(void);


#ifdef __cplusplus
}
#endif

#endif
