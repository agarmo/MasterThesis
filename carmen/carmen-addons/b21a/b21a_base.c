/*****************************************************************************
 * PROJECT: Xavier
 *
 * (c) Copyright 1993 Richard Goodwin & Joseph O'Sullivan. All rights reserved.
 *
 * FILE: base_interface.c
 *
 * ABSTRACT:
 *
 * Straightforward translation of the base interface commands to C.
 *
 *****************************************************************************/

#include <carmen/carmen.h>
#include "Common.h"
#include "b21a_base.h"
#include "abus_sensors.h"

#include "B21/Sonar.h"
#include "B21/Ir.h"
#include "B21/Tactile.h"
#include "B21/Base.h"
#include "B21/libmsp.h"
#include "B21/Rai.h"

#undef DEBUG
#undef DEBUG2

#define BASE_BUFFER_SIZE (4*1024)

/*****************************************************************************
 * Global Constants
 *****************************************************************************/

static struct timeval COMMAND_TIMEOUT = {10,0};

/*****************************************************************************
 * Global Types
 *****************************************************************************/

typedef enum { TERR=0, TC=1, RERR=2, RCC=4, BHI=8, BLO=16 } BASE_ERROR_TYPE;

#define ALL_BASE_ERRORS (TERR | RCC | RERR | BHI | BLO)

/*****************************************************************************
 * Global variables
 *****************************************************************************/

BASE_TYPE base_device;

/* external variables */
_Base rwi_base;
_Base previous_state;
_Base tmp_state;

/* private variables */
#define STATUS_PREFIX_LENGTH 3
static b32_integer status_data = 0;
static int last_report_line = 0;
static char *report_data[] = {
  "SRD", "CLK", "GSF", "BBS", "BPX", "BPY", "TPH", "BRH",
  "TPE", "TVE", "TPW", "TSF", "TC0", "TC1", "TC2", "TC3",
  "RPE", "RVE", "RPW", "RSF", "RC0", "RC1", "RC2", "RC3",
  "A01", "A23", "A45", "A67", "RX1", "RX2", "RX3", "RX4"
};

static FILE *debug_file_in = NULL;

static HandlerList rwibase_handlers = NULL;

static BOOLEAN BASE_waiting_for_report = FALSE;

CMS_PER_SEC_2 transAcceleration;
CMS transDeceleration;
DEGREES_PER_SEC_2 rotAcceleration = 85.0;
DEGREES_PER_SEC_2 rotDeceleration = 85.0;
char *baseTTY = NULL;
int baseBAUD = 13;
int baseWatchDogTimeout=0;

/* Forward declaration of private procedures */
static void InitBase(void);
static BOOLEAN InitDevice(void);
static void JoystickDisable(void);
static void HalfDuplex(void);
static void TorqueLimit (b8_integer limit);
static void InitializeTorsoPosition(void);
static void DoDeadReckoning(void);
void ProcessBump(int switches);
static void FireEventHandlers(void);
static int WriteCommand (char *buffer, int nbytes, unsigned long l);
static void BaseParseReturn(char *start);
static void BASE_report_check(Pointer call_back, Pointer client_data);
static void disconnectBASE(DEV_PTR ignore);
static void reconnectBASE(DEV_PTR ignore);
static void BASE_StatusReportData (b32_integer items);
static void setErrorDelay(long delay);

DEGREES _0_to_360(DEGREES deg)
{
  for(;;) {
    if (deg >= 360.0)
      deg -= 360.0;
    else if (deg < 0.0)
      deg += 360.0;
    else
      return (deg);
  }
}

static double update_time(void)
{
  struct timeval now;
  
  gettimeofday(&now, NULL);
  return now.tv_sec + now.tv_usec/1000000.0;
}

/********************/
/* PUBLIC FUNCTIONS */
/********************/

void BASE_reset(void)
{
  struct timeval poll_interval;

  JoystickDisable();
  HalfDuplex();
  setErrorDelay(128); /* number of 256Hz cycles between error and shutdown. */
  TorqueLimit(100);
  BASE_RotateAcceleration(rotAcceleration);
  BASE_TranslateAcceleration(transAcceleration);
  //  BASE_RotateVelocity(init_rot_speed);
  //  BASE_TranslateVelocity(init_trans_speed);
  BASE_StatusReportData(SR_POS_X |
			SR_POS_Y |
			SR_TOP_HEADING |
			SR_BASE_HEADING |
			SR_TRANS_VELOCITY |
			SR_TRANS_STATUS |
			SR_ROT_VELOCITY |
			SR_ROT_STATUS |
			SR_TRANS_MOTOR_PULSE |
			SR_ROT_MOTOR_PULSE);

  /* commands in the streaming data from base */
  BASE_SetIntervalUpdates(STREAMING_MILLISECONDS);
  poll_interval.tv_usec = 0;
  poll_interval.tv_sec = CHECKING_SECONDS;
  devStartPolling(base_device.dev, &poll_interval, BASE_report_check, NULL);
  BASE_InstallHandler((Handler)BASE_SetRotationReference, STATUS_REPORT,
		      (Pointer) NULL, TRUE);
}

BOOLEAN BASE_init(void)
{
  devInit();
  rwibase_handlers = CreateHandlerList(BASE_NUMBER_EVENTS);
  if(InitDevice()) {
    InitBase();
    BASE_Halt();
    //    BASE_TranslateVelocity(init_trans_speed);
    //    BASE_RotateVelocity(init_rot_speed);
    BASE_reset();
    BASE_EnableStopWhenBump();
    return TRUE;
  }
  else
    return FALSE;
}

void BASE_JoystickDisable(void)
{
  JoystickDisable();
}

void BASE_SetRotationReference(void)
{
  if ((rwi_base.rot_reference == -1) ||
      ((rwi_base.rot_reference == 0) && rwi_base.rot_position == 0))
      InitializeTorsoPosition();
}

void BASE_SetPos(CMS x, CMS y, DEGREES orientation)
{
  rwi_base.pos_x = x;
  rwi_base.pos_y = y;
  rwi_base.orientation = orientation;

  previous_state.pos_x = x;
  previous_state.pos_y = y;
  previous_state.orientation = orientation;
}

