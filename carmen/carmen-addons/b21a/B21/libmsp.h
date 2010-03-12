#ifndef _LIB_MSP_H_
#define _LIB_MSP_H_

typedef struct {
  int interval;
} mspIrParmsType;

typedef struct {
  int echoCount;		/* number of echos to capture */
  int echoTimeout;		/* time to wait for possible echos 1/150 sec */
  int fireDelay;		/* don't worry about this  */
  int fireInterval;		/* time between sets 1/150 sec */
  int echoBlankTime;		/* between echo blanking */
  int initialBlankTime;		/* ping to look for echo blanking */
  int startDelay;		/* don't worry about this either */
} mspSonParmsType;

typedef struct {
  int *validMspId;		/* list of valid MSP id nums */
				/* 0 terminated. */
  /* These are callbacks */
  int (*mspConnect)   (long mspNum, int connectFlag);
				/* 1 connected, 0 disconnected */

  int (*verRep)       (long mspNum, const char *verString);
  int (*dbgRepStr)    (long mspNum, const char *dbgString);

  int (*dbgRepBin)    (long mspNum, int len, const char *dbgData);
				/* binary data, don't worry about it */

  int (*bmpRep)       (long mspNum, unsigned long bumps);
				/* 1 bit per switch */

  int (*irRep)        (long mspNum, const unsigned long *irs);
				/* 0 terminated.  Currently 8 values */

  int (*irParms)      (long mspNum, const mspIrParmsType *parms);
				/* see mspIrParmsType struct above */

  int (*sonRep)       (long mspNum, const unsigned long *table[]);
				/* table[i][0] is transducer number */
				/* table[i][1] is echo 1 */
				/* table[i][2] is echo 2 */
				/* table[i][...] is echo ... */
				/* table[i][echoCount+1] is 0 */
				/* last table[i] is NULL */

  int (*sonTable)     (long mspNum, const unsigned long *table[]);
				/* table[0] = {xducer, xducer, xducer, 0} */
				/* table[i] = {xducer, xducer, xducer, 0} */
				/* table[last] = NULL */

  int (*sonParms)     (long mspNum, const mspSonParmsType *parms);
} mspOpsType;

/*
 * mspLibInit() returns fd which is used
 * to decide when to call mspSelect().
 */

#define devId2mspNum(a) (abdDev[(a)].devId.devNum)
int mspNum2devId (long mspNum);

int mspLibInit (const char *devFile, const mspOpsType *ops);
int mspLibSelect(void);

int mspReqReset       (long mspNum);
int mspReqVer         (long mspNum);
int mspReqIrParms     (long mspNum);
int mspSetIrParms     (long mspNum, const mspIrParmsType *parms);
int mspReqSonTable    (long mspNum);
int mspSetSonTable    (long mspNum, const unsigned long *table[]);
int mspReqSon         (long mspNum, const unsigned long *list);
				/* This is used to get single  */
				/* readings from a single MSP. */
				/* It is not typically used.   */
				/* See the mspterm.c courses   */
				/* for more info.              */

int mspReqSonStart    (long mspNum);
int mspReqSonStop     (long mspNum);
int mspReqSonParms    (long mspNum);
int mspSetSonParms    (long mspNum, const mspSonParmsType *parms);
				/* set any parms that you don't want */
				/* to change to -1 (0xFFFF) */

#endif /* _LIB_MSP_H_ */
