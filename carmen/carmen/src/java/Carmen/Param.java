package Carmen;

import java.util.HashMap;
import IPC.*;
import java.lang.reflect.*;

/**  Carmen class for handling robot parameters in file carmen.ini 
   */
public class Param {
  private static boolean started = false;
  private static HashMap handlerMap;

  private static final String CARMEN_PARAM_VARIABLE_CHANGE_NAME = 
    "carmen_param_variable_change";
  private static final String CARMEN_PARAM_VARIABLE_CHANGE_FMT =
    "{string, string, string, int, int, double, string}";

  private static final String CARMEN_PARAM_QUERY_NAME =
    "carmen_param_query_string";
  private static final String CARMEN_PARAM_QUERY_FMT = 
    "{string, string, double, string}";

  private static final String CARMEN_PARAM_RESPONSE_STRING_NAME =
    "carmen_param_respond_string";
  private static final String CARMEN_PARAM_RESPONSE_STRING_FMT =
    "{string, string, string, int, int, double, string}";

  private static final String CARMEN_PARAM_SET_NAME =
    "carmen_param_set";
  private static final String CARMEN_PARAM_SET_FMT =
    "{string, string, string, double, string}";

  /** Class method that handles query of current parameter values by module and variable */
  public static class ParamQuery extends Message {
    public String moduleName;
    public String variableName;

    ParamQuery(String moduleName, String variableName) {
      this.moduleName = moduleName;
      this.variableName = variableName;
    }
  }
  /** inner class of Param */
  public static class ParamResponse extends Message {
    public String moduleName;
    public String variableName;
    public String value;
    public int expert;
    public int status;
  }

  /** inner class of Param */
  public static class ParamSet extends Message {
    public String moduleName;
    public String variableName;
    public String value;

    ParamSet(String moduleName, String variableName, String value) {
      this.moduleName = moduleName;
      this.variableName = variableName;
      this.value = value;
    }
  }

  /** Class method for parameter queryring */
  public static String query(String moduleName, String variableName) {
    IPC.defineMsg(CARMEN_PARAM_QUERY_NAME, CARMEN_PARAM_QUERY_FMT);
    IPC.defineMsg(CARMEN_PARAM_RESPONSE_STRING_NAME, 
		  CARMEN_PARAM_RESPONSE_STRING_FMT);
    ParamQuery query = new ParamQuery(moduleName, variableName);
    ParamResponse response = (ParamResponse)IPC.queryResponseData
      (CARMEN_PARAM_QUERY_NAME, query, ParamResponse.class, 5000);
    return response.value;
  }
  /** Class method to set a variable with a new value */
  public static boolean set(String moduleName, String variable, 
			    String newValue) {
    ParamSet msg = new ParamSet(moduleName, variable, newValue);
    ParamResponse response = (ParamResponse)IPC.queryResponseData
      (CARMEN_PARAM_SET_NAME, msg, ParamResponse.class, 5000);
    if (response.status == 0)
      return true;
    return false;
  }

  public static boolean parseBoolean(String value) {
    if (value.equalsIgnoreCase("1"))
      return true;
    return false;
  }

  public static boolean parseOnoff(String value) {
    if (value.equalsIgnoreCase("on"))
      return true;
    return false;
  }
}