void BASE_outputHnd(int fd, long chars_available)
{
  static char buffer[BASE_BUFFER_SIZE+1];
  static char *startPos = buffer; /* position to start parsing from */
  static char *endPos = buffer; /* position to add more characters */
  
  int roomLeft;
  char *lineEnd, *lineStart;
  int numRead = 0;

  /* figure out how much room is left in the buffer */
  roomLeft = MIN(chars_available,(BASE_BUFFER_SIZE -
				  (endPos - startPos)));
  lineEnd = (char *) strpbrk(startPos,"\n\r");

  if (startPos == endPos)
    {
      startPos = endPos = buffer;
      bzero(buffer, BASE_BUFFER_SIZE+1);
    }

  if ((roomLeft > 0) && (lineEnd == NULL))
    {
      /* read in the output. */
      numRead = devReadN(fd, endPos, roomLeft);
      endPos += numRead;
      if (numRead == 0)
	{ /* handle error here. The port is already closed. */
	}
    }

  /* see if we have a \n  or null character */
  lineEnd = (char *) strpbrk(startPos,"\n\r");
  while ((lineEnd != NULL) && (startPos < endPos))
    {/* found a string, pass it to the parsing routines. */
      *lineEnd = '\0';
      lineStart = startPos;
      startPos = lineEnd+1;
      BaseParseReturn(lineStart);
      lineEnd = (char *) strpbrk(startPos,"\n\r");
      /*	  fprintf(stderr,"Uncomsummed %d\n",(endPos - startPos));*/
    }
  /* Fix up the buffer. Throw out any consumed lines.*/
  if (startPos >= endPos)
    { /* parsed the whole thing, just clean it all up */
      bzero(buffer, BASE_BUFFER_SIZE+1);
      startPos = endPos = buffer;
    }
  else if (startPos != buffer)
    { /* slide it back and wait for more characters */
      bcopy(startPos, buffer, (endPos - startPos));
      endPos = buffer + (endPos - startPos);
      startPos = buffer;
      bzero(endPos,BASE_BUFFER_SIZE - (endPos - startPos));
      /*	  fprintf(stderr,"Remains %d\n",(endPos - startPos));*/
    }
}


/*	Function Name: BASE_timeoutHnd
 *	Arguments:
 *	Description:   Called when there is a timeout
 *	Returns:
 */

void BASE_timeoutHnd(Pointer call_back, Pointer client_data)
{
  /* stub : will be called when there is a timeout.
   */
}


/*	Function Name: BASE_terminate
 *	Arguments:    
 *	Description:   Close the base device
 *	Returns:
 */

void BASE_terminate(void)
{
  devDisconnectDev(base_device.dev,FALSE,FALSE);
}


/*	Function Name: BASE_Debug
 *	Arguments:     debug_flag -- boolean
 *	Description:   This function set or reset the base debug, which
 *                     will printout the characters send and received by
 *                     the base interface.
 *	Returns:
 */

void BASE_Debug (BOOLEAN debug_flag, char *file_name)
{
  char file[30];

  devSetDebug(base_device.dev,debug_flag);
  if (debug_flag &&
      file_name != NULL) {
    strcpy (file, file_name);
    strcat (file, ".out");
    devSetDebugFile(base_device.dev,fopen(file, "w"));
    strcpy (file, file_name);
    strcat (file, ".in");
    debug_file_in = fopen(file, "w");
  }
}



/*	Function Name: BASE_StatusReportData
 *	Arguments:     items -- items in the status report. 32 bit,
 *                              with an 1 in the bits corresponding to
 *                              the data in which we are interested.
 *	Description:   Setup the data for periodic status reports
 *	Returns:     
 */

static void BASE_StatusReportData(b32_integer items)
{
  status_data = items;
  /* set the last item in the report */
  for (last_report_line=31; !(status_data & (1<<last_report_line));
       last_report_line--);
  last_report_line++; /* want 1 more than the last index. */
  WriteCommand ("SD", 32, items);
}


/*	Function Name: BASE_SetIntervalUpdates
 *	Arguments:     milliseconds -- rate of incoming status reports
 *	Description:   Setup the period for the status report
 *	Returns:
 */

void BASE_SetIntervalUpdates(long milliseconds)
{
  long rwi_period;

  rwi_period = milliseconds * 256 / 1000;
  WriteCommand("SP", 16, rwi_period);
}



/*	Function Name:
 *	Arguments:
 *	Description:
 *	Returns:
 */

void  BASE_InstallHandler(Handler handler, int event, Pointer client_data,
			  BOOLEAN remove)
{
  InstallHandler(rwibase_handlers, handler, event, client_data, remove);
}

/*	Function Name:
 *	Arguments:
 *	Description:
 *	Returns:
 */

void  BASE_RemoveHandler(Handler handler, int event)
{
  RemoveHandler(rwibase_handlers, handler, event);
}


void  BASE_RemoveAllHandlers(int event)
{
  RemoveAllHandlers(rwibase_handlers, event);
}

/***********/
/* QUERIES */
/***********/

/*	Function Name: BASE_QueryBatteryVoltage
 *	Arguments:   
 *	Description:   This query returns the battery voltage
 *                     running to the base control computer and the top
 *                     plate connector.
 *	Returns:       Nothing
 */

void BASE_QueryBatteryVoltage(Handler handler, Pointer client_data)
{
  BASE_InstallHandler(handler, ANSWER_BATTERY_VOLTAGE, client_data, TRUE);
  WriteCommand("BV", 0, 0);
}


/*	Function Name: BASE_QueryRotatePosition
 *	Arguments:   
 *	Description:   This query returns the torso orientation
 *	Returns:       Nothing
 */

void BASE_QueryRotatePosition(Handler handler, Pointer client_data)
{
  BASE_InstallHandler(handler, ANSWER_ROTATE_POSITION,
		      client_data, TRUE);
  WriteCommand("RW", 0, 0);
}

/******************/
/* MISC. COMMANDS */
/******************/


/*	Function Name: BASE_Kill
 *	Arguments:   
 *	Base manual:   This command causes both motors to go limp. This
 *                     command does not observe acceleration and
 *                     velocity parameters.
 *	Returns:       Nothing
 */

void BASE_Kill(void)
{
  WriteCommand ("KI", 0, 0);
}


