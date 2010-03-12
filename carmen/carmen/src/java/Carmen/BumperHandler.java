package Carmen;
/**
 * Declaration of Carmen event handler for bump sensors.
 *
 * A Carmen module will implement this.
 * @author RSS-CARMEN development team 
 * @version 1.0, Mar 1, 2005
 */

public interface BumperHandler {
  /**
   *  event handler for bump sensors
   */
  public void handle(BumperMessage message);
}
