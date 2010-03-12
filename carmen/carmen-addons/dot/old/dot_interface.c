
#include <carmen/carmen.h>
#include "dot_interface.h"


static carmen_dot_person_msg *person_msg_ptr_ext = NULL;
static carmen_handler_t person_msg_handler_ext = NULL;
static carmen_dot_trash_msg *trash_msg_ptr_ext = NULL;
static carmen_handler_t trash_msg_handler_ext = NULL;
static carmen_dot_door_msg *door_msg_ptr_ext = NULL;
static carmen_handler_t door_msg_handler_ext = NULL;

static carmen_dot_all_people_msg *all_people_msg_ptr_ext = NULL;
static carmen_handler_t all_people_msg_handler_ext = NULL;
static carmen_dot_all_trash_msg *all_trash_msg_ptr_ext = NULL;
static carmen_handler_t all_trash_msg_handler_ext = NULL;
static carmen_dot_all_doors_msg *all_doors_msg_ptr_ext = NULL;
static carmen_handler_t all_doors_msg_handler_ext = NULL;


void carmen_dot_reset() {

  IPC_RETURN_TYPE err;
  static carmen_dot_reset_msg msg;

  err = IPC_defineMsg(CARMEN_DOT_RESET_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_DOT_RESET_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define message", 
		       CARMEN_DOT_RESET_MSG_NAME);
  
  strcpy(msg.host, carmen_get_tenchar_host_name());
  msg.timestamp = carmen_get_time_ms();
  err = IPC_publishData(CARMEN_DOT_RESET_MSG_NAME, &msg);
  carmen_test_ipc_return(err, "Could not reset tracking", 
			 CARMEN_DOT_RESET_MSG_NAME);
}

int carmen_dot_get_all_people(carmen_dot_person_p *people, int *num_people) {

  IPC_RETURN_TYPE err;
  carmen_dot_query query;
  carmen_dot_all_people_msg *response;

  err = IPC_defineMsg(CARMEN_DOT_QUERY_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_DOT_QUERY_FMT);
  carmen_test_ipc_exit(err, "Could not define message", 
		       CARMEN_DOT_QUERY_NAME);

  query.type = CARMEN_DOT_PERSON;
  query.timestamp = carmen_get_time_ms();
  strcpy(query.host, carmen_get_tenchar_host_name());

  err = IPC_defineMsg(CARMEN_DOT_QUERY_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_DOT_QUERY_FMT);
  carmen_test_ipc_exit(err, "Could not define message", 
		       CARMEN_DOT_QUERY_NAME);  

  err = IPC_queryResponseData(CARMEN_DOT_QUERY_NAME, &query,
			      (void **) &response, 500);
  carmen_test_ipc_return_int(err, "Could not query dot",
			     CARMEN_DOT_QUERY_NAME);

  if (people) {
    if (response->people) {
      *people = (carmen_dot_person_p)
	calloc(response->num_people, sizeof(carmen_dot_person_t));
      carmen_test_alloc(people);
      memcpy(*people, response->people, response->num_people*sizeof(carmen_dot_person_t));
    }
    else
      *people = NULL;
  }
  if (num_people)
    *num_people = response->num_people;

  return 0;
}

int carmen_dot_get_all_trash(carmen_dot_trash_p *trash, int *num_trash) {

  IPC_RETURN_TYPE err;
  carmen_dot_query query;
  carmen_dot_all_trash_msg *response;

  query.type = CARMEN_DOT_TRASH;
  query.timestamp = carmen_get_time_ms();
  strcpy(query.host, carmen_get_tenchar_host_name());

  err = IPC_defineMsg(CARMEN_DOT_QUERY_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_DOT_QUERY_FMT);
  carmen_test_ipc_exit(err, "Could not define message", 
		       CARMEN_DOT_QUERY_NAME);  

  err = IPC_queryResponseData(CARMEN_DOT_QUERY_NAME, &query,
			      (void **) &response, 500);
  carmen_test_ipc_return_int(err, "Could not query dot",
			     CARMEN_DOT_QUERY_NAME);

  if (trash) {
    if (response->trash) {
      *trash = (carmen_dot_trash_p)
	calloc(response->num_trash, sizeof(carmen_dot_trash_t));
      carmen_test_alloc(trash);
      memcpy(*trash, response->trash, response->num_trash*sizeof(carmen_dot_trash_t));
    }
    else
      *trash = NULL;
  }
  if (num_trash)
    *num_trash = response->num_trash;

  return 0;
}