/*	Function Name: BASE_DisableStopWhenBump
 *	Arguments:
 *	Description:   Disable the emergency stop procedure
 *	Returns:
 */

void BASE_DisableStopWhenBump(void)
{
  SET_ROBOT_STATE(emergency, FALSE);
  SET_ROBOT_STATE(stopWhenBump, FALSE);
  WriteCommand("BE", 32, 0xFFFFFFFF);
}


/*	Function Name: BASE_EnableStopWhenBump
 *	Arguments:
 *	Description:   Enable the emergency stop procedure
 *	Returns:
 */

void BASE_EnableStopWhenBump(void)
{
  SET_ROBOT_STATE(stopWhenBump, TRUE);
  WriteCommand("BE", 32, 0);
}

/*********************/
/* ROTATION COMMANDS */
/*********************/



/*	Function Name: BASE_RotateAcceleration
 *	Arguments:     acceleration -- double, in degrees/second squared
 *	Base manual:   This command sets the internal rotational
 *                     acceleration variable.  The parameter is in
 *                     encoder counts per second squared. This command
 *                     may affect motions already in progress, but it
 *                     alone does not cause the base to move.
 *	Returns:       Nothing
 */

void BASE_RotateAcceleration(DEGREES_PER_SEC_2 acceleration)
{
  if (rwi_base.rot_acceleration != acceleration) {
    SET_ROBOT_STATE(rot_acceleration, acceleration);
    WriteCommand ("RA", 16, acceleration * COUNTS_PER_DEGREE);
  }
}



/*	Function Name: BASE_Halt
 *	Arguments:
 *	Description:   Halt the base.
 *	Base manual:   This command halts both translation and rotation.
 *	Returns:       Nothing
 */

void BASE_Halt(void)
{
  BASE_TranslateAcceleration(transDeceleration);
  BASE_TranslateHalt();
  BASE_RotateAcceleration(rotDeceleration);
  BASE_RotateHalt();
  BASE_TranslateAcceleration(transDeceleration);
  BASE_RotateAcceleration(rotDeceleration);
}



/*	Function Name: BASE_RotateHalt
 *	Arguments:
 *	Description:   Check the current velocity and only send
 *                     the command if needed
 *	Base manual:   This command decelerates and stops the rotation
 *                     motor. The rotational velocity and acceleration
 *                     values do not change.
 *	Returns:       Nothing
 */

void BASE_RotateHalt(void)
{
  SET_ROBOT_STATE(rot_set_direction, ZERO_DIR);
  WriteCommand("RH", 0, 0);
}


/*	Function Name: BASE_RotateVelocity
 *	Arguments:     velocity -- double, in degrees/sec
 *	Description:   See RotateVelocity
 *	Base manual:   This command sets the internal rotational
 *                     velocity variable. The velocity value is in
 *                     encoder counts per second. The rotational
 *                     velocity will never exceed this programmed
 *                     velocity unless the velocity is already over
 *                     it, in which case, the rotation will decelerate
 *                     until it is at the new velocity. This command
 *                     may affect motions already in progress, but it
 *                     alone does not cause the base to move.
 *	Returns:       Nothing
 */

void BASE_RotateVelocity(DEGREES velocity)
{
  if (velocity >=  0.0) {
    if (velocity != rwi_base.rot_set_speed) {
      SET_ROBOT_STATE(rot_set_speed, (rwi_base.rot_set_direction == POSITIVE
				      ? velocity : -velocity));
      WriteCommand("RV", 16, velocity * COUNTS_PER_DEGREE);
    }
  }
  else
    fprintf(stderr,
	    "Error in BASE_RotateVelocity>> velocity must be positive\n");
}



/*	Function Name: BASE_RotateClockwise
 *	Arguments:   
 *	Description:   Check the current state of the base and only
 *                     send the command if needed
 *	Base manual:   This command cause the base to start rotating at
 *                     the programmed acceleration and velocity values
 *                     in the specified direction.  Positive stands
 *                     for clockwise and negative for anticlockwise.
 *                     The base will continue rotating until a new
 *                     rotation command is given.
 *	Returns:       Nothing
 */

void BASE_RotateClockwise(void)
{
  if ((rwi_base.rot_current_speed == 0.0 ||
       rwi_base.rot_direction != POSITIVE) &&
      rwi_base.rot_set_speed != 0.0)
    {
      BASE_RotateAcceleration(rotAcceleration);
    
      SET_ROBOT_STATE(rot_set_direction, POSITIVE);
    
      SET_ROBOT_STATE(rot_set_speed, FABS(rwi_base.rot_set_speed));
    
      WriteCommand("R+", 0, 0);
    }
}



/*	Function Name: BASE_RotateAnticlockwise
 *	Arguments:   
 *	Description:   Check the current state of the base and only
 *                     send the command if needed
 *	Base manual:   This command cause the base to start rotating at
 *                     the programmed acceleration and velocity values
 *                     in the specified direction.  Positive stands
 *                     for clockwise and negative for anticlockwise.
 *                     The base will continue rotating until a new
 *                     rotation command is given.
 *	Returns:       Nothing
 */

void BASE_RotateAnticlockwise(void)
{
  if ((rwi_base.rot_current_speed == 0.0 ||
       rwi_base.rot_direction != NEGATIVE) &&
      rwi_base.rot_set_speed != 0.0)
    {
      BASE_RotateAcceleration(rotAcceleration);
    
      SET_ROBOT_STATE(rot_set_direction, NEGATIVE);
    
      SET_ROBOT_STATE(rot_set_speed, -FABS(rwi_base.rot_set_speed));
    
      WriteCommand("R-", 0, 0);
    }
}



/*	Function Name: BASE_Rotate
 *	Arguments:     position -- double with number of degrees
 *	Description:   Position may be positive or negative. See
 *                     RotateRelativePositive and RotateRelativeNegative.
 *	Base manual:   This command cause the base to rotate to a new
 *                     position given as an offset of its present
 *                     position using the current acceleration and
 *                     velocity values. The parameter units are
 *                     encoder counts.
 *	Returns:       Nothing
 */

