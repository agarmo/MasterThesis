package Carmen;

import IPC.*;
 
public class ArmMessage extends Message {
  public int flags;
  public int num_joints;
  public double joint_angles[];
  public int num_currents;
  public double joint_currents[];
  public int num_vels;
  public double joint_angular_vels[];
  public int gripper_closed;
  
  private static final String CARMEN_ARM_STATE_NAME = 
    "carmen_arm_state";
  private static final String CARMEN_ARM_STATE_FMT =  
    "{int,int,<double:2>,int,<double:4>,int,<double:6>,int,double,string}";

  public static void subscribe(CameraHandler handler) {
    subscribe(CARMEN_ARM_STATE_NAME, CARMEN_ARM_STATE_FMT, handler, 
	      ArmMessage.class, "handle");
  }
}