int carmen_dot_get_all_doors(carmen_dot_door_p *doors, int *num_doors) {

  IPC_RETURN_TYPE err;
  carmen_dot_query query;
  carmen_dot_all_doors_msg *response;

  query.type = CARMEN_DOT_DOOR;
  query.timestamp = carmen_get_time_ms();
  strcpy(query.host, carmen_get_tenchar_host_name());

  err = IPC_defineMsg(CARMEN_DOT_QUERY_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_DOT_QUERY_FMT);
  carmen_test_ipc_exit(err, "Could not define message", 
		       CARMEN_DOT_QUERY_NAME);  

  err = IPC_queryResponseData(CARMEN_DOT_QUERY_NAME, &query,
			      (void **) &response, 500);
  carmen_test_ipc_return_int(err, "Could not query dot",
			     CARMEN_DOT_QUERY_NAME);

  if (doors) {
    if (response->doors) {
      *doors = (carmen_dot_door_p)
	calloc(response->num_doors, sizeof(carmen_dot_door_t));
      carmen_test_alloc(doors);
      memcpy(*doors, response->doors, response->num_doors*sizeof(carmen_dot_door_t));
    }
    else
      *doors = NULL;
  }
  if (num_doors)
    *num_doors = response->num_doors;

  return 0;
}

static void all_people_msg_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
			       void *clientData __attribute__ ((unused))) {
  
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);

  if (all_people_msg_ptr_ext)
    err = IPC_unmarshallData(formatter, callData, all_people_msg_ptr_ext,
                             sizeof(carmen_dot_all_people_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall",
			 IPC_msgInstanceName(msgRef));

  if (all_people_msg_handler_ext)
    all_people_msg_handler_ext(all_people_msg_ptr_ext);
}

void carmen_dot_subscribe_all_people_message(carmen_dot_all_people_msg *msg,
					 carmen_handler_t handler,
					 carmen_subscribe_t subscribe_how) {

  IPC_RETURN_TYPE err = IPC_OK;  

  err = IPC_defineMsg(CARMEN_DOT_ALL_PEOPLE_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_DOT_ALL_PEOPLE_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define message", 
		       CARMEN_DOT_ALL_PEOPLE_MSG_NAME);

  if(subscribe_how == CARMEN_UNSUBSCRIBE) {
    IPC_unsubscribe(CARMEN_DOT_ALL_PEOPLE_MSG_NAME, 
		    all_people_msg_handler);
    return;
  }

  if (msg)
    all_people_msg_ptr_ext = msg;
  else {
    all_people_msg_ptr_ext = (carmen_dot_all_people_msg *)
      calloc(1, sizeof(carmen_dot_all_people_msg));
    carmen_test_alloc(all_people_msg_ptr_ext);
  }

  all_people_msg_handler_ext = handler;
  err = IPC_subscribe(CARMEN_DOT_ALL_PEOPLE_MSG_NAME, 
		      all_people_msg_handler, NULL);

  if(subscribe_how == CARMEN_SUBSCRIBE_LATEST)
    IPC_setMsgQueueLength(CARMEN_DOT_ALL_PEOPLE_MSG_NAME, 1);
  else
    IPC_setMsgQueueLength(CARMEN_DOT_ALL_PEOPLE_MSG_NAME, 100);
  
  carmen_test_ipc(err, "Could not subscribe", CARMEN_DOT_ALL_PEOPLE_MSG_NAME);
}

static void all_trash_msg_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
			       void *clientData __attribute__ ((unused))) {
  
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);

  if (all_trash_msg_ptr_ext)
    err = IPC_unmarshallData(formatter, callData, all_trash_msg_ptr_ext,
                             sizeof(carmen_dot_all_trash_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall",
			 IPC_msgInstanceName(msgRef));

  if (all_trash_msg_handler_ext)
    all_trash_msg_handler_ext(all_trash_msg_ptr_ext);
}

