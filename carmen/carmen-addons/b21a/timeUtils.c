/*****************************************************************************
 * PROJECT: TCA
 *
 * (c) Copyright Richard Goodwin, 1995. All rights reserved.
 *
 * FILE: timeUtils.c
 *
 * ABSTRACT:
 * 
 * This file provides a set for routines for manipulating time structures.
 *
 *****************************************************************************/

#include <sys/time.h>
#include "Common.h"
#include "timeUtils.h"

/******************************************************************************
 *
 * FUNCTION: lessTime (struct timeval *t1, struct timeval *t2)
 *
 * DESCRIPTION: 
 * Returns true if t1 is less than t2.
 *
 * INPUTS:
 *
 * OUTPUTS: 
 *
 *****************************************************************************/

BOOLEAN lessTime (struct timeval *t1, struct timeval *t2)
{
  return((t1->tv_sec < t2->tv_sec ||
	  (t1->tv_sec == t2->tv_sec &&
	   t1->tv_usec < t2->tv_usec)) ? TRUE : FALSE);
}

/******************************************************************************
 *
 * FUNCTION: greaterTime (struct timeval *t1, struct timeval *t2)
 *
 * DESCRIPTION: 
 * Returns true if t1 is greater than t2.
 *
 * INPUTS:
 *
 * OUTPUTS: 
 *
 *****************************************************************************/

BOOLEAN greaterTime (struct timeval *t1, struct timeval *t2)
{
  return((t1->tv_sec > t2->tv_sec ||
	  (t1->tv_sec == t2->tv_sec &&
	   t1->tv_usec > t2->tv_usec)) ? TRUE : FALSE);
}

/******************************************************************************
 *
 * FUNCTION: void addTime (struct timeval *t1,
 *                         struct timeval *t2,
 *                         struct timeval *result)
 *
 * DESCRIPTION: 
 * Adds t1 to t2 and stores the result in  result.
 *
 * INPUTS:
 *
 * OUTPUTS: 
 *
 *****************************************************************************/

void addTime(struct timeval *t1,
	     struct timeval *t2,
	     struct timeval *result)
{
  result->tv_usec = t1->tv_usec + t2->tv_usec;
  result->tv_sec = t1->tv_sec + t2->tv_sec;
  /* do a carry if needed */
  if (result->tv_usec > 1000000) {
    result->tv_usec -= 1000000;
    result->tv_sec += 1;
  }
}

/******************************************************************************
 *
 * FUNCTION: void subTime (struct timeval *t1, struct timeval *t2)
 *
 * DESCRIPTION: 
 * subtracts t2 from t1.
 *
 * INPUTS:
 *
 * OUTPUTS: 
 *
 *****************************************************************************/

void subTime(struct timeval *t1, struct timeval *t2)
{
  t1->tv_usec -= t2->tv_usec;
  t1->tv_sec -= t2->tv_sec;
  /* do a borrow if needed */
  if (t1->tv_usec < 0) {
    t1->tv_usec += 1000000;
    t1->tv_sec -= 1;
  }
}

/******************************************************************************
 *
 * FUNCTION: void printTime (struct timeval *t1, struct timeval *t2)
 *
 * DESCRIPTION: 
 * prints t.
 *
 * INPUTS:
 *
 * OUTPUTS: 
 *
 *****************************************************************************/

void printTime(FILE *fd, struct timeval *t)
{
  fprintf(fd," %ld.%06ld ",(long)(t->tv_sec), (long)(t->tv_usec));
}
