/*********************************************************
 *
 * This source code is part of the Carnegie Mellon Robot
 * Navigation Toolkit (CARMEN)
 *
 * CARMEN Copyright (c) 2002 Michael Montemerlo, Nicholas
 * Roy, and Sebastian Thrun
 *
 * CARMEN is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public 
 * License as published by the Free Software Foundation; 
 * either version 2 of the License, or (at your option)
 * any later version.
 *
 * CARMEN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied 
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more 
 * details.
 *
 * You should have received a copy of the GNU General 
 * Public License along with CARMEN; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, 
 * Suite 330, Boston, MA  02111-1307 USA
 *
 ********************************************************/

#include <carmen/carmen.h>

static carmen_robot_laser_message **frontlaser_msg;
static carmen_robot_laser_message **rearlaser_msg;
static carmen_robot_sonar_message **sonar_msg;
static carmen_robot_vector_status_message **vector_status_msg;
static carmen_base_binary_data_message **base_data_msg;
static carmen_handler_t *frontlaser_msg_handler;
static carmen_handler_t *rearlaser_msg_handler;
static carmen_handler_t *sonar_msg_handler;
static carmen_handler_t *vector_status_msg_handler;
static carmen_handler_t *base_data_msg_handler;

static int context_array_size = 0;
static IPC_CONTEXT_PTR *context_array = NULL;

static int
get_context_id(void)
{
  int index = 0;
  IPC_CONTEXT_PTR current_context = IPC_getContext();

  if (context_array == NULL)
    return -1;

  while (context_array[index] != NULL) 
    {
      if (context_array[index] == current_context)
	return index;
      index++;
    }

  return -1;

}

static int 
add_context(void)
{
  int index;
  
  if (context_array == NULL)
    {
      context_array = (IPC_CONTEXT_PTR *)calloc(10, sizeof(IPC_CONTEXT_PTR));
      carmen_test_alloc(context_array);

      frontlaser_msg = (carmen_robot_laser_message **)
	calloc(10, sizeof(carmen_robot_laser_message *));
      carmen_test_alloc(frontlaser_msg);

      rearlaser_msg = (carmen_robot_laser_message **)
	calloc(10, sizeof(carmen_robot_laser_message *));
      carmen_test_alloc(rearlaser_msg);

      sonar_msg = (carmen_robot_sonar_message **)
	calloc(10, sizeof(carmen_robot_sonar_message *));
      carmen_test_alloc(sonar_msg);

      vector_status_msg = (carmen_robot_vector_status_message **)
	calloc(10, sizeof(carmen_robot_vector_status_message *));
      carmen_test_alloc(vector_status_msg);

      base_data_msg = (carmen_base_binary_data_message **)
	calloc(10, sizeof(carmen_base_binary_data_message *));
      carmen_test_alloc(base_data_msg);

      frontlaser_msg_handler = (carmen_handler_t *)
	calloc(10, sizeof(carmen_handler_t));
      carmen_test_alloc(frontlaser_msg_handler);

      rearlaser_msg_handler = (carmen_handler_t *)
	calloc(10, sizeof(carmen_handler_t));
      carmen_test_alloc(rearlaser_msg_handler);

      sonar_msg_handler = (carmen_handler_t *)
	calloc(10, sizeof(carmen_handler_t));
      carmen_test_alloc(sonar_msg_handler);

      vector_status_msg_handler = (carmen_handler_t *)
	calloc(10, sizeof(carmen_handler_t));
      carmen_test_alloc(vector_status_msg_handler);

      base_data_msg_handler = (carmen_handler_t *)
	calloc(10, sizeof(carmen_handler_t));
      carmen_test_alloc(base_data_msg_handler);

      context_array_size = 10;
      context_array[0] = IPC_getContext();


      return 0;
    }

  index = 0;
  while (index < context_array_size && context_array[index] != NULL) 
    index++;
  
  if (index == context_array_size) 
    {
      context_array_size += 10;
      context_array = (IPC_CONTEXT_PTR *)realloc
	(context_array, context_array_size*sizeof(IPC_CONTEXT_PTR));
      carmen_test_alloc(context_array);
      memset(context_array+index, 0, 10*sizeof(IPC_CONTEXT_PTR));
    }

  context_array[index] = IPC_getContext();
  return index;
}