void carmen_dot_subscribe_all_trash_message(carmen_dot_all_trash_msg *msg,
					 carmen_handler_t handler,
					 carmen_subscribe_t subscribe_how) {

  IPC_RETURN_TYPE err = IPC_OK;  

  err = IPC_defineMsg(CARMEN_DOT_ALL_TRASH_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_DOT_ALL_TRASH_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define message", 
		       CARMEN_DOT_ALL_TRASH_MSG_NAME);

  if(subscribe_how == CARMEN_UNSUBSCRIBE) {
    IPC_unsubscribe(CARMEN_DOT_ALL_TRASH_MSG_NAME, 
		    all_trash_msg_handler);
    return;
  }

  if (msg)
    all_trash_msg_ptr_ext = msg;
  else {
    all_trash_msg_ptr_ext = (carmen_dot_all_trash_msg *)
      calloc(1, sizeof(carmen_dot_all_trash_msg));
    carmen_test_alloc(all_trash_msg_ptr_ext);
  }

  all_trash_msg_handler_ext = handler;
  err = IPC_subscribe(CARMEN_DOT_ALL_TRASH_MSG_NAME, 
		      all_trash_msg_handler, NULL);

  if(subscribe_how == CARMEN_SUBSCRIBE_LATEST)
    IPC_setMsgQueueLength(CARMEN_DOT_ALL_TRASH_MSG_NAME, 1);
  else
    IPC_setMsgQueueLength(CARMEN_DOT_ALL_TRASH_MSG_NAME, 100);
  
  carmen_test_ipc(err, "Could not subscribe", CARMEN_DOT_ALL_TRASH_MSG_NAME);
}

static void all_doors_msg_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
			       void *clientData __attribute__ ((unused))) {
  
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);

  if (all_doors_msg_ptr_ext)
    err = IPC_unmarshallData(formatter, callData, all_doors_msg_ptr_ext,
                             sizeof(carmen_dot_all_doors_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall",
			 IPC_msgInstanceName(msgRef));

  if (all_doors_msg_handler_ext)
    all_doors_msg_handler_ext(all_doors_msg_ptr_ext);
}

void carmen_dot_subscribe_all_doors_message(carmen_dot_all_doors_msg *msg,
					 carmen_handler_t handler,
					 carmen_subscribe_t subscribe_how) {

  IPC_RETURN_TYPE err = IPC_OK;  

  err = IPC_defineMsg(CARMEN_DOT_ALL_DOORS_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_DOT_ALL_DOORS_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define message", 
		       CARMEN_DOT_ALL_DOORS_MSG_NAME);

  if(subscribe_how == CARMEN_UNSUBSCRIBE) {
    IPC_unsubscribe(CARMEN_DOT_ALL_DOORS_MSG_NAME, 
		    all_doors_msg_handler);
    return;
  }

  if (msg)
    all_doors_msg_ptr_ext = msg;
  else {
    all_doors_msg_ptr_ext = (carmen_dot_all_doors_msg *)
      calloc(1, sizeof(carmen_dot_all_doors_msg));
    carmen_test_alloc(all_doors_msg_ptr_ext);
  }

  all_doors_msg_handler_ext = handler;
  err = IPC_subscribe(CARMEN_DOT_ALL_DOORS_MSG_NAME, 
		      all_doors_msg_handler, NULL);

  if(subscribe_how == CARMEN_SUBSCRIBE_LATEST)
    IPC_setMsgQueueLength(CARMEN_DOT_ALL_DOORS_MSG_NAME, 1);
  else
    IPC_setMsgQueueLength(CARMEN_DOT_ALL_DOORS_MSG_NAME, 100);
  
  carmen_test_ipc(err, "Could not subscribe", CARMEN_DOT_ALL_DOORS_MSG_NAME);
}

static void person_msg_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
			       void *clientData __attribute__ ((unused))) {
  
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);

  if (person_msg_ptr_ext)
    err = IPC_unmarshallData(formatter, callData, person_msg_ptr_ext,
                             sizeof(carmen_dot_person_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall",
			 IPC_msgInstanceName(msgRef));

  if (person_msg_handler_ext)
    person_msg_handler_ext(person_msg_ptr_ext);
}

void carmen_dot_subscribe_person_message(carmen_dot_person_msg *msg,
					 carmen_handler_t handler,
					 carmen_subscribe_t subscribe_how) {

  IPC_RETURN_TYPE err = IPC_OK;  

  err = IPC_defineMsg(CARMEN_DOT_PERSON_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_DOT_PERSON_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define message", 
		       CARMEN_DOT_PERSON_MSG_NAME);

  if(subscribe_how == CARMEN_UNSUBSCRIBE) {
    IPC_unsubscribe(CARMEN_DOT_PERSON_MSG_NAME, 
		    person_msg_handler);
    return;
  }

  if (msg)
    person_msg_ptr_ext = msg;
  else {
    person_msg_ptr_ext = (carmen_dot_person_msg *)
      calloc(1, sizeof(carmen_dot_person_msg));
    carmen_test_alloc(person_msg_ptr_ext);
  }

  person_msg_handler_ext = handler;
  err = IPC_subscribe(CARMEN_DOT_PERSON_MSG_NAME, 
		      person_msg_handler, NULL);

  if(subscribe_how == CARMEN_SUBSCRIBE_LATEST)
    IPC_setMsgQueueLength(CARMEN_DOT_PERSON_MSG_NAME, 1);
  else
    IPC_setMsgQueueLength(CARMEN_DOT_PERSON_MSG_NAME, 100);
  
  carmen_test_ipc(err, "Could not subscribe", CARMEN_DOT_PERSON_MSG_NAME);
}

