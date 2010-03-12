#include "hokuyolaser.h"
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>



#define HK_QUIT  "\nQT\n"
#define HK_SCIP  "\nSCIP2.0\n"
#define HK_BEAM  "\nBM\n"
#define HK_RESET "\nRS\n"

//#define wcm(cmd) write (hokuyoLaser->fd,  cmd, strlen(cmd))
static inline void write_command(HokuyoLaser * hokuyoLaser, char * cmd){
  unsigned int bytes_written = write (hokuyoLaser->fd,  cmd, strlen(cmd));
  if (bytes_written!=strlen(cmd))
    carmen_warn("couldn't write the requested number of byes");
}


//parses an int of x bytes in the hokyo format
unsigned int parseInt(int bytes, char** s){
  char* b=*s;
  unsigned int ret=0;
  int j=bytes-1;
  int k=0;
  //int i=0;
  //while (i < bytes){
  for (int i=0; i<bytes;){
    k++;
    //printf("k = %i\n",k);
    if (*b=='\0'||*b=='\n'){
      *s='\0';
      return 0;
    }
    if (*(b+1)=='\n'){ //check for a wrapped line (? and checksum ?)
      b++;
      //printf("5\n");
    } else {
      unsigned char c=*b-0x30;
      ret+=((unsigned int ) c)<<(6*j);
      i++;
      j--;
    }
    b++;
  }
  *s=b;
  return ret;
}


// Parse data line, which is comprised of
//    i) Data block (up to 64 bytes)
//   ii) Sum (1 byte)
//  iii) LF
//   iv) LF (AT END OF REMAINING BYTES)
//unsigned int parseDataLine(


//skips a line
carmen_inline char* skipLine(char* buf){
  while (*buf!='\0' && *buf!='\n')
    buf++;
  return (*buf=='\n')?buf+1:0;
}

//parses a reading response
void hokuyo_parseReading(HokuyoRangeReading* r, char* buffer, carmen_laser_laser_type_t laser_type){
  char* s=buffer;
  int expectedStatus=0;

  if (s[0]=='M')
    expectedStatus=99;
  if (s[0]=='C')
    expectedStatus=00;

  int beamBytes=0;
  if (s[1]=='D')
    beamBytes=3;
  if (s[0]=='C') {
    beamBytes = 3;
    //printf("got C\n");
  }
  if (s[1] == 'S') {
    beamBytes = 2;
   // printf("got C\n");
  }
//  printf("s[0] = %c s[1] = %c\n",s[0],s[1]);

  if (! beamBytes || ! expectedStatus){
    fprintf(stderr, "Invalid return packet, cannot parse reading\n");
    r->status=-1;
    return;
  }
  s+=2; // jump to "Starting Step"
  char v[5];
  v[4]='\0';
  strncpy(v,s,4); r->startStep=atoi(v); s+=4; // grab StartStep and jump to EndStep
  strncpy(v,s,4); r->endStep=atoi(v);   s+=4; // grab EndStep and jump to ClusterCount
  v[2]='\0'; strncpy(v,s,2); r->clusterCount=atoi(v);

  //printf("startStep = %i, endStep = %i\n",r->startStep, r->endStep);

  s=skipLine(s); // skip over rest of line (ScanInterval, NumberOfScans, StringCharacters, and LF
  if (s==0){
    fprintf(stderr, "error, line broken when reading the range parameters\n");
    r->status=-1;
    return;
  }

  // When the status is 99: The second line starts with '99' (status),
  // followed by a 'b', a LF and the Hokuyo TimeStamp
  strncpy(v,s,2); r->status=atoi(v); s+=2;

  if (r->status==expectedStatus){
  } else {
    fprintf(stderr,"Error, Status=%d",r->status);
    return;
  }
  r->timestamp=parseInt(4,&s);
  s=skipLine(s); // skip over the rest of the line (Sum and LF)

  // The remaining lines begin with a 64B data block, followed by a sum and LF
  int i=0;
  int max_beams =0;
  if (laser_type==HOKUYO_URG)
    max_beams=URG_MAX_BEAMS;
  else if (laser_type==HOKUYO_UTM)
    max_beams=UTM_MAX_BEAMS;
  else{
    carmen_die("UNKNOWN hokuyo type!\n");
  }
  while(s!='\0'){
    if ( i>(max_beams)){
      printf("broke because of max_beams!\n"); //shouldn't happen
      break;
    }
    //r->ranges[i++]=parseInt(beamBytes,&s);
    r->ranges[i]=parseInt(beamBytes,&s);
    i++;
  }
  i--; //i is 1 more than the number we've actually read
  r->n_ranges=i;
}