void BASE_Rotate(DEGREES position)
{
  if (position > 0.0) {
    SET_ROBOT_STATE(rot_set_direction, POSITIVE);
    WriteCommand("R>", 32, (int) (position * COUNTS_PER_DEGREE));
  }
  else {
    SET_ROBOT_STATE(rot_set_direction, NEGATIVE);
    WriteCommand("R<", 32, (int) (-position * COUNTS_PER_DEGREE));
  }

  SET_ROBOT_STATE(rot_set_speed, (rwi_base.rot_set_direction == POSITIVE
				  ? FABS(rwi_base.rot_set_speed)
				  : -FABS(rwi_base.rot_set_speed)+0.0));
}


/*	Function Name: BASE_RotateTo
 *	Arguments:     position -- double with the degrees (0-north, 90-east)
 *	Description:   Rotate to the absolute position. For this command works
 *                     properly it is needed the information returned by the
 *                     status report.
 *	Base manual:   This command causes the base to rotate to a
 *                     specified absolute encoder position. This
 *                     rotation occurs at the programmed velocity and
 *                     acceleration. When switched on, the rotation
 *                     encoder's position is set to 0 and thereafter
 *                     tracks the cumulative rotational motions. Refer
 *                     to rotate relative position for more convenient
 *                     rotate commands.
 *	Returns:       Nothing
 */

void BASE_RotateTo(DEGREES position)
{
  int wanted_direction_in_counts;
  unsigned long increment;

  wanted_direction_in_counts = position * COUNTS_PER_DEGREE;
  increment = (wanted_direction_in_counts - rwi_base.rot_position) %
    COUNTS_IN_360_DEGREES;
  WriteCommand ("RP", 32, rwi_base.rot_position + increment + 0x80000000);
}




/************************/
/* TRANSLATION COMMANDS */
/************************/


/*	Function Name: BASE_TranslateAcceleration
 *	Arguments:     acceleration -- double, in cm/second squared
 *	Base manual:   This command sets the internal translational
 *                     acceleration variable.  The parameter is in
 *                     encoder counts per second squared. This command
 *                     may affect motions already in progress, but it
 *                     alone does not cause the base to move.
 *	Returns:       Nothing
 */

void BASE_TranslateAcceleration(CMS_PER_SEC_2 acceleration)
{
  if (rwi_base.trans_acceleration != acceleration) {
    SET_ROBOT_STATE(trans_acceleration, acceleration);
    WriteCommand("TA", 16, acceleration * COUNTS_PER_CM);
  }
}



/*	Function Name: BASE_TranslateHalt
 *	Arguments:
 *	Description:   Check the current state of the base and only
 *                     send the command if needed
 *	Base manual:   This command decelerates and stops the translation
 *                     motor. The translational velocity and acceleration
 *                     values do not change.
 *	Returns:       Nothing
 */

void BASE_TranslateHalt(void)
{
  SET_ROBOT_STATE(trans_set_direction, ZERO_DIR);
  WriteCommand ("TH", 0, 0);
}



/*	Function Name: BASE_TranslateVelocity
 *	Arguments:     velocity -- double, in cm/sec
 *	Base manual:   This command sets the internal translational
 *                     velocity variable. The velocity value is in
 *                     encoder counts per second. The translational
 *                     velocity will never exceed this programmed
 *                     velocity unless the velocity is already over
 *                     it, in which case, the translation will decelerate
 *                     until it is at the new velocity. This command
 *                     may affect motions already in progress, but it
 *                     alone does not cause the base to move.
 *	Returns:       Nothing
 */

void BASE_TranslateVelocity(CMS_PER_SEC velocity)
{
  if (velocity >= 0.0) {
    if (velocity != rwi_base.trans_set_speed) {
      SET_ROBOT_STATE(trans_set_speed, 
		      (rwi_base.trans_set_direction == POSITIVE
		       ? velocity : -velocity));
      WriteCommand("TV", 16, velocity * COUNTS_PER_CM);
    }
  }
  else
    fprintf(stderr,
	    "Error in BASE_TranslateVelocity>> velocity must be positive\n");
}


/*	Function Name: BASE_TranslateForward
 *	Arguments:   
 *	Description:   Check the current state of the base and only
 *                     send the command if needed
 *	Description:   This command cause the base to start translating at
 *                     the programmed acceleration and velocity values
 *                     in the specified direction.  Positive stands
 *                     for forward and negative for backward.
 *                     The base will continue translating until a new
 *                     translation command is given.
 *	Returns:       Nothing
 */

void BASE_TranslateForward(void)
{
  if (rwi_base.emergency)
    return;
  if (rwi_base.trans_current_speed == 0 ||
      (rwi_base.trans_direction != POSITIVE &&
       rwi_base.trans_set_speed != 0.0))
    {
      BASE_TranslateAcceleration(transAcceleration);

      SET_ROBOT_STATE(trans_set_direction, POSITIVE);
    
      SET_ROBOT_STATE(trans_set_speed, FABS(rwi_base.trans_set_speed));
    
      WriteCommand("T+", 0, 0);
    }
}


/*	Function Name: BASE_TranslateBackward
 *	Arguments:   
 *	Description:   Check the current state of the base and only
 *                     send the command if needed
 *	Base manual:   This command cause the base to start translating at
 *                     the programmed acceleration and velocity values
 *                     in the specified direction.  Positive stands
 *                     for forward and negative for backward.
 *                     The base will continue translating until a new
 *                     translation command is given.
 *	Returns:       Nothing
 */

void BASE_TranslateBackward(void)
{
  if (rwi_base.emergency)
    return;
  if (rwi_base.trans_current_speed == 0 ||
      (rwi_base.trans_direction != NEGATIVE &&
       rwi_base.trans_set_speed != 0.0)) {
  
    SET_ROBOT_STATE(trans_set_direction, NEGATIVE);
  
    SET_ROBOT_STATE(trans_set_speed, -FABS(rwi_base.trans_set_speed)+0.0);
  
    rwi_base.trans_set_direction = NEGATIVE;
    WriteCommand("T-", 0, 0);
  }
}


/*	Function Name: BASE_Translate
 *	Arguments:     position -- double with number of cm
 *	Description:   Position may be positive or negative. See
 *                     TranslateRelativePositive and TranslateRelativeNegative.
 *	Base manual:   This command cause the base to translate to a new
 *                     position given as an offset of its present
 *                     position using the current acceleration and
 *                     velocity values. The parameter units are
 *                     encoder counts.
 *	Returns:       Nothing
 */

