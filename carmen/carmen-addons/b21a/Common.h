/*****************************************************************************
 * PROJECT: Xavier
 *
 * (c) Copyright 1993 Richard Goodwin & Joseph O'Sullivan. All rights reserved.
 *
 * FILE: Common.h
 *
 * ABSTRACT:
 *
 * This file provides some common type and macro definitions.
 *
 *****************************************************************************/

#ifndef COMMON_LOADED
#define COMMON_LOADED

typedef int BOOLEAN;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef double DEGREES;
typedef double RADIANS;
typedef double METERS;
typedef double CMS;
typedef double CMS_PER_SEC;
typedef double CMS_PER_SEC_2;
typedef double DEGREES_PER_SEC;
typedef double DEGREES_PER_SEC_2;
typedef double RADIANS_PER_SEC;
typedef double RADIANS_PER_SEC_2;
typedef double FEET;
typedef double SECS;
typedef double AMPS;
typedef double VOLTS;

#define ABS(x)	           ((x) >= 0 ? (x) : -(x))
#define FABS(x) 	   ((x) >= 0.0 ? (x) : -(x))
#ifndef MAX
#define MAX(x,y)           ((x) > (y) ? (x) : (y))
#endif
#ifndef MIN
#define MIN(x,y)           ((x) > (y) ? (y) : (x))
#endif
#define SQR(x)             ((x) * (x))
#define NEAR(x1,x2,eps)    (ABS((x1)-(x2))<=(eps))
#define IRINT(x)           ((int) rint(x))
#define RAD_TO_DEG(r)      ((r) * 180.0 / M_PI)
#define DEG_TO_RAD(d)      ((d) * M_PI / 180.0)
#define FT2M               (12.0/39.37)
#define FEET_TO_METERS(ft) ((ft)*FT2M)
#define METERS_TO_FEET(ms) ((ms)/FT2M)
#define INCHES_TO_CMS(in)  (100.0*FEET_TO_METERS((in)/12.0))
#define CMS_TO_INCHES(cm)  (12.0*METERS_TO_FEET((cm)/100.0))

#define BIT_NULL         (0x00000000)
#define BIT_ZERO         (0x00000001) /*(1<<0)*/
#define BIT_ONE          (0x00000002) /*(1<<1)*/
#define BIT_TWO          (0x00000004) /*(1<<2)*/
#define BIT_THREE        (0x00000008) /*(1<<3)*/
#define BIT_FOUR         (0x00000010) /*(1<<4)*/
#define BIT_FIVE         (0x00000020) /*(1<<5)*/
#define BIT_SIX          (0x00000040) /*(1<<6)*/
#define BIT_SEVEN        (0x00000080) /*(1<<7)*/
#define BIT_EIGHT        (0x00000100) /*(1<<8)*/
#define BIT_NINE         (0x00000200) /*(1<<9)*/
#define BIT_TEN          (0x00000400) /*(1<<10)*/
#define BIT_ELEVEN       (0x00000800) /*(1<<11)*/
#define BIT_TWELVE       (0x00001000) /*(1<<12)*/
#define BIT_THIRTEEN     (0x00002000) /*(1<<13)*/
#define BIT_FOURTEEN     (0x00004000) /*(1<<14)*/
#define BIT_FIFTEEN      (0x00008000) /*(1<<15)*/
#define BIT_SIXTEEN      (0x00010000) /*(1<<16)*/
#define BIT_SEVENTEEN    (0x00020000) /*(1<<17)*/
#define BIT_EIGHTEEN     (0x00040000) /*(1<<18)*/
#define BIT_NINTEEN      (0x00080000) /*(1<<19)*/
#define BIT_TWENTY       (0x00100000) /*(1<<20)*/
#define BIT_TWENTY_ONE   (0x00200000) /*(1<<21)*/
#define BIT_TWENTY_TWO   (0x00400000) /*(1<<22)*/
#define BIT_TWENTY_THREE (0x00800000) /*(1<<23)*/
#define BIT_TWENTY_FOUR  (0x01000000) /*(1<<24)*/
#define BIT_TWENTY_FIVE  (0x02000000) /*(1<<25)*/
#define BIT_TWENTY_SIX   (0x04000000) /*(1<<26)*/
#define BIT_TWENTY_SEVEN (0x08000000) /*(1<<27)*/
#define BIT_TWENTY_EIGHT (0x10000000) /*(1<<28)*/
#define BIT_TWENTY_NINE  (0x20000000) /*(1<<29)*/
#define BIT_THIRTY       (0x40000000) /*(1<<30)*/
#define BIT_THIRTY_ONE   (0x80000000) /*(1<<31)*/

#endif