static void 
robot_frontlaser_interface_handler(MSG_INSTANCE msgRef, 
				   BYTE_ARRAY callData,
				   void *clientData __attribute ((unused)))
{
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;
  int context_id;

  context_id = get_context_id();

  if (context_id < 0) 
    {
      carmen_warn("Bug detected: invalid context\n");
      IPC_freeByteArray(callData);
      return;
    }

  formatter = IPC_msgInstanceFormatter(msgRef);

  if (frontlaser_msg[context_id]) 
    {
      if(frontlaser_msg[context_id]->range != NULL)
	free(frontlaser_msg[context_id]->range);
      if(frontlaser_msg[context_id]->tooclose != NULL)
	free(frontlaser_msg[context_id]->tooclose);

      err = IPC_unmarshallData(formatter, callData, 
			       frontlaser_msg[context_id], 
			       sizeof(carmen_robot_laser_message));
    }

  IPC_freeByteArray(callData);

  carmen_test_ipc_return(err, "Could not unmarshall", 
			 IPC_msgInstanceName(msgRef));

  if (frontlaser_msg_handler[context_id]) 
    frontlaser_msg_handler[context_id](frontlaser_msg[context_id]);
}

void
carmen_robot_subscribe_frontlaser_message(carmen_robot_laser_message *laser,
					  carmen_handler_t handler,
					  carmen_subscribe_t subscribe_how) 
{
  IPC_RETURN_TYPE err = IPC_OK;  
  int context_id;

  err = IPC_defineMsg(CARMEN_ROBOT_FRONTLASER_NAME, 
		      IPC_VARIABLE_LENGTH, 
		      CARMEN_ROBOT_FRONTLASER_FMT);
  carmen_test_ipc_exit(err, "Could not define message", 
		       CARMEN_ROBOT_FRONTLASER_NAME);

  if (subscribe_how == CARMEN_UNSUBSCRIBE) 
    {
      IPC_unsubscribe(CARMEN_ROBOT_FRONTLASER_NAME, 
		      robot_frontlaser_interface_handler);
      return;
    }
  
  context_id = get_context_id();
  if (context_id < 0)
    context_id = add_context();

  if (laser) 
    {
      frontlaser_msg[context_id] = laser;
      memset(frontlaser_msg[context_id], 0, 
	     sizeof(carmen_robot_laser_message));
    }
  else if (frontlaser_msg[context_id] == NULL)
    {
      frontlaser_msg[context_id] = 
	(carmen_robot_laser_message *)
	calloc(1, sizeof(carmen_robot_laser_message));
      carmen_test_alloc(frontlaser_msg[context_id]);
    }

  frontlaser_msg_handler[context_id] = handler;

  err = IPC_subscribe(CARMEN_ROBOT_FRONTLASER_NAME, 
		      robot_frontlaser_interface_handler, NULL);
  if (subscribe_how == CARMEN_SUBSCRIBE_LATEST)
    IPC_setMsgQueueLength(CARMEN_ROBOT_FRONTLASER_NAME, 1);
  else
    IPC_setMsgQueueLength(CARMEN_ROBOT_FRONTLASER_NAME, 100);

  carmen_test_ipc(err, "Could not subscribe", CARMEN_ROBOT_FRONTLASER_NAME);
}

