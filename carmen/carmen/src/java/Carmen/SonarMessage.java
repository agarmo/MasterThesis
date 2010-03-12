package Carmen;

import IPC.*;

/**  Carmen SonarHandler's message
   */

public class SonarMessage extends Message {
  public int num_sonars;
  /** width of sonar cone */
  public double sensor_angle;   
  /** for each sonar, the range reading */
  public double range[];
  /** location of each sonar with respect to robot as a Point */
  public Point sonar_positions[];
  /** robot state: point location */
  public Point robot_pose;
  public double tv;
  public double rv;

  private static final String CARMEN_ROBOT_SONAR_NAME = "robot_sonar";
  private static final String CARMEN_ROBOT_SONAR_FMT =
    "{int,double,<double:1>,<{double,double,double}:1>,{double,double,double},double,double,double,string}";

  /** Application module calls this to subscribe to SonarMessage.
   *  Application module must extend SonarHandler
   */
  public static void subscribe(SonarHandler handler) {
    subscribe(CARMEN_ROBOT_SONAR_NAME, CARMEN_ROBOT_SONAR_FMT, handler, 
	      SonarMessage.class, "handle");
  }

}

