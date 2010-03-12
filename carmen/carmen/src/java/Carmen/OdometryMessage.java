package Carmen;

import IPC.*;

/**  Carmen OdometryHandler's message
   */

public class OdometryMessage extends Message {
  /** robot pose at the center of the robot */
  public double x, y, theta;
  /** commanded translational (m/s) and rotational (r/s) velocity */
  public double tv, rv;
  /** robot acceleration (m/s^2) */
  public double acceleration;

  private static final String CARMEN_ROBOT_ODOMETRY_NAME = "carmen_base_odometry";
  private static final String CARMEN_ROBOT_ODOMETRY_FMT = 
  	"{double, double, double, double, double, double, double, string}";

  /** Application module calls this to subscribe to OdometryMessage.
   *  Application module must extend OdometryHandler.
   */
  public static void subscribe(OdometryHandler handler) {
    subscribe(CARMEN_ROBOT_ODOMETRY_NAME, CARMEN_ROBOT_ODOMETRY_FMT, handler, 
	      OdometryMessage.class, "handle");
  }
  
}

