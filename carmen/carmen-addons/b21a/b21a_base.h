/*****************************************************************************
 * PROJECT: Xavier
 *
 * (c) Copyright 1993 Richard Goodwin & Joseph O'Sullivan. All rights reserved.
 *
 * FILE: base_interface.h
 *
 * ABSTRACT:
 *
 * Header file to be included to use Xavier motion functions.
 * Straightforward translation of the base interface commands to C.
 * Most of the comments are from the RWI documentation.
 *
 *****************************************************************************/


#ifndef RWIBASE_INTERFACE_H
#define RWIBASE_INTERFACE_H

#include <math.h>
#include "devUtils.h"
#include "handlers.h"

/* Set the state value, while "remembering" the previous value */
#define SET_ROBOT_STATE(which, value) \
{ previous_state.which = rwi_base.which; rwi_base.which = (value); }

/*****************************************************************************
 * Global Constants
 *****************************************************************************/

#define COUNTS_IN_360_DEGREES    1024
#define COUNTS_PER_DEGREE        (COUNTS_IN_360_DEGREES/360.0)
#define COUNTS_PER_RAD           (COUNTS_PER_DEGREE *  180.0 / M_PI)
#define STREAMING_MILLISECONDS   125
#define CHECKING_SECONDS         1
#define NUMBER_BUMP_PANELS       8
#define NUMBER_BUMP_SWITCHES     (2*NUMBER_BUMP_PANELS)
#define DEGREES_PER_BUMP_PANEL   (360.0/NUMBER_BUMP_PANELS)

#define COUNTS_PER_CM            548.20
#define COUNTS_PER_INCH          COUNTS_PER_CM*2.54
#define POS_COUNTS_PER_CM        (COUNTS_PER_CM/256)

/*
 *      EVENT                       Type of callback data
 *      *****                       *********************                 
 */

#define ANSWER_BATTERY_VOLTAGE   0  /* int */
#define ANSWER_ROTATE_POSITION   1  /* int */   /* busy */
#define STATUS_REPORT            2  /* int */   /* busy */
#define BUMP                     3  /* int */
#define NOT_BUMP                 4  /* int */
#define TRANS_STOPPED            5  /* nothing */
#define ROT_STOPPED              6  /* nothing */
#define ERROR_TRANSLATION        7
#define ERROR_ROTATION           8
#define BASE_STOPPED             9
#define BASE_REPORT_MISSED      10  /* nothing */
#define BASE_DISCONNECT         11  /* nothing */
#define BASE_RECONNECT          12  /* nothing */

#define BASE_NUMBER_EVENTS      13

#define MIN_TRANS_VEL_RESOLUTION (0x0100)
#define MIN_ROT_VEL_RESOLUTION   (0x0000)

typedef enum
{ Base_OK, Base_Resetting,
  Translate_Current_Error, Translate_Servo_Error, 
  Rotate_Current_Error, Rotate_Servo_Error, 
  Translate_Current_Clear, Rotate_Current_Clear
} RWIBASE_STATUS_ENUM;

/*****************************
 * Constants for status report
 *****************************/

#define SR_REPORT_DATA        BIT_ZERO
#define SR_CLOCK              BIT_ONE
#define SR_GEN_STATUS         BIT_TWO
#define SR_BUMP               BIT_THREE
#define SR_POS_X              BIT_FOUR
#define SR_POS_Y              BIT_FIVE
#define SR_TOP_HEADING        BIT_SIX
#define SR_BASE_HEADING       BIT_SEVEN

#define SR_TRANS_ERROR        BIT_EIGHT
#define SR_TRANS_VELOCITY     BIT_NINE
#define SR_TRANS_MOTOR_PULSE  BIT_TEN
#define SR_TRANS_STATUS       BIT_ELEVEN
#define SR_TRANS_M72_0        BIT_TWELVE
#define SR_TRANS_M72_1        BIT_THIRTEEN
#define SR_TRANS_M72_2        BIT_FOURTEEN
#define SR_TRANS_M72_3        BIT_FIFTEEN

#define SR_ROT_ERROR          BIT_SIXTEEN
#define SR_ROT_VELOCITY       BIT_SEVENTEEN
#define SR_ROT_MOTOR_PULSE    BIT_EIGHTEEN
#define SR_ROT_STATUS         BIT_NINTEEN
#define SR_ROT_M72_0          BIT_TWENTY
#define SR_ROT_M72_1          BIT_TWENTY_ONE
#define SR_ROT_M72_2          BIT_TWENTY_TWO
#define SR_ROT_M72_3          BIT_TWENTY_THREE

#define SR_BASE_AD_0_1        BIT_TWENTY_FOUR
#define SR_BASE_AD_2_3        BIT_TWENTY_FIVE
#define SR_BASE_AD_4_5        BIT_TWENTY_SIX
#define SR_BASE_AD_6_7        BIT_TWENTY_SEVEN
#define SR_RADIO_1            BIT_TWENTY_EIGHT
#define SR_RADIO_2            BIT_TWENTY_NINE
#define SR_RADIO_3            BIT_THIRTY
#define SR_RADIO_4            BIT_THIRTY_ONE