static void 
robot_rearlaser_interface_handler(MSG_INSTANCE msgRef,
				  BYTE_ARRAY callData,
				  void *clientData __attribute ((unused)))
{
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;
  int context_id;

  context_id = get_context_id();

  if (context_id < 0) 
    {
      carmen_warn("Bug detected: invalid context\n");
      IPC_freeByteArray(callData);
      return;
    }
  
  formatter = IPC_msgInstanceFormatter(msgRef);

  if (rearlaser_msg[context_id])    
    {
      if (rearlaser_msg[context_id]->range != NULL)
	free(rearlaser_msg[context_id]->range);
      if (rearlaser_msg[context_id]->tooclose != NULL)
	free(rearlaser_msg[context_id]->tooclose);
      
      err = IPC_unmarshallData(formatter, callData, 
			       rearlaser_msg[context_id], 
			       sizeof(carmen_robot_laser_message)); 
    }

  IPC_freeByteArray(callData);

  carmen_test_ipc_return(err, "Could not unmarshall", 
			 IPC_msgInstanceName(msgRef));      

  if (rearlaser_msg_handler[context_id]) 
    rearlaser_msg_handler[context_id](rearlaser_msg[context_id]);
}

void 
carmen_robot_subscribe_rearlaser_message(carmen_robot_laser_message *laser,
					 carmen_handler_t handler,
					 carmen_subscribe_t subscribe_how) 
{
  IPC_RETURN_TYPE err = IPC_OK;  
  int context_id;

  err = IPC_defineMsg(CARMEN_ROBOT_REARLASER_NAME, 
		      IPC_VARIABLE_LENGTH, 
		      CARMEN_ROBOT_REARLASER_FMT);
  carmen_test_ipc_exit(err, "Could not define message", 
		       CARMEN_ROBOT_REARLASER_NAME);

  if (subscribe_how == CARMEN_UNSUBSCRIBE) 
    {
      IPC_unsubscribe(CARMEN_ROBOT_REARLASER_NAME, 
		      robot_rearlaser_interface_handler);
      return;
    }

  context_id = get_context_id();
  if (context_id < 0)
    context_id = add_context();

  if (laser)
    {
      rearlaser_msg[context_id] = laser;
      memset(rearlaser_msg[context_id], 0,
	     sizeof(carmen_robot_laser_message));
    }
  else if (rearlaser_msg[context_id] == NULL)
    {
      rearlaser_msg[context_id] = 
	(carmen_robot_laser_message *)
	calloc(1, sizeof(carmen_robot_laser_message));
      carmen_test_alloc(rearlaser_msg[context_id]);
    }

  rearlaser_msg_handler[context_id] = handler;

  err = IPC_subscribe(CARMEN_ROBOT_REARLASER_NAME, 
		      robot_rearlaser_interface_handler, NULL);
  if (subscribe_how == CARMEN_SUBSCRIBE_LATEST)
    IPC_setMsgQueueLength(CARMEN_ROBOT_REARLASER_NAME, 1);
  else
    IPC_setMsgQueueLength(CARMEN_ROBOT_REARLASER_NAME, 100);

  carmen_test_ipc(err, "Could not subscribe", CARMEN_ROBOT_REARLASER_NAME);
}

static void 
robot_sonar_interface_handler(MSG_INSTANCE msgRef, 
			      BYTE_ARRAY callData,
			      void *clientData __attribute ((unused)))
{
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;
  int context_id;

  context_id = get_context_id();

  if (context_id < 0) 
    {
      carmen_warn("Bug detected: invalid context\n");
      IPC_freeByteArray(callData);
      return;
    }

  formatter = IPC_msgInstanceFormatter(msgRef);
  
  if (sonar_msg[context_id]) 
    {
      if(sonar_msg[context_id]->range != NULL)
	free(sonar_msg[context_id]->range);
      if(sonar_msg[context_id]->tooclose != NULL)
	free(sonar_msg[context_id]->tooclose);
      if(sonar_msg[context_id]->sonar_locations != NULL)
	free(sonar_msg[context_id]->sonar_locations);

      err = IPC_unmarshallData(formatter, callData, 
			       sonar_msg[context_id],
			       sizeof(carmen_robot_sonar_message));
    }
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall", 
			 IPC_msgInstanceName(msgRef));

  if (sonar_msg_handler[context_id]) 
    sonar_msg_handler[context_id](sonar_msg[context_id]);
}

