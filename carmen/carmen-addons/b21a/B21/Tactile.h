#ifndef CONFIG_COMMON_B21_TACTILE_H
#define CONFIG_COMMON_B21_TACTILE_H

#define CONFIG_B21_Tactile
                                /* Architecture-based define.   */

/*  This is the standard B21 configuration of tactile sensors.  You
    can override this in a specific robot's Robot.h file by
    redefining any of these variables.  */
 
/***** TACTILE INFORMATION ***/
#define NUM_TACTILE_ROWS      4
#define TACTILE_ROWS { 12, 12, 16, 16 }
#define MAX_TACTILES_PER_ROW 16

#endif /* CONFIG_COMMON_B21_TACTILE_H */
