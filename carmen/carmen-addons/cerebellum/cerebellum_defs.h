
/********DATA FORMAT INFORMATION*****************/
// all velocities are 16-bit signed ints
// all accelerations are 16-bit unsigned ints
// all counts are 32-bit signed ints 
// voltage is an unsigned 16-bit int

// NOTE: DATA IS EXPECTED AND RETURNED LSB FIRST
// so for an int32 that is comprised of [c3 c2 c1 c0] (0xc3c2c1c0)
// send and receive it it as c0 c1 c2 c3

#define INIT1                     253 // The init commands are used in sequence(1,2,3)
#define INIT2                     252 // to initialize a link to a cerebellum.
#define INIT3                     251 // It will then blink green and start accepting other commands.

#define DEINIT                    250

#define SET_VELOCITIES            118 // 'v'(left_vel, right_vel) as 16-bit signed ints
#define SET_ACCELERATIONS          97 // 'a'(left_accel, right_accel) as 16-bit unsigned ints
#define ENABLE_VELOCITY_CONTROL   101 // 'e'()
#define DISABLE_VELOCITY_CONTROL  100 // 'd'()
#define GET_ODOMETRY              111 // 'o'()->(left_count, right_count, left_vel, right_vel)
#define GET_BATT_VOLTAGE           98 // 'b'()->(batt_voltage)
#define STOP                      115 // 's'()  [shortcut for set_velocities(0,0)]
#define KILL                      107 // 'k'()  [shortcut for disable_velocity_control]

#define FIRE                      102 // 'f'()  [shoots gun]
#define TILT_GUN                  116 // 't'(level) as a single character, range is restricted
#define GET_SHROUD                'S' // 'S' gets if we've been hit and
#define FLASH                     'F'

#define HEARTBEAT                 104 // 'h'() sends keepalive

#define PRINT_TEMP                 '-' // '-'() sends get temperature


/***********ACK VALUES***************/

#define ACKNOWLEDGE                 6 // if command acknowledged
#define NACKNOWLEDGE               21 // if garbled message

/*******NUMERIC CONVERSIO6NS******************/

#define MACH5_TICKS_METER           5  // ticks per meter, for converting odometry to distance
#define MACH5_TICKS_RADIAN          5  // ticks per radian, for converting odometry to theta
#define MACH5_WHEEL_BASE            5  // distance between wheels, in m
#define MACH5_VELOCITY_FACTOR       5  // converts m/s to cereb velocity units
#define MACH5_ACCELERATION_FACTOR   5  // converts m/s^2 to cereb acceleration units
#define MACH5_VOLTAGE_FACTOR        1  // converts cereb battery units to mV
