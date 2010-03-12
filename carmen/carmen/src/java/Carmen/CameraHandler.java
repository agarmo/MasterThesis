package Carmen;
/**
 * Declaration of Carmen event handler for camera.
 *
 * A Carmen module will implement this.
 * @author RSS-CARMEN development team 
 * @version 1.0, Mar 1, 2005
 */

public interface CameraHandler {
   /**
   *  event handler for camera
   */
 public void handle (CameraMessage message);
}