unsigned int hokuyo_readPacket(HokuyoLaser* hokuyoLaser, char* buf, int bufsize, int faliures){
    int failureCount=faliures;
    if (hokuyoLaser->fd<=0){
        fprintf(stderr, "Invalid hokuyoLaser->fd\n");
        return -1;
    }

    memset(buf, 0, bufsize);
    int wasLineFeed=0;
    char* b=buf;
    int k=0;
    int bufLeft = bufsize;
    while (1){
        int c=read(hokuyoLaser->fd, b, bufLeft);
        if (! c){
            fprintf(stderr, "null" );
            usleep(25000);
            failureCount--;
        }else {
            for (int i=0; i<c; i++){
                k++;
                if (wasLineFeed && b[i]=='\n'){
                    b++;
                    return b-buf;
                }
                wasLineFeed=(b[i]=='\n');
            }
            b+=c;
            bufLeft -=c;
        }

        if (bufLeft<=0){
          fprintf(stderr,"no space left in buffer\n");
          usleep(25000);
          failureCount--;
        }
        if (failureCount<0){
          printf("failurCount<0\n");
          return 0;
        }
    }
}

unsigned int hokuyo_readStatus(HokuyoLaser* hokuyoLaser, char* cmd, int echo __attribute__((unused))){
    char buf[HOKUYO_BUFSIZE];
    write_command(hokuyoLaser,cmd);
    while (1){
        int c=hokuyo_readPacket(hokuyoLaser, buf, HOKUYO_BUFSIZE,10);
        if (c>0 && !strncmp(buf,cmd+1,strlen(cmd)-1)){
            //if (echo)
            //printf("%s",buf);
            char*s=buf;
            s=skipLine(s);
            char v[3]={s[0], s[1], 0};
            return atoi(v);
        }
    }

    return 0;

}




int hokuyo_open(HokuyoLaser* hokuyoLaser, const char* filename){
  hokuyoLaser->isProtocol2=0;
  hokuyoLaser->isInitialized=0;
  hokuyoLaser->isContinuous=0;
  hokuyoLaser->fd=open(filename, O_RDWR| O_NOCTTY | O_SYNC);
  return hokuyoLaser->fd;
}