void BASE_Translate(CMS position)
{
  if (rwi_base.emergency)
    return;

  if (position > 0.0) {
    SET_ROBOT_STATE(trans_set_direction, POSITIVE);
    WriteCommand("T>", 32, position * COUNTS_PER_CM);
  }
  else {
    SET_ROBOT_STATE(trans_set_direction, NEGATIVE);
    WriteCommand("T<", 32, -position * COUNTS_PER_CM);
  }
  SET_ROBOT_STATE(trans_set_speed, (rwi_base.trans_set_direction == POSITIVE
				    ? FABS(rwi_base.trans_set_speed)
				    : -FABS(rwi_base.trans_set_speed)+0.0));
}

void BASE_HyperJump(CMS x, CMS y, DEGREES o)
{
  if (devGetUseSimulator(base_device.dev)) {
    char buffer[55];
  
    sprintf(buffer, "HY %f %f %f\r", x,y,o);
  
    /* set the timeout */
    devSetTimer(base_device.dev, &COMMAND_TIMEOUT, BASE_timeoutHnd, NULL);
  
    devWriteN(devGetFd(base_device.dev), buffer,strlen(buffer));
  }
}


/*********************/
/* PRIVATE FUNCTIONS */
/*********************/


static void InitBase(void)
{
  rwi_base.time               = previous_state.time               = 0;
  rwi_base.update	      = previous_state.update		= update_time();
  rwi_base.rot_acceleration   = previous_state.rot_acceleration   = 0.0;
  rwi_base.rot_position       = previous_state.rot_position       = 0;
  rwi_base.rot_reference      = previous_state.rot_reference      = -1;
  rwi_base.trans_acceleration = previous_state.trans_acceleration = 0.0;
  rwi_base.bumpers            = previous_state.bumpers            = 0xFFFF;
  rwi_base.bump               = previous_state.bump               = FALSE;
  rwi_base.emergency          = previous_state.emergency          = FALSE;
  rwi_base.stopWhenBump       = previous_state.stopWhenBump       = TRUE;
  rwi_base.pos_x              = previous_state.pos_x              = 0.0;
  rwi_base.pos_y              = previous_state.pos_y              = 0.0;
  rwi_base.orientation        = previous_state.orientation        = 0.0;
  rwi_base.trans_pulse        = previous_state.trans_pulse        = 0;
  rwi_base.rot_pulse          = previous_state.rot_pulse          = 0;

  rwi_base.rot_moving         = previous_state.rot_moving         = FALSE;
  rwi_base.trans_moving       = previous_state.trans_moving       = FALSE;

  rwi_base.rot_set_direction   = previous_state.rot_set_direction   = ZERO_DIR;
  rwi_base.trans_set_direction = previous_state.trans_set_direction = ZERO_DIR;

  rwi_base.rot_encoder_direction = 
    previous_state.rot_encoder_direction = ZERO_DIR;
  rwi_base.trans_encoder_direction = 
    previous_state.trans_encoder_direction = ZERO_DIR;
}


static BOOLEAN InitDevice(void)
{
  base_device.dev = devCreateDev("rwibase",
				 DEV_LISTENING, (LISTENING | TALKING),
				 DEV_DISCONNECTHND, disconnectBASE,
				 DEV_RECONNECTHND, reconnectBASE,
				 DEV_TTY_PORT, baseTTY,
				 DEV_OUTPUTHND, BASE_outputHnd,
				 DEV_BAUD_CODE, baseBAUD,
				 NULL);

  /* code to open the real device here. */
  devConnectTotty(base_device.dev);
  return devIsConnected(base_device.dev);
}

/*	Function Name: ReceivedStatusReport
 *	Arguments:   
 *	Description:   Function done each time that a new status
 *                     report arrives from the base
 *	Returns:
 */

static void ReceivedStatusReport(BOOLEAN report_corrupted)
{
  if (!report_corrupted) {
  
    /* clear the waiting flag */
  
    BASE_waiting_for_report=FALSE;
  
    /* store the previous state */
  
    previous_state = rwi_base;
  
    /* commit the data in the temporal variable */
  
    rwi_base.time = tmp_state.time;
    rwi_base.update = update_time();
    rwi_base.trans_direction = tmp_state.trans_direction;
    /* No longer set: rwi_base.rot_position = tmp_state.rot_position; */
    rwi_base.rot_direction = tmp_state.rot_direction;
    rwi_base.trans_current_speed = tmp_state.trans_current_speed;
    rwi_base.rot_current_speed = tmp_state.rot_current_speed;
  
    rwi_base.trans_encoder_direction = tmp_state.trans_encoder_direction;
    rwi_base.rot_encoder_direction = tmp_state.rot_encoder_direction;
    
    rwi_base.trans_pulse = tmp_state.trans_pulse;
    rwi_base.rot_pulse = tmp_state.rot_pulse;
  
    DoDeadReckoning();
    FireEventHandlers();
  
    FireHandler(rwibase_handlers, STATUS_REPORT, (Pointer) NULL);
  }
  else
    fprintf(stderr, ":Bad status report:\n");

}

static void DoDeadReckoning(void)
{
  if (rwi_base.rot_encoder_direction != POSITIVE) 
    rwi_base.rot_current_speed = -rwi_base.rot_current_speed;
  if (rwi_base.trans_encoder_direction != POSITIVE)
    rwi_base.trans_current_speed = -rwi_base.trans_current_speed;
  /* Is the robot is moving or stopped? */
  rwi_base.rot_moving =  (rwi_base.rot_current_speed != 0.0);
  rwi_base.trans_moving = (rwi_base.trans_current_speed != 0.0);
}


/*	Function Name: BASE_resetPosition
 *	Arguments:
 *	Description:
 *	Returns:
 */

void BASE_resetPosition(void)
{
  rwi_base.pos_x = 0.0;
  rwi_base.pos_y = 0.0;
  rwi_base.orientation = 0.0;
}

/*	Function Name: FireEventHandlers
 *	Arguments:   
 *	Description:   Look at the current and previous base state, sending
 *                     all the events that have occured
 *	Returns:
 */

