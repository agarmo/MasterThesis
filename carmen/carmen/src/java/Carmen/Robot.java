package Carmen;

import java.util.*;
import IPC.*;
/** Carmen class for  a Robot */
public class Robot {
  /** Event Handler receives this and can set its fields */
  public static class BaseResetMessage extends Message {
    private static final String CARMEN_BASE_RESET_COMMAND_NAME = 
      "carmen_base_reset_command";
    private static final String CARMEN_BASE_RESET_COMMAND_FMT = 
      "{double,string}";
      
    public void publish() {
      publish(CARMEN_BASE_RESET_COMMAND_NAME, 
	      CARMEN_BASE_RESET_COMMAND_FMT, this);
    }
  }


  /** Event Handler receives this and can set its fields */
  public static class RobotVelocityMessage extends Message {
    public double tv, rv;

    private static final String CARMEN_ROBOT_VELOCITY_NAME = 
      "carmen_robot_vel";
    private static final String CARMEN_ROBOT_VELOCITY_FMT = 
    "{double,double,double,string}";

    /** Event Handler receives this and can set its fields */
    RobotVelocityMessage(double tv, double rv) {
      this.tv = tv;
      this.rv = rv;
    }

    public void publish() {
      publish(CARMEN_ROBOT_VELOCITY_NAME, CARMEN_ROBOT_VELOCITY_FMT, this);
    }
  }


  /** Event Handler receives this and can set its fields */
  public static class RobotVectorMoveMessage extends Message {
    public double distance, theta;

    private static final String CARMEN_ROBOT_VECTOR_MOVE_NAME = 
      "carmen_robot_vector_move";
    private static final String CARMEN_ROBOT_VECTOR_MOVE_FMT = 
      "{double,double,double,string}";
    
    RobotVectorMoveMessage(double distance, double theta) {
      this.distance = distance;
      this.theta = theta;
    }

    public void publish() {
      publish(CARMEN_ROBOT_VECTOR_MOVE_NAME, CARMEN_ROBOT_VECTOR_MOVE_FMT, 
	      this);
    }
  }

  /** Application module calls this class method to direct the robot
   * at stated translational velocity (m/s) and rotational velocity (r/s).
   * This results in a message via IPC to the Orc_daemon then out to the 
   * robot base. To view how the message is processed in more detail, see
   * the Carmen c-code base in subdirectory orc_daemon file orclib.c and
   * procedure: carmen_base_direct_set_velocity 
   */
  public static void setVelocity(double tv, double rv) {
    RobotVelocityMessage velocityMessage = new RobotVelocityMessage(tv, rv);
    velocityMessage.publish();
  }

  /** Application module calls this class method to direct the robot
   * to proceed in a vector stated by distance and orientation.
   */
   public static void moveAlongVector(double distance, double theta) {
    RobotVectorMoveMessage vectorMessage = 
      new RobotVectorMoveMessage(distance, theta);
    vectorMessage.publish();
  }

/** Application module calls this class method to reset the 
   * robot base to the initialized state.
   */
   public static void resetRobotBase() {
    BaseResetMessage resetMessage = new BaseResetMessage();
    resetMessage.publish();
  }


  public static void initialize(String moduleName, String SBCName)
  {
    IPC.connectModule(moduleName, SBCName);
  }

  public static void dispatch()
  {
    IPC.dispatch();
  }

  // Dispatch but only for as long as timeoutMSecs 

  public static int listen (long timeoutMSecs) {
    return IPC.listen(timeoutMSecs);
  }

}
