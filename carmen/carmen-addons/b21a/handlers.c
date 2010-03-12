/*****************************************************************************
 * PROJECT: TCA
 *
 * (c) Copyright Richard Goodwin, 1995. All rights reserved.
 *
 * FILE: handlers.c
 *
 * ABSTRACT:
 * 
 * A couple of utility functions to manage handlers.
 * Possible improvements: allow more than one event handler
 *
 * ADAPTED FROM XAVIER SOFTWARE TO FACILITATE ADDING I/O TO MODULES.
 *
 *****************************************************************************/

#include <carmen/carmen.h>
#include "handlers.h"

/*	Function Name: CreateHandlerList
 *	Arguments:     number_handlers
 *	Description:   create a handler list to manage the number
 *                     of handlers given as paramenter
 *	Returns:       nothing
 */

HandlerList CreateHandlerList(int number_handlers)
{
  HandlerList handler_list;
  
  handler_list = (HandlerList) calloc((size_t)sizeof(HandlerData),
				      (size_t)number_handlers);
  carmen_test_alloc(handler_list);
  bzero(handler_list,sizeof(HandlerData));
  
  return handler_list;
}



/*	Function Name: InstallHandler
 *	Arguments:     list     -- handler list
 *                     position -- event position where store the handler
 *                     client_data -- client data to store with the handler
 *	Description:   add a handler in the list, in the given position.
 *                     The module support a maximum of MAX_HANDLERS_PER_EVENT
 *                     (10)
 *                     per event (position)
 *	returns:       nothing
 */

void InstallHandler(HandlerList list, Handler handler,
		    int position, void *client_data,
		    BOOLEAN remove)
{
  int i;
  
  for( i = 0; i < MAX_HANDLERS_PER_EVENT; i++ ) {
    if( list[position][i].handler == handler )
      return;
    if (list[position][i].handler == NULL)
      break;
  }
  if (i == MAX_HANDLERS_PER_EVENT)
    return;
  list[position][i].handler = handler;
  list[position][i].client_data = client_data;
  list[position][i].remove = remove;
}


/*	Function Name: IsHandlerInstalled
 *	Arguments:     list     -- handler list
 *                     handler  -- handler to ask about
 *                     position -- event position
 *	Description:   check if <handler> is in the given position
 *	Returns:       BOOLEAN
 */

BOOLEAN IsHandlerInstalled(HandlerList list, Handler handler, int position)
{
  int i;
  
  for( i = 0; i < MAX_HANDLERS_PER_EVENT; i++ )
    if (list[position][i].handler == handler)
      return TRUE;
    else if (list[position][i].handler == NULL)
      return FALSE;
  return FALSE;
}



/*	Function Name: RemoveHandler
 *	Arguments:     list     -- handler list
 *                     handler  -- handler
 *                     position -- event position
 *	Description:   remove a handler from the handler list
 *	Returns:       nothing
 */

void RemoveHandler(HandlerList list, Handler handler, int position)
{
  int i;
  
  for( i = 0; i < MAX_HANDLERS_PER_EVENT; i++ )
    if (list[position][i].handler == handler)
      break;
  if ( i == MAX_HANDLERS_PER_EVENT ) /* not found */
    return;
  for (i++; i < MAX_HANDLERS_PER_EVENT; i++){
    list[position][i-1] = list[position][i];
    if (list[position][i].handler == NULL)
      return;
  }
}

void RemoveAllHandlers(HandlerList list, int position)
{
  int i;
  
  for( i = 0; (i < MAX_HANDLERS_PER_EVENT) &&
      (list[position][i].handler != NULL); i++ )
    list[position][i].handler = NULL;
}


/*	Function Name: CallHandler
 *	Arguments:     list          -- handler entry
 *                     callback_data -- pointer that will be read
 *                                      by the handler
 *	Description:   the handler is executed with the given data
 *                     and the client_data previously installed. The
 *                     callback_data can be a pointer to a structure,
 *                     a char or an integer (the later two casted to 
 *                     void *)
 *	Returns:       nothing
 */

void CallHandler(HandlerEntry handler, void *callback_data)
{
  if(handler != NULL) {
    (handler->handler)(callback_data, handler->client_data);
    if (handler->remove) {
      bzero(handler, sizeof(HandlerEntry *));
    }
  }
}

/*	Function Name: FireHandler
 *	Arguments:     list          -- handler list
 *                     position      -- event position
 *                     callback_data -- pointer that will be read
 *                                      by the handler
 *	Description:   the handler is executed with the given data
 *                     and the client_data previously installed. The
 *                     callback_data can be a pointer to a structure,
 *                     a char or an integer (the later two casted to
 *                     void *)
 *	Returns:       nothing
 */

void FireHandler(HandlerList list, int position, Pointer callback_data)
{
  int i;
  
  /* call the routines to fire the handlers. */
  for( i = 0; ((i < MAX_HANDLERS_PER_EVENT) &&
	       (list[position][i].handler != NULL));) {
    if(list[position][i].handler != NULL) {
      (*list[position][i].handler)(callback_data,
				   list[position][i].client_data);
      if(list[position][i].remove) {
	/* Readjust the list so it is valid. */
	if (i < MAX_HANDLERS_PER_EVENT-1)
	  bcopy(&(list[position][i+1]), &(list[position][i]),
		(MAX_HANDLERS_PER_EVENT - i -1)* sizeof(_HandlerEntry));
	bzero(&(list[position][MAX_HANDLERS_PER_EVENT-1]), 
	      sizeof(_HandlerEntry));
      } else {
	i++;
      }
    }
  }
}

/************/
/*   TEST   */
/************/


#ifdef TEST

#define EVENT1 0
#define EVENT2 1
#define EVENT3 2
#define NUM_EVENTS 3

typedef struct test {
  double data1, data2, data3;
} _Test, *Test;


static void hand1(void)
{
  printf ("\nHello world, event1\n");
}

static void hand2(double callback_data, void *client_data)
{
  printf ("\nHello world, event2\n");
  printf("Callback data: %lf \nClient data: %d\n", callback_data,
	 (int32) client_data);
}

static void hand3(void *callback_data, void *client_data)
{
  Test test;
  
  test = (Test) callback_data;
  
  printf ("\nHello world, event3\n");
  printf("Callback data: %f %f %f\nClient data: %d\n", test->data1, 
	 test->data2, test->data3, (int32) client_data);
}

void main(int argc, char *argv[])
{
  HandlerList list=NULL;
  double      pp;
  _Test       test;
  
  list = CreateHandlerList(NUM_EVENTS);
  
  InstallHandler(list, hand1, EVENT1, NULL, FALSE);
  InstallHandler(list, hand2, EVENT2, (Pointer) 1000, FALSE);
  InstallHandler(list, hand3, EVENT3, (Pointer) 2000, FALSE);
  
  FireHandler(list, EVENT1, NULL);
  
  test.data1 = 100.0;
  test.data2 = 200.0;
  test.data3 = 300.0;
  
  FireHandler(list, EVENT3, &test);
  
  RemoveHandler(list, hand3, EVENT3);
  FireHandler(list, EVENT3, NULL);
}

#endif