static void FireEventHandlers(void)
{
  if (previous_state.trans_moving && !rwi_base.trans_moving) {
    FireHandler(rwibase_handlers, TRANS_STOPPED, (Pointer) TRANS_STOPPED);
    
  }
  previous_state.trans_moving = rwi_base.trans_moving;

  if (previous_state.rot_moving && !rwi_base.rot_moving) {
    FireHandler(rwibase_handlers, ROT_STOPPED, (Pointer) ROT_STOPPED);
  }
  previous_state.rot_moving = rwi_base.rot_moving;
}

/* find angle to the panel/panels that was/were bumped. */
DEGREES bumpSwitchesToBaseAngle(unsigned int switches)
{
  int i;
  float location;

  for( i=0; (switches & 1<<i); i++);
  location = ((i/2) % NUMBER_BUMP_PANELS);
  /* Change the location if the two ends of the same panel were bumped 
     (for all panels, the right side of the panel is an odd number,
     and the left side of the same panel is (i+1)%NUMBER_BUMP_SWITCHES */
  if ((i & 1) && !(switches & 1<<((i+1)%NUMBER_BUMP_SWITCHES))) {
    location += 0.5;
  }
  return (double)(DEGREES_PER_BUMP_PANEL*(location + 0.5));
}

void ProcessBump(int switches)
{
  DEGREES baseAngle, torsoAngle;

  rwi_base.emergency = (switches != 0xFFFF);
  rwi_base.bumpers = switches;

  if (rwi_base.emergency) {
    /* DON'T HALT HERE -- IT SCREWS UP THE BUMP RECOVERY PROCEDURE */

    /* "baseAngle" is with respect to the base; 
       Need to convert to the angle with respect to the torso */
    baseAngle = bumpSwitchesToBaseAngle(switches);
    torsoAngle = _0_to_360(baseAngle + 
			   rwi_base.rot_reference/COUNTS_PER_DEGREE +
			   rwi_base.orientation);
    //    fprintf(stderr, "*** Bump Detected *** Angle: %.2f (Base: %.2f)\n", 
    //	    _0_to_360(torsoAngle - rwi_base.orientation), baseAngle);
    FireHandler(rwibase_handlers, BUMP, (Pointer) &torsoAngle);
  } 
  else
    FireHandler(rwibase_handlers, NOT_BUMP, (Pointer) NULL);
}

/*	Function Name: TorqueLimit
 *	Arguments:     limit - limit of translate torque.
 *	Description:   The translate torque is by default really strong,
 *                     so the robot even can go throgh walls!.
 *                     A good limit is 20.
 *	Returns:     
 */

static void TorqueLimit(b8_integer limit)
{
  WriteCommand("TT", 8, limit);
}


/*	Function Name:
 *	Arguments:
 *	Description:
 *	Returns:
 */

static void JoystickDisable(void)
{
  WriteCommand("JD", 8, 1);
}



/*	Function Name:
 *	Arguments:
 *	Description:
 *	Returns:
 */

static void HalfDuplex(void)
{
  WriteCommand("HD", 0, 0);
}

/*	Function Name: InitializeTorsoPosition
 *	Arguments:
 *	Description:
 *	Returns:
 */

static void InitializeTorsoPosition(void)
{
  BASE_TranslateHalt();
  WriteCommand("IX", 0, 0);
}

/*	Function Name: InitializeTorsoPosition
 *	Arguments:
 *	Description:
 *	Returns:
 */

static void BASE_resetWatchDogTimer(int msec)
{
  WriteCommand("WD", 8, (msec*256)/1000);
}

/*	Function Name: setErrorDelay
 *	Arguments:
 *	Description:
 *	Returns:
 */

static void setErrorDelay(long delay)
{
  WriteCommand("ED", 16, delay);
}

/*	Function Name: errorAck
 *	Arguments:
 *	Description:
 *	Returns:
 */

static void errorAck(long errorMask)
{
  errorMask = errorMask ^ ALL_BASE_ERRORS;
  fprintf(stderr, "Acknowledging base error: %ld\n",errorMask);
  WriteCommand("EA", 16, errorMask);
}

/*	Function Name:
 *	Arguments:
 *	Description:
 *	Returns:
 */

static int WriteCommand(char *command, int n_bits, unsigned long l)
{
  unsigned long mask;
  char buffer[DEFAULT_LINE_LENGTH];

  if (!devIsConnected(base_device.dev)) {
    fprintf(stderr,
	    "Error in base_interface::WriteCommand>> device is not initialized\n");
    return -1;
  }

  /* Sonar must be on before we make any move */
  if (SONAR_pausedp())
    SONAR_continue();
  SONAR_reset_timer();

  /* check that l doesn't overflow nbits */

  switch (n_bits) {
  case 0:
    l = 0;
    mask = 0x0;
  case 8:
    mask = 0xFF;
    break;
  case 16:
    mask = 0xFFFF;
    break;
  case 32:
    mask = 0xFFFFFFFF;
    break;
  default:
    fprintf(stderr,
	    "Error in base_interface::WriteCommand>> bad nbits %d\n", n_bits);
    return -1;
  }
  if (~mask & l) {
    fprintf(stderr,
	    "Error in base_interface::WriteCommand>> overflow on argument\n");
    fprintf(stderr, "debug: mask = %lx l = %lx\n", mask, l);
    fprintf(stderr,
	    "debug: ~mask = %lx ~mask | l = %lx\n", ~mask, (~mask | l));
    return -1;
  }

  if (n_bits != 0)
    sprintf (buffer, "%s %lx\r", command, l);
  else
    sprintf (buffer, "%s\r", command);

  /* set the timeout */
  devSetTimer(base_device.dev, &COMMAND_TIMEOUT, BASE_timeoutHnd, NULL);

  return ((int) devWriteN(devGetFd(base_device.dev), buffer,strlen(buffer)));
}

static void ProcessServoError (RWIBASE_STATUS_ENUM error)
{
  if (error == Translate_Current_Error || error == Translate_Servo_Error)
    FireHandler(rwibase_handlers, ERROR_TRANSLATION, (Pointer)(long)error);
  else
    FireHandler(rwibase_handlers, ERROR_ROTATION, (Pointer)(long)error);
}

