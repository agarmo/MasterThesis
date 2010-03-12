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

#ifdef __cplusplus
extern "C" {
#endif

void set_deadspot(int on_off, double size);

/* init_joystick() - Initializes the joystick.  If a joystick is connected,
   the function returns the number of axes in a, the number of buttons
   in b, and a return value of 0.  If no joystick is registered with the
   system, the function returns -1. */
int init_joystick(int *a, int *b);

/* get_joystick() - Gets the current state of the joystick (if available).
   If the joystick has changed state, vectors of axis and button values
   are returned in a and b.  The user is responsible for allocating the
   arrays passed to get_joystick. The function returns 0 if a new state
   is available, otherwise 0. 

   This function does some scaling of the joystick values.  It creates
   a dead zone, and it flips one axis. This was done specifically for 
   the Microsoft Sidewinder joystick. */
int get_joystick(int *a, int *b);

/* close_joystick() - Closes the connection to the joystick. */
void close_joystick(void);

#ifdef __cplusplus
}
#endif