typedef unsigned long b8_integer;
typedef unsigned long b16_integer;
typedef unsigned long b32_integer;

typedef enum {
  POSITIVE, NEGATIVE, ZERO_DIR
  } directionStatus;

typedef struct _Base {
    int             time;                /* updated by status report:
					    base clock */
    double          update;              /* Time that pose was last updated 
					    (in seconds) */
    DEGREES_PER_SEC_2 rot_acceleration;  /* set by command */
    DEGREES_PER_SEC rot_current_speed;   /* updated by status_report */
    DEGREES_PER_SEC rot_set_speed;       /* set by command */
    CMS_PER_SEC_2   trans_acceleration;  /* set by command */
    CMS_PER_SEC     trans_current_speed; /* updated by status report */
    CMS_PER_SEC     trans_set_speed;     /* set by command */
    CMS             pos_x;               /* dead reckoning */
    CMS             pos_y;
    DEGREES         orientation;       
    unsigned long   rot_position;         /* in encoder counts */
    long            rot_reference;        /* in encoder counts */
    directionStatus trans_direction;      /* updated by status report */
    directionStatus trans_set_direction;  /* updated by command */
    directionStatus trans_encoder_direction; /* updated by status report */
    directionStatus rot_direction;        /* updated by status report */
    directionStatus rot_set_direction;    /* updated by command */
    directionStatus rot_encoder_direction; /* updated by status report */
    BOOLEAN         rot_moving;           /* computed from status report */
    BOOLEAN         trans_moving;         /* computed from status report */
    int             bumpers;              /* updated by status report */
    BOOLEAN         bump;
    BOOLEAN         emergency;            /* updated by status report */
    BOOLEAN         stopWhenBump;         /* updated by command */
    /* These two added by Reid (1/98) to get data for internal monitoring */
    unsigned int    trans_pulse;   /* pulse width of translation motor */
    unsigned int    rot_pulse;     /* pulse width of rotation motor */
} _Base, *Base;


typedef struct {
  DEV_PTR dev;
  /* put in here whatever is needed for starting up the real device.*/
}  BASE_TYPE, *BASE_PTR;

int  BASE_init(void);
void BASE_outputHnd(int fd, long chars_available);
void BASE_timeoutHnd(Pointer call_back, Pointer client_data);
void BASE_terminate(void);

void BASE_DisableStopWhenBump(void);
void BASE_EnableStopWhenBump(void);

void BASE_SetIntervalUpdates(long milliseconds);

void BASE_InstallHandler(Handler handler, int event, Pointer client_data,
			  BOOLEAN remove);
void BASE_RemoveHandler(Handler handler, int event);
void BASE_RemoveAllHandlers(int event);

/* Two files will be created with the following function:
   debug_file.out and debug_file.in, in which will be recorded the
   data going to or coming from the base */

void BASE_Debug(BOOLEAN debug_flag, char *debug_file);
void BASE_Kill(void);

void BASE_Halt(void);
void BASE_JoystickDisable(void);
void BASE_SetRotationReference(void);
void BASE_SetPos(CMS x, CMS y, DEGREES orientation);
void BASE_QueryBatteryVoltage(Handler handler, Pointer client_data);
void BASE_QueryRotatePosition(Handler handler, Pointer client_data);
void BASE_RotateAcceleration(DEGREES_PER_SEC_2 acceleration);
void BASE_RotateHalt(void);
void BASE_RotateVelocity (DEGREES_PER_SEC degrees_per_second);
void BASE_RotateClockwise(void);
void BASE_RotateAnticlockwise(void);
void BASE_Rotate (DEGREES position);
void BASE_RotateTo (DEGREES position);
void BASE_TranslateAcceleration (CMS_PER_SEC_2 cms_per_second_sqr);
void BASE_TranslateHalt(void);
void BASE_TranslateVelocity (CMS_PER_SEC cms_per_second);
void BASE_TranslateForward(void);
void BASE_TranslateBackward(void);
void BASE_Translate(CMS cms);
void BASE_HyperJump(CMS x, CMS y, DEGREES o);
void BASE_missed(Handler handler, Pointer client_data);
void BASE_resetPosition(void);
void BASE_reset(void);
void ProcessBump(int switches);
DEGREES bumpSwitchesToBaseAngle(unsigned int switches);

/*****************************************************************************
 * External variables
 *****************************************************************************/

extern BASE_TYPE base_device;
extern _Base rwi_base;
extern _Base previous_state;

extern CMS_PER_SEC_2 transAcceleration;
extern CMS transDeceleration;
extern int baseWatchDogTimeout;

#endif
