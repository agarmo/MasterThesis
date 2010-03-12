#ifndef CONFIG_COMMON_B21_BASE_H
#define CONFIG_COMMON_B21_BASE_H

#define CONFIG_B21_Base
                                /* Architecture-based define.   */
 
#define ROBOT_RADIUS 267        /* maximum radius of the robot */

#define MM_TO_BASE_ENCODERS 27.3
                     /* multiplier to convert base shaft encoder */
                     /* to mm, so that  translateRelativePos()   */
                     /* can take millimeters on any robot        */  

#define BASE_POSITION_TO_MM 9.37 
                     /* multiplier to convert status report x and y to */
                     /* millimeters. Each position unit = 256 encoders */
                     /* so this is 256/MM_TO_BASE_ENCODERS. Note if you*/
                     /* define this as (256/MM_TO_BASE_ENCODERS)       */
                     /* be truncated to an int, which is not accurate  */

/* #undef BASE_ROTATE_BACKWARDS */
                     /* Some bases, specifically Ramona, rotate backwards*/
                     /* which totally screws up the base library unless  */
                     /* we fix it.  If your robot rotates backwards,     */
                     /* change the above undef to a define */


#define WARNING_VOLTAGE 44
#define PANIC_VOLTAGE   39  

#endif /* CONFIG_COMMON_B21_BASE_H */
