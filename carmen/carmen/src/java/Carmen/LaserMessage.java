package Carmen;

import IPC.*;

/**  Carmen FrontLaserHandler's and RearLaserHandler's message
   */

public class LaserMessage extends Message {
  public LaserConfig config;
  public int num_readings;
  public float range[];
  public char tooclose[];
  public int num_remissions;
  public float remission[];
  /** position of the center of the laser **/
  public Point laser_pose; 
  /** position of the center of the robot **/
  public Point robot_pose; 
  /** robot state: translational velocity and rotational velocity */
  public double tv, rv;
  /** application defined safety distance in metres */
  public double forward_safety_dist, side_safety_dist;
  public double turn_axis;

  private static final String CARMEN_ROBOT_FRONTLASER_NAME =
    "carmen_robot_frontlaser";
  private static final String CARMEN_ROBOT_REARLASER_NAME =
    "carmen_robot_rearlaser";

  private static final String CARMEN_ROBOT_LASER_FMT = 
    "{{int,double,double,double,double,double,int},int,<float:2>,<char:2>,int,<float:5>,{double,double,double},{double,double,double},double,double,double,double,double,double,string}";

  /** Application module calls this to subscribe to LaserMessage.
   *  Application module must extend either FrontLaserHandler or RearLaserHandler.
   */
  public static void subscribeFront(FrontLaserHandler handler) {
    subscribe(CARMEN_ROBOT_FRONTLASER_NAME, CARMEN_ROBOT_LASER_FMT, handler, 
	      LaserMessage.class, "handleFrontLaser");
  }

  /** Application module calls this to subscribe to LaserMessage.
   *  Application module must extend either FrontLaserHandler or RearLaserHandler.
   */
  public static void subscribeRear(RearLaserHandler handler) {
    subscribe(CARMEN_ROBOT_FRONTLASER_NAME, CARMEN_ROBOT_LASER_FMT, handler, 
	      LaserMessage.class, "handleRearLaser");
  }

}

