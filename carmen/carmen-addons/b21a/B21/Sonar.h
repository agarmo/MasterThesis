#ifndef CONFIG_COMMON_B21_SONAR_H
#define CONFIG_COMMON_B21_SONAR_H

#define CONFIG_B21_Sonar
                     /* Architecture based define. */

#include "Enclosure.h"

#define SONAR_RADIUS ENCLOSURE_RADIUS
         /*  Sonars sit on the outside of the B21 enclosure.  */

#define NUM_SONARS 24
         /*  The standard number of sonars for the B21 is 24.
             This number can be redefined in Robot.h.  */

#endif /* CONFIG_COMMON_B21_SONAR_H */
