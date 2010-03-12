import Carmen.*;

public class TestBase implements OdometryHandler
{
  private boolean initialized = false;
  private double startTime = 0;

  public void handle(OdometryMessage message) {
    if (!initialized) {
      startTime = message.timestamp;
      initialized = true;
      return;
    }
    message.timestamp -= startTime;
    System.out.println("OdometryHandler: Got message at "+message.timestamp+
		       " from "+message.host);
    if (message.timestamp > 2) {
      System.exit(0);
    }
  }

  public static void main (String args[]) {
    Robot.initialize("TestBase", "localhost");
    Robot.resetRobotBase();
    OdometryMessage.subscribe(new TestBase());
    Robot.dispatch();
  }
}
