/*****************************************************************************
 * PROJECT: TCA
 *
 * (c) Copyright Richard Goodwin, 1995. All rights reserved.
 *
 * FILE: timeUtils.h
 *
 *****************************************************************************/

#ifndef TIMEUTIL_LOADED
#define TIMEUTIL_LOADED

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

void subTime(struct timeval *t1, struct timeval *t2);
void addTime(struct timeval *t1,
	     struct timeval *t2,
	     struct timeval *result);
BOOLEAN lessTime(struct timeval *t1, struct timeval *t2);
BOOLEAN greaterTime (struct timeval *t1, struct timeval *t2);
void printTime(FILE *fd, struct timeval *t);

#define TIME_MAX 0x7FFFFFFF

#ifdef __cplusplus
}
#endif

#endif /* TIMEUTIL_LOADED */