int hokuyo_init(HokuyoLaser* hokuyoLaser){
    if (hokuyoLaser->fd<=0){
        return -1;
    }


#ifdef HOKUYO_ALWAYS_IN_SCIP20
  fprintf(stderr, "\nAssuming laser is already in SCIP2.0 mode!\n");
  fprintf(stderr, " (if the dirver hangs, either your Hokuyo has the old\n");
  fprintf(stderr, " firmware or you have to disable -DHOKUYO_ALWAYS_IN_SCIP20\n");
  fprintf(stderr, " in the Makefile of the laser driver)\n");

  hokuyoLaser->isContinuous=0;
  hokuyoLaser->isProtocol2=1;
#else

  // stop the  device anyhow

  fprintf(stderr, "Stopping the device... ");
  write_command(hokuyoLaser,HK_QUIT);
  write_command(hokuyoLaser,HK_QUIT);
  write_command(hokuyoLaser,HK_QUIT);
  fprintf(stderr, "done\n");
  hokuyoLaser->isContinuous=0;

  // The UTM-30LX functions only in SCIP2.0 Mode, as do some URG's
  fprintf(stderr, "Switching to enhanced mode (SCIP2.0)... ");
  fprintf(stderr, " (if the dirver hangs you probably configured your Hokuyo\n");
  fprintf(stderr, " so that SCIP is alwas on. IN this case, enable the Option\n");
  fprintf(stderr, " CFLAGS += -DHOKUYO_ALWAYS_IN_SCIP20\n");
  fprintf(stderr, " in the Makefile of the laser driver)\n");


  int status=hokuyo_readStatus(hokuyoLaser, HK_SCIP,1);
  if (status==0){
      fprintf(stderr, "\nSwitching to SCIP 2.0 was successful!\n");
      hokuyoLaser->isProtocol2=1;
  } else {
      fprintf(stderr, "Error. Unable to switch to SCIP2.0 Mode, please upgrade the firmware of your device.\n");
      return -1;
  }

#endif
  fprintf(stderr, "Device initialized successfully\n");
  hokuyoLaser->isInitialized=1;

  hokuyo_readStatus(hokuyoLaser, "\nVV\n", 1);
  hokuyo_readStatus(hokuyoLaser, "\nPP\n", 1);
  hokuyo_readStatus(hokuyoLaser, "\nII\n", 1);

  return 1;
}

int hokuyo_startContinuous(HokuyoLaser* hokuyoLaser, int startStep, int endStep, int clusterCount, int scanInterval){
    if (! hokuyoLaser->isInitialized)
        return -1;
    if (hokuyoLaser->isContinuous)
        return -1;

    // switch on the laser
    fprintf(stderr, "Switching on the laser emitter...  ");
    int status=hokuyo_readStatus(hokuyoLaser, HK_BEAM, 1);
    if (! status){
        fprintf(stderr, "Ok\n");
    } else {
        fprintf(stderr, "Error. Unable to control the laser, status is %d\n", status);
        return -1;
    }

    char command[1024];
    sprintf (command, "\nMD%04d%04d%02d%01d00\n", startStep, endStep, clusterCount,scanInterval);


    status=hokuyo_readStatus(hokuyoLaser, command, 0);
    if (status==99 || status==0){
        fprintf(stderr, "Continuous mode started with command %s\n", command);
        hokuyoLaser->isContinuous=1;
        return 1;
    }
    fprintf(stderr, "Error. Unable to set the continuous mode, status=%02d\n", status);

    return -1;
}


int hokuyo_stopContinuous(HokuyoLaser* hokuyoLaser){
    if (! hokuyoLaser->isInitialized)
        return -1;
    if (! hokuyoLaser->isContinuous)
        return -1;

    int status=hokuyo_readStatus(hokuyoLaser, HK_QUIT, 1);
    if (status==0){
        fprintf(stderr, "Ok\n");
        hokuyoLaser->isContinuous=0;
    } else {
        fprintf(stderr, "Error. Unable to stop the laser\n");
        return -1;
    }
    return 1;
}



int hokuyo_reset(HokuyoLaser* hokuyoLaser){
    if (! hokuyoLaser->isInitialized)
        return -1;

    int status=hokuyo_readStatus(hokuyoLaser, HK_RESET, 1);
    if (status==0){
        fprintf(stderr, "Ok\n");
        hokuyoLaser->isContinuous=0;
    } else {
        fprintf(stderr, "Error. Unable to reset laser\n");
        return -1;
    }
    return 1;
}

int hokuyo_close(HokuyoLaser* hokuyoLaser){
    if (! hokuyoLaser->isInitialized)
        return -1;
    hokuyo_stopContinuous(hokuyoLaser);
    close(hokuyoLaser->fd);
    hokuyoLaser->isProtocol2=0;
    hokuyoLaser->isInitialized=0;
    hokuyoLaser->isContinuous=0;
    hokuyoLaser->fd=-1;
    return 1;
}
