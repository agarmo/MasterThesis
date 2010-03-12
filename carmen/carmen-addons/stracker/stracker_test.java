public class stracker_test {

  protected static class carmen_stracker_pos_msg {
      public double dist;
      public double theta;
      public double timestamp;
      public char[] host;
  }

  protected static final String CARMEN_STRACKER_POS_MSG_NAME = "carmen_stracker_pos_msg";
  protected static final String CARMEN_STRACKER_POS_MSG_FMT =
      "{double, double, double, [char:10]}";

  private static class stracker_pos_handler implements IPC.HANDLER_TYPE {
    public void handle (IPC.MSG_INSTANCE msgRef, Object callData) {
	carmen_stracker_pos_msg msg = (carmen_stracker_pos_msg) callData;
	System.out.println("recv'd pos: (" + msg.dist + ", " + msg.theta + ")");
    }
  }

  public static void main (String args[]) throws Exception {

    // Connect to the central server
    IPC.connect("stracker_test");

    IPC.defineFormat(CARMEN_STRACKER_POS_MSG_NAME, CARMEN_STRACKER_POS_MSG_FMT);
  
    IPC.subscribeData(CARMEN_STRACKER_POS_MSG_NAME, new stracker_pos_handler(),
		      carmen_stracker_pos_msg.class);

    //IPC.dispatch();
    while (true) {
	IPC.listenWait(10);
	System.out.print(".");
    }

    //IPC.disconnect();
  }
}
