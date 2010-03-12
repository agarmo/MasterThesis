package Carmen;

import IPC.*;

/**  Carmen CameraHandler's message
   */

public class CameraMessage extends Message {
  /** unit is pixels */
  public int width;
  /** unit is pixels */
  public int height;
  public int bytes_per_char;
  /** size of image[]  */
  public int image_size;
  /** camera image */
  public char image[];

  private static final String CARMEN_CAMERA_IMAGE_NAME = 
    "carmen_camera_image";    
  private static final String CARMEN_CAMERA_IMAGE_FMT = 
    "{int,int,int,int,<char:4>,double,string}";

  public static void subscribe(CameraHandler handler) {
    subscribe(CARMEN_CAMERA_IMAGE_NAME, CARMEN_CAMERA_IMAGE_FMT, handler, 
	      CameraMessage.class, "handle");
  }

  public void publish() {
    publish(CARMEN_CAMERA_IMAGE_NAME, CARMEN_CAMERA_IMAGE_FMT, this);
  }
}