static BOOLEAN in_report = FALSE;
static int report_line = 0;
static BOOLEAN report_corrupted = FALSE;

/*	Function Name: ProcessLine
 *	Arguments:     line -- output line from the base
 *	Description:   process the base output, updating the global
 *                     variable rwi_base and firing the installed handlers
 *	Returns:
 */

static void ProcessLine(char *line)
{
  double data_double;

  /*
     double  data_double;
     */
  int    data_int;
  unsigned long data_unsigned;

  if (devGetDebug(base_device.dev)) {
    if (debug_file_in)
      fprintf(debug_file_in,"BaseInterface receiving: [%s]\n", line);
    else
      fprintf(stderr,"BaseInterface receiving: [%s]\n", line);
  }

  /* all the streaming data come without any *, and
   * the first line with one star marks the end of the
   * status report
   */

  /* Skip prompt */
  if ( *line == '*') line++;

  if (strncmp (line, "%PSR", 4) == 0) {
    if (report_corrupted)
      ReceivedStatusReport(report_corrupted);
    report_corrupted = FALSE;
    in_report = TRUE;
    report_line = 0;
  } else if (in_report && !report_corrupted) {
  
    /* See if the string that comes is the correct one by
       testing the command and the length of the line */
    while (!(status_data & (1<<report_line)) && (report_line < 32))
      report_line++;
  
    if ( report_line >= last_report_line) {
      report_corrupted = TRUE;
      fprintf(stderr,
	      "Bad data: Got :%s: \n", line);
      return;
    }
  
    if (strncmp(line, report_data[report_line], STATUS_PREFIX_LENGTH) != 0){
      report_corrupted = TRUE;
      fprintf(stderr,
	      "Bad data: Got :%s: Expected :%s: (length %d) at %d\n",
	      line, report_data[report_line], STATUS_PREFIX_LENGTH,
	      report_line);
      return;
    }
  
    switch (report_line++) {
    case 0: /* SRD */
      break;
    case 1: /* CLK */
      sscanf(line,"CL %x", &data_int);
      /* the units are 1/256 secs. */
      tmp_state.time = data_int / 256.0;
      break;
    case 2: /* GSF */
      break;
    case 3: /* BBS */
    
      /* report bump */
    
      if (strlen(line) != 8) {
	report_corrupted = TRUE;
	fprintf(stderr, "Bad status report data %s\n", line);
      }
      if (!report_corrupted) {
	sscanf(line, "BBS %x", &data_int);
	rwi_base.bumpers = data_int;
	if (rwi_base.bumpers != 0xFFFF &&
	    previous_state.bumpers == 0xFFFF) {
	  rwi_base.bump = TRUE;
	  if (rwi_base.stopWhenBump) {
	    ProcessBump(data_int);
	  }
	}
	if (rwi_base.bumpers == 0xFFFF &&
	    previous_state.bumpers != 0xFFFF) {
	  rwi_base.bump = FALSE;
	  if (rwi_base.stopWhenBump) {
	    rwi_base.emergency = FALSE;
	  }
	}
      }
      break;
    case 4: /* BPX */
      sscanf(line,"BPX %x", &data_int);
      SET_ROBOT_STATE(pos_x, (double)((data_int - 0x8000)/POS_COUNTS_PER_CM));
      break;
    case 5: /* BPY */
      sscanf(line,"BPY %x", &data_int);
      SET_ROBOT_STATE(pos_y, (double)((data_int - 0x8000)/POS_COUNTS_PER_CM));
      break;
    case 6: /* TPH */
      sscanf(line,"TPH %x", &data_int);
      SET_ROBOT_STATE(rot_position, data_int);
      SET_ROBOT_STATE(orientation, (double) (data_int / COUNTS_PER_DEGREE));
      break;
    case 7: /* BRH */
      sscanf(line,"BRH %x", &data_int);
      rwi_base.rot_reference = (data_int - rwi_base.rot_position);
      break;
    case 8: /* TPE */
      break;
    case 9: /* TVE */
    
      /* translate velocity */
    
      if (!report_corrupted) {
	if (strlen(line) != 8) {
	  report_corrupted = TRUE;
	  fprintf(stderr, "Bad status report data %s\n", line);
	  return;
	}
	sscanf(line,"TVE %x", &data_int);
	/* Treat very small velocities as zero */
	data_double = (data_int <= MIN_TRANS_VEL_RESOLUTION ? 0 : data_int);
	tmp_state.trans_current_speed = data_double / COUNTS_PER_CM;
      }
      break;
    case 10: /* TPW */
      /* translate motor pulse width */
      if (!report_corrupted) {
	if (strlen(line) != 8) {
	  report_corrupted = TRUE;
	  fprintf(stderr, "Bad status report data %s\n", line);
	  return;
	}
	sscanf(line, "TPW %x", &data_int);
	tmp_state.trans_pulse = data_int;
      }
      break;
    case 11: /* TSF */
      /* report translate flag */
    
      if (!report_corrupted) {
	if (strlen(line) != 8) {
	  report_corrupted = TRUE;
	  fprintf(stderr, "Bad status report data %s\n", line);
	  return;
	}
	
	sscanf(line, "TSF %x", &data_int);
	tmp_state.trans_direction = ((data_int & 0x100) ? NEGATIVE : POSITIVE);
	tmp_state.trans_encoder_direction = ((data_int & 0x800)
					     ? NEGATIVE : POSITIVE);
      }
      break;
    case 12: /* TC0 */
      break;
    case 13: /* TC1 */
      break;
    case 14: /* TC2 */
      break;
    case 15: /* TC3 */
      break;
    case 16: /* RPE */
      break;
    case 17: /* RVE */
    
      /* rotate velocity */
    
      if (!report_corrupted) {
	if (strlen(line) != 8) {
	  report_corrupted = TRUE;
	  fprintf(stderr, "Bad status report data %s\n", line);
	  return;
	}
	sscanf(line,"RVE %x", &data_int);
	/* Treat very small velocities as zero */
	data_double = (data_int <= MIN_ROT_VEL_RESOLUTION ? 0 : data_int);
	tmp_state.rot_current_speed = data_double / COUNTS_PER_DEGREE;
      }
      break;
    case 18: /* RPW */
      /* rotate motor pulse width */
      if (!report_corrupted) {
	if (strlen(line) != 8) {
	  report_corrupted = TRUE;
	  fprintf(stderr, "Bad status report data %s\n", line);
	  return;
	}
	sscanf(line, "RPW %x", &data_int);
	tmp_state.rot_pulse = data_int;
      }
      break;
    case 19: /* RSF */
    
      /* report rotation flag */
    
      if (!report_corrupted) {
	if (strlen(line) != 8) {
	  report_corrupted = TRUE;
	  fprintf(stderr, "Bad status report data %s\n", line);
	  return;
	}
	sscanf(line, "RSF %x", &data_int);
	tmp_state.rot_direction = ((data_int & 0x100) ? NEGATIVE : POSITIVE);
	tmp_state.rot_encoder_direction = ((data_int & 0x800)
					   ? NEGATIVE : POSITIVE);
      }
      break;
    case 20: /* RC0 */
      break;
    case 21: /* RC1 */
      break;
    case 22: /* RC2 */
      break;
    case 23: /* RC3 */
      break;
    case 24: /* A01 */
      break;
    case 25: /* A23 */
      break;
    case 26: /* A45 */
      break;
    case 27: /* A67 */
      break;
    case 28: /* RX1 */
      break;
    case 29: /* RX2 */
      break;
    case 30: /* RX3 */
      break;
    case 31: /* RX4 */
      break;
    }
  
    if (report_line >= last_report_line) {
      /* This is the last line of a status report */
    
      in_report = FALSE;
      report_corrupted = FALSE;
      report_line = 0;
      ReceivedStatusReport(report_corrupted);
    }
  }
  /* Error Notification */


  if ((line[0] == '%') && !(strncmp (line, "%PSR", 4) == 0)) {
    /* Notification of bumps and other errors.*/
    fprintf(stderr, "Base Error Message: %s\n",line);
    if (strncmp (line, "%BS", 3) == 0) {
    
      sscanf(line,"%%BS %x", &data_int);
      ProcessBump(data_int);
    } else if (strncmp (line, "%BHI", 4) == 0) {
      errorAck(BHI);
    } else if (strncmp (line, "%BLO", 4) == 0) {
      errorAck(BLO);
    } else if (strncmp (line, "%TERR", 5) == 0) {
      ProcessServoError(Translate_Servo_Error);
    } else if (strncmp (line, "%RC", 3) == 0) {
      ProcessServoError((strncmp (line, "%RCLR", 6) ? Rotate_Current_Clear
			 : Rotate_Current_Error));
    } else if (strncmp (line, "%RERR", 4) == 0) {
      ProcessServoError(Rotate_Servo_Error);
    } else if (strncmp (line, "%TC", 3) == 0) {
      ProcessServoError((strncmp (line, "%TCLR", 6) == 0
			 ? Translate_Current_Clear
			 : Translate_Current_Error));
    }
  }

  /* answers to queries */

  else if (strncmp (line, "BV", 2) == 0) {
  
    /* answer battery voltage */
  
    sscanf(line,"BV %x", &data_int);
    /* the units are 1/10 volts */
    FireHandler(rwibase_handlers, ANSWER_BATTERY_VOLTAGE,
		(Pointer)(long) data_int);
  }

  else if (strncmp (line, "RW", 2) == 0) {
  
    /* answer rotate position */
  
    sscanf(line,"RW %lx", &data_unsigned);
  
    FireHandler(rwibase_handlers, ANSWER_ROTATE_POSITION,
		(Pointer) data_unsigned);
  }

}

