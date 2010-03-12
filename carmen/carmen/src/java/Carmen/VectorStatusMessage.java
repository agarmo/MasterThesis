package Carmen;
/**  Carmen VectorStatusHandler's message
   */

public class VectorStatusMessage extends Message {
  /** current distance in vector path */
  public double vector_distance;
  /** current vector angle in vector path */
  public double vector_angle;
  public double timestamp;
  public char host[];

  private static final String CARMEN_ROBOT_VECTOR_STATUS_NAME =
    "carmen_robot_vector_status";
  private static final String CARMEN_ROBOT_VECTOR_STATUS_FMT =
    "{double,double,double,string}";
  
  public static void subscribe(VectorStatusHandler handler)
  {
    subscribe(CARMEN_ROBOT_VECTOR_STATUS_NAME, CARMEN_ROBOT_VECTOR_STATUS_FMT, 
	      handler, VectorStatusMessage.class, "handle");
  }

  public void publish()
  {
    publish(CARMEN_ROBOT_VECTOR_STATUS_NAME, 
	    CARMEN_ROBOT_VECTOR_STATUS_FMT, this);
  }

}

