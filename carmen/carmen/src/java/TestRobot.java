import Carmen.*;

public class TestRobot implements FrontLaserHandler, OdometryHandler {
  private boolean initialized = false;
  private double startTime = 0;

  public void handle (OdometryMessage message) {
    if (!initialized) {
      startTime = message.timestamp;
      initialized = true;
      return;
    }
    message.timestamp -= startTime;
    System.out.println("Odometry: "+message.tv+" m/s "+
		       Math.toDegrees(message.rv)+" deg/s");
    if (message.timestamp > 2) {
      System.exit(0);
    }
  }

  public void handleFrontLaser (LaserMessage message) {
    if (!initialized) {
      startTime = message.timestamp;
      initialized = true;
      return;
    }
    message.timestamp -= startTime;
    System.out.println("LaserHandler: Got "+message.num_readings);
    if (message.timestamp > 2) {
      System.exit(0);
    }
  }

  public static void main (String args[]) {
    Robot.initialize("TestRobot", "localhost");
    TestRobot test = new TestRobot();
    OdometryMessage.subscribe(test);
    LaserMessage.subscribeFront(test);
    Robot.setVelocity(0, Math.toRadians(30));
    Robot.dispatch();
  }
}