void
carmen_robot_subscribe_sonar_message(carmen_robot_sonar_message *sonar,
				     carmen_handler_t handler,
				     carmen_subscribe_t subscribe_how) 
{
  IPC_RETURN_TYPE err = IPC_OK;  
  int context_id;

  err = IPC_defineMsg(CARMEN_ROBOT_SONAR_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_ROBOT_SONAR_FMT);
  carmen_test_ipc_exit(err, "Could not define message", 
		       CARMEN_ROBOT_SONAR_NAME);

  if (subscribe_how == CARMEN_UNSUBSCRIBE) 
    {
      IPC_unsubscribe(CARMEN_ROBOT_SONAR_NAME, 
		      robot_sonar_interface_handler);
      return;
    }

  context_id = get_context_id();
  if (context_id < 0)
    context_id = add_context();

  if (sonar)
    {
      sonar_msg[context_id] = sonar;
      memset(sonar_msg[context_id], 0,
	     sizeof(carmen_robot_sonar_message));
    }
  else if (sonar_msg[context_id] == NULL)
    {
      sonar_msg[context_id] = 
	(carmen_robot_sonar_message *)
	calloc(1, sizeof(carmen_robot_sonar_message));
      carmen_test_alloc(sonar_msg[context_id]);
    }

  sonar_msg_handler[context_id] = handler;

  err = IPC_subscribe(CARMEN_ROBOT_SONAR_NAME, 
		      robot_sonar_interface_handler, NULL);
  if (subscribe_how == CARMEN_SUBSCRIBE_LATEST)
    IPC_setMsgQueueLength(CARMEN_ROBOT_SONAR_NAME, 1);
  else
    IPC_setMsgQueueLength(CARMEN_ROBOT_SONAR_NAME, 100);

  carmen_test_ipc(err, "Could not subscribe", CARMEN_ROBOT_SONAR_NAME);
}

static void 
vector_status_message_handler(MSG_INSTANCE msgRef, 
			      BYTE_ARRAY callData,
			      void *clientData __attribute ((unused)))
{
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;
  int context_id;

  context_id = get_context_id();

  if (context_id < 0) 
    {
      carmen_warn("Bug detected: invalid context\n");
      IPC_freeByteArray(callData);
      return;
    }

  formatter = IPC_msgInstanceFormatter(msgRef);
  
  if (vector_status_msg[context_id]) 
    err = IPC_unmarshallData(formatter, callData, 
			     vector_status_msg[context_id],
			     sizeof(carmen_robot_vector_status_message));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall", 
			 IPC_msgInstanceName(msgRef));

  if (vector_status_msg_handler[context_id]) 
    vector_status_msg_handler[context_id](vector_status_msg[context_id]);
}

void
carmen_robot_subscribe_vector_status_message
(carmen_robot_vector_status_message *status, carmen_handler_t handler, 
 carmen_subscribe_t subscribe_how) 
{
  IPC_RETURN_TYPE err = IPC_OK;  
  int context_id;

  err = IPC_defineMsg(CARMEN_ROBOT_VECTOR_STATUS_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_ROBOT_VECTOR_STATUS_FMT);
  carmen_test_ipc_exit(err, "Could not define message", 
		       CARMEN_ROBOT_VECTOR_STATUS_NAME);

  if (subscribe_how == CARMEN_UNSUBSCRIBE) 
    {
      IPC_unsubscribe(CARMEN_ROBOT_VECTOR_STATUS_NAME, 
		      vector_status_message_handler);
      return;
    }

  context_id = get_context_id();
  if (context_id < 0)
    context_id = add_context();

  if (status)
    {
      vector_status_msg[context_id] = status;
      memset(vector_status_msg[context_id], 0,
	     sizeof(carmen_robot_vector_status_message));
    }
  else if (vector_status_msg[context_id] == NULL)
    {
      vector_status_msg[context_id] = 
	(carmen_robot_vector_status_message *)
	calloc(1, sizeof(carmen_robot_vector_status_message));
      carmen_test_alloc(vector_status_msg[context_id]);
    }

  vector_status_msg_handler[context_id] = handler;

  err = IPC_subscribe(CARMEN_ROBOT_VECTOR_STATUS_NAME, 
		      vector_status_message_handler, NULL);
  if (subscribe_how == CARMEN_SUBSCRIBE_LATEST)
    IPC_setMsgQueueLength(CARMEN_ROBOT_VECTOR_STATUS_NAME, 1);
  else
    IPC_setMsgQueueLength(CARMEN_ROBOT_VECTOR_STATUS_NAME, 100);

  carmen_test_ipc(err, "Could not subscribe", CARMEN_ROBOT_VECTOR_STATUS_NAME);
}