/*	Function Name: BaseParseReturn
 *	Arguments:
 *	Description:   Used to parse the return from the base, a line at a time
 *	Returns:
 */

static void BaseParseReturn(char *start)
{
  /* cancel the timeout. */
  devCancelTimeout(base_device.dev);

  /* parse the characters in the buffer and update state */

  if (strcmp(start, "") != 0)
    ProcessLine (start);

}

static void BASE_report_check(Pointer call_back, Pointer client_data)
{
  BASE_resetWatchDogTimer(baseWatchDogTimeout);
  if (BASE_waiting_for_report)
    {/* missed some reports from the base */
      FireHandler(rwibase_handlers, BASE_REPORT_MISSED, (Pointer) NULL);
    }
  BASE_waiting_for_report=TRUE;
}

/*****************************************************************************
 *
 * FUNCTION: void BASE_missed(Handler handler,
 *                             Pointer client_data)
 *
 * DESCRIPTION: register a function to handle cases where a good base report
 * never in the time that 5 are expected.
 *
 *****************************************************************************/


/*****************************************************************************
 *
 * FUNCTION: void BASE_missed(Handler handler,
 *                             Pointer client_data)
 *
 * DESCRIPTION: register a function to handle cases where a good base report
 * never in the time that 5 are expected.
 *
 *****************************************************************************/

void BASE_missed(Handler handler, Pointer client_data)
{
  BASE_InstallHandler(handler, BASE_REPORT_MISSED, client_data, FALSE);
}

/*****************************************************************************
 *
 * FUNCTION: void disconnectBASE(void)
 *
 *
 *****************************************************************************/

static void disconnectBASE(DEV_PTR ignore)
{
  /* called when the base disconnects. */
  devStopPolling(base_device.dev);
  FireHandler(rwibase_handlers, BASE_DISCONNECT, (Pointer) NULL);
}


/*****************************************************************************
 *
 * FUNCTION: void BASE_missed(Handler handler,
 *                             Pointer client_data)
 *
 * DESCRIPTION: register a function to handle cases where a good base report
 * never in the time that 5 are expected.
 *
 *****************************************************************************/

static void reconnectBASE(DEV_PTR ignore)
{
  fprintf(stderr,"Reconnecting to the base:\n");
  if (devGetUseSimulator(base_device.dev)) {
    /* set up to use the simulator */
    devConnectDev(base_device.dev,devConnectToSimulator(base_device.dev));
  } else {
    /* code to open the real device here. */
    devConnectTotty(base_device.dev);
  }

  BASE_reset();

  if(devIsConnected(base_device.dev)) {
    FireHandler(rwibase_handlers, BASE_RECONNECT, (Pointer) NULL);
  }
}
