package Carmen;
/** Carmen Trajectory Point */

public class TrajPoint {
  /** current trajectory status: x, y (metres), theta (radians) */
  public double x, y, theta;
  /** current trajectory status in translational velocity (m/s) and rotational velocity (r/s) */
  public double t_vel, r_vel;
}