static void 
robot_base_data_interface_handler(MSG_INSTANCE msgRef, 
				  BYTE_ARRAY callData,
				  void *clientData __attribute ((unused)))
{
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;
  int context_id;

  context_id = get_context_id();

  if (context_id < 0) 
    {
      carmen_warn("Bug detected: invalid context\n");
      IPC_freeByteArray(callData);
      return;
    }

  formatter = IPC_msgInstanceFormatter(msgRef);

  if (base_data_msg[context_id]) 
    {
      if(base_data_msg[context_id]->data != NULL)
	free(base_data_msg[context_id]->data);

      err = IPC_unmarshallData(formatter, callData, 
			       base_data_msg[context_id], 
			       sizeof(carmen_base_binary_data_message));
    }

  IPC_freeByteArray(callData);

  carmen_test_ipc_return(err, "Could not unmarshall", 
			 IPC_msgInstanceName(msgRef));

  if (base_data_msg_handler[context_id]) 
    base_data_msg_handler[context_id](base_data_msg[context_id]);
}

void
carmen_robot_subscribe_base_binary_data_message
 (carmen_base_binary_data_message *base_data, carmen_handler_t handler,
  carmen_subscribe_t subscribe_how) 
{
  IPC_RETURN_TYPE err = IPC_OK;  
  int context_id;

  err = IPC_defineMsg(CARMEN_BASE_BINARY_DATA_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_BASE_BINARY_DATA_FMT);
  carmen_test_ipc_exit(err, "Could not define message", 
		       CARMEN_BASE_BINARY_DATA_NAME);

  if (subscribe_how == CARMEN_UNSUBSCRIBE) 
    {
      IPC_unsubscribe(CARMEN_BASE_BINARY_DATA_NAME, 
		      robot_base_data_interface_handler);
      return;
    }
  
  context_id = get_context_id();
  if (context_id < 0)
    context_id = add_context();

  if (base_data) 
    {
      base_data_msg[context_id] = base_data;
      memset(base_data_msg[context_id], 0, 
	     sizeof(carmen_base_binary_data_message));
    }
  else if (base_data_msg[context_id] == NULL)
    {
      base_data_msg[context_id] = 
	(carmen_base_binary_data_message *)
	calloc(1, sizeof(carmen_base_binary_data_message));
      carmen_test_alloc(base_data_msg[context_id]);
    }

  base_data_msg_handler[context_id] = handler;

  err = IPC_subscribe(CARMEN_BASE_BINARY_DATA_NAME, 
		      robot_base_data_interface_handler, NULL);
  if (subscribe_how == CARMEN_SUBSCRIBE_LATEST)
    IPC_setMsgQueueLength(CARMEN_BASE_BINARY_DATA_NAME, 1);
  else
    IPC_setMsgQueueLength(CARMEN_BASE_BINARY_DATA_NAME, 100);

  carmen_test_ipc(err, "Could not subscribe", 
		  CARMEN_BASE_BINARY_DATA_NAME);
}


