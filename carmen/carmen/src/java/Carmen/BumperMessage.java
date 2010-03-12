package Carmen;

import IPC.*;

/**  Carmen BumperHandler's message
   */
public class BumperMessage extends Message 
{  
  public int num_bumpers; 
  /** state of bumper */
  public char bumper[]; 
  /** robot pose at the center of the robot */
  public Point robot_location; 
  /** commanded translational velocity (m/s) */
  public double tv;
  /** commanded rotational velocity (r/s) */
  public double rv;

  private static final String CARMEN_ROBOT_BUMPER_NAME =
    "carmen_robot_bumper";
  private static final String CARMEN_ROBOT_BUMPER_FMT = 
    "{int, <char:1>, {double,double,double}, double, double, double, string}";

  public static void subscribe(BumperHandler handler) {
    subscribe(CARMEN_ROBOT_BUMPER_NAME, CARMEN_ROBOT_BUMPER_FMT, handler,
	      BumperMessage.class, "handle");
  }

}

