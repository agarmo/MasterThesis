package Carmen;

public class LaserConfig 
{
  /** what kind of laser is this */
  public int laser_type;  
  /** angle of the first beam relative 
      to to the center of the laser **/
  public double start_angle;                     
  /** field of view of the laser **/
  public double fov;
  /** angular resolution of the laser **/
  public double angular_resolution;
  /** the maximum valid range of a measurement  **/
  public double maximum_range;
  /** error in the range measurements**/
  public double accuracy;

  /** if and what kind of remission values are used **/
  public int remission_mode;
}