void 
carmen_robot_velocity_command(double tv, double rv)
{
  IPC_RETURN_TYPE err;
  static carmen_robot_velocity_message v;
  static int first = 1;

  if(first) {
    strcpy(v.host, carmen_get_tenchar_host_name());
    err = IPC_defineMsg(CARMEN_ROBOT_VELOCITY_NAME, IPC_VARIABLE_LENGTH, 
			CARMEN_ROBOT_VELOCITY_FMT);
    carmen_test_ipc_exit(err, "Could not define message", 
			 CARMEN_ROBOT_VELOCITY_NAME);
    first = 0;
  }

  v.tv = tv;
  v.rv = rv;
  v.timestamp = carmen_get_time_ms();

  err = IPC_publishData(CARMEN_ROBOT_VELOCITY_NAME, &v);
  carmen_test_ipc(err, "Could not publish", CARMEN_ROBOT_VELOCITY_NAME);
}

void
carmen_robot_move_along_vector(double distance, double theta)
{
  IPC_RETURN_TYPE err;
  static carmen_robot_vector_move_message msg;
  static int first = 1;

  if(first) {
    strcpy(msg.host, carmen_get_tenchar_host_name());
    err = IPC_defineMsg(CARMEN_ROBOT_VECTOR_MOVE_NAME, IPC_VARIABLE_LENGTH, 
			CARMEN_ROBOT_VECTOR_MOVE_FMT);
    carmen_test_ipc_exit(err, "Could not define message", 
			 CARMEN_ROBOT_VECTOR_MOVE_NAME);
    first = 0;
  }
  msg.distance = distance;
  msg.theta = theta;
  msg.timestamp = carmen_get_time_ms();

  err = IPC_publishData(CARMEN_ROBOT_VECTOR_MOVE_NAME, &msg);
  carmen_test_ipc(err, "Could not publish", CARMEN_ROBOT_VECTOR_MOVE_NAME);
}

void
carmen_robot_follow_trajectory(carmen_traj_point_t *trajectory, 
			       int trajectory_length,
			       carmen_traj_point_t *robot)
{
  IPC_RETURN_TYPE err;
  static carmen_robot_follow_trajectory_message msg;
  static int first = 1;

  if(first) {
    strcpy(msg.host, carmen_get_tenchar_host_name());
    err = IPC_defineMsg(CARMEN_ROBOT_FOLLOW_TRAJECTORY_NAME, 
			IPC_VARIABLE_LENGTH, 
			CARMEN_ROBOT_FOLLOW_TRAJECTORY_FMT);
    carmen_test_ipc_exit(err, "Could not define message", 
			 CARMEN_ROBOT_FOLLOW_TRAJECTORY_NAME);
    first = 0;
  }

  msg.trajectory = trajectory;
  msg.trajectory_length = trajectory_length;
  msg.robot_position = *robot;

  msg.timestamp = carmen_get_time_ms();

  err = IPC_publishData(CARMEN_ROBOT_FOLLOW_TRAJECTORY_NAME, &msg);
  carmen_test_ipc(err, "Could not publish", 
		  CARMEN_ROBOT_FOLLOW_TRAJECTORY_NAME);
}

void 
carmen_robot_send_base_binary_command(char *data, int length)
{
  IPC_RETURN_TYPE err;
  static carmen_base_binary_data_message msg;
  static int first = 1;

  if(first) {
    strcpy(msg.host, carmen_get_tenchar_host_name());
    err = IPC_defineMsg(CARMEN_BASE_BINARY_COMMAND_NAME, 
			IPC_VARIABLE_LENGTH, 
			CARMEN_BASE_BINARY_COMMAND_FMT);
    carmen_test_ipc_exit(err, "Could not define message", 
			 CARMEN_BASE_BINARY_COMMAND_NAME);
    first = 0;
  }

  msg.data = data;
  msg.size = length;

  msg.timestamp = carmen_get_time_ms();

  err = IPC_publishData(CARMEN_BASE_BINARY_COMMAND_NAME, &msg);
  carmen_test_ipc(err, "Could not publish", 
		  CARMEN_BASE_BINARY_COMMAND_NAME);
}