static void trash_msg_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
			       void *clientData __attribute__ ((unused))) {
  
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);

  if (trash_msg_ptr_ext)
    err = IPC_unmarshallData(formatter, callData, trash_msg_ptr_ext,
                             sizeof(carmen_dot_trash_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall",
			 IPC_msgInstanceName(msgRef));

  if (trash_msg_handler_ext)
    trash_msg_handler_ext(trash_msg_ptr_ext);
}

void carmen_dot_subscribe_trash_message(carmen_dot_trash_msg *msg,
					 carmen_handler_t handler,
					 carmen_subscribe_t subscribe_how) {

  IPC_RETURN_TYPE err = IPC_OK;  

  err = IPC_defineMsg(CARMEN_DOT_TRASH_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_DOT_TRASH_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define message", 
		       CARMEN_DOT_TRASH_MSG_NAME);

  if(subscribe_how == CARMEN_UNSUBSCRIBE) {
    IPC_unsubscribe(CARMEN_DOT_TRASH_MSG_NAME, 
		    trash_msg_handler);
    return;
  }

  if (msg)
    trash_msg_ptr_ext = msg;
  else {
    trash_msg_ptr_ext = (carmen_dot_trash_msg *)
      calloc(1, sizeof(carmen_dot_trash_msg));
    carmen_test_alloc(trash_msg_ptr_ext);
  }

  trash_msg_handler_ext = handler;
  err = IPC_subscribe(CARMEN_DOT_TRASH_MSG_NAME, 
		      trash_msg_handler, NULL);

  if(subscribe_how == CARMEN_SUBSCRIBE_LATEST)
    IPC_setMsgQueueLength(CARMEN_DOT_TRASH_MSG_NAME, 1);
  else
    IPC_setMsgQueueLength(CARMEN_DOT_TRASH_MSG_NAME, 100);
  
  carmen_test_ipc(err, "Could not subscribe", CARMEN_DOT_TRASH_MSG_NAME);
}

static void door_msg_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
			       void *clientData __attribute__ ((unused))) {
  
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);

  if (door_msg_ptr_ext)
    err = IPC_unmarshallData(formatter, callData, door_msg_ptr_ext,
                             sizeof(carmen_dot_door_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall",
			 IPC_msgInstanceName(msgRef));

  if (door_msg_handler_ext)
    door_msg_handler_ext(door_msg_ptr_ext);
}

void carmen_dot_subscribe_door_message(carmen_dot_door_msg *msg,
					 carmen_handler_t handler,
					 carmen_subscribe_t subscribe_how) {

  IPC_RETURN_TYPE err = IPC_OK;  

  err = IPC_defineMsg(CARMEN_DOT_DOOR_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_DOT_DOOR_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define message", 
		       CARMEN_DOT_DOOR_MSG_NAME);

  if(subscribe_how == CARMEN_UNSUBSCRIBE) {
    IPC_unsubscribe(CARMEN_DOT_DOOR_MSG_NAME, 
		    door_msg_handler);
    return;
  }

  if (msg)
    door_msg_ptr_ext = msg;
  else {
    door_msg_ptr_ext = (carmen_dot_door_msg *)
      calloc(1, sizeof(carmen_dot_door_msg));
    carmen_test_alloc(door_msg_ptr_ext);
  }

  door_msg_handler_ext = handler;
  err = IPC_subscribe(CARMEN_DOT_DOOR_MSG_NAME, 
		      door_msg_handler, NULL);

  if(subscribe_how == CARMEN_SUBSCRIBE_LATEST)
    IPC_setMsgQueueLength(CARMEN_DOT_DOOR_MSG_NAME, 1);
  else
    IPC_setMsgQueueLength(CARMEN_DOT_DOOR_MSG_NAME, 100);
  
  carmen_test_ipc(err, "Could not subscribe", CARMEN_DOT_DOOR_MSG_NAME);
}

