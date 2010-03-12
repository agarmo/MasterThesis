#ifndef CONFIG_COMMON_B21_IR_H
#define CONFIG_COMMON_B21_IR_H

#define CONFIG_B21_Ir
                                /* Architecture-based define.   */
 
#include "Enclosure.h"

#define IR_RADIUS ENCLOSURE_RADIUS
           /*  The radius of the irs is that of the enclosure.  */

/*  This is the standard B21 configuration of ir sensors.  You
    can override this in a specific robot's Robot.h file by
    redefining any of these variables.  */

/***** IR INFORMATION ***/
#define NUM_IR_ROWS      3	/* Bottom row has only 8 down looking */
#define IR_ROWS { 24, 24, 8  } /* Used to fill in irsInRow in Ir.h */
#define MAX_IRS_PER_ROW 24

#endif /* CONFIG_COMMON_B21_IR_H */
