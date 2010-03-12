#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "hokuyo_dev.h"

#define HOKUYO_MAXPACKET 1024
#define HOKUYO_RXRETRIES 10
// ms!
#define HOKUYO_CONNECT_WAIT_TIME 3000000
#define BAUDRATE B38400

const unsigned long hokuyoWaitTime=1000000;
const unsigned char packetHeader=0x2;
const unsigned char packetFooter=0x3;
const unsigned char connectCmd=0x26;
const unsigned char dataCmd=0x44;
const unsigned char connectRequest    [] = {0x02,0x26,0x31,0x30,0x30,0x33,0x30,0x03};
const unsigned int connectRequestSize=8;
const unsigned int connectRequestCoreSize=5;
const unsigned char disconnectRequest [] = {0x02,0x26,0x30,0x30,0x30,0x33,0x30,0x03};
const unsigned int disconnectRequestSize=8;
const unsigned int disconnectRequestCoreSize=5;
const unsigned char dataRequest    [] = {0x02,0x44,0x03};
const unsigned int dataRequestSize=3;
const unsigned int dataRequestHeaderSize=4;
const unsigned int dataReadingsPayloadSize=364;
const unsigned int readingSize=91;
const double hokuyoConversionFactor=.5*18.73703e-4/M_PI;


void parseData(double* readings,const  unsigned char * packet){
	const unsigned char * pptr=packet+2+dataRequestHeaderSize;
	unsigned int i;
	for (i=0; i<readingSize; i++){
		unsigned short d=0;
		unsigned int x;
		int j;
		for (j=3; j>=0; j--){
			unsigned short nibble=0;
			if (*pptr<'A')
				nibble=*pptr-'0';
			else
				nibble=10+*pptr-'A';
			d+=nibble<<(j<<2);
			//cout << *pptr << ":" << nibble << " ";
			pptr++;
		}
		x=d;
		if (x>=0xff00){
			x=0x0;
		}
		readings[i]=  hokuyoConversionFactor*(double)x;
	}
}

unsigned char checksum(unsigned int size, const unsigned char * packet){
	unsigned char c=0;
	unsigned int i;
	for (i=0; i<size; i++){
		c^=packet[i];
	}
	return c;
}

void writePacket(int fd, unsigned int size, const unsigned char * packet){
	/* XXX unsigned char check=*/ checksum(size, packet);
	write(fd, packet, size);
	unsigned char c=checksum(size,packet);
	write(fd, &c, 1);
}

void hokuyo_start(hokuyo_p hokuyo){
	struct termios oldtio, newtio;
	hokuyo->state=Disconnected;
	hokuyo->new_reading=0;
	hokuyo->num_readings=readingSize;
	hokuyo->connected=0;
	hokuyo->successfullConnection=0;
	hokuyo->timestamp=0;
	hokuyo->fd=open(hokuyo->device_name, O_RDWR, O_NOCTTY);
	tcgetattr(hokuyo->fd, &oldtio);
	memset(&newtio, 0, sizeof(newtio));
	newtio.c_cflag= BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_cc[VMIN]=1;
	newtio.c_cc[VTIME]=0;
	tcflush(hokuyo->fd, TCIFLUSH);
	tcsetattr(hokuyo->fd, TCSANOW, &newtio);
	printf("opening device %s\n", hokuyo->device_name);
	if (hokuyo->fd>0){
		hokuyo->opened=1;
		printf("device opened\n");
	}else{
		printf("error opening device\n");
	}
	
}
typedef enum {Header, Command, ConnectPayload, DataHeader, DataReadings, Footer, Checksum, Complete}
PacketState;

void hokuyo_run(hokuyo_p hokuyo){
	int retries=HOKUYO_RXRETRIES;
	unsigned char buf[HOKUYO_MAXPACKET];
	unsigned char * bptr =buf;
	unsigned int residualBytes=0;
	char c;
	hokuyo->new_reading=0;
	struct timeval timestamp;
	PacketState packetState=Header;
	switch (hokuyo->state){
		case Disconnected:
			writePacket(hokuyo->fd, connectRequestSize, connectRequest);
			//printf("N");
			break;
		case Connected:
			hokuyo->connected=1;
			writePacket(hokuyo->fd, dataRequestSize, dataRequest);
			//printf("K");
			break;
	}
	usleep(10000);
	while(!packetState==Complete || retries>0){
		struct timeval tv;
		tv.tv_sec=0;
		tv.tv_usec=hokuyoWaitTime;

		fd_set serialSet;
		FD_ZERO(&serialSet);
		FD_SET(hokuyo->fd, &serialSet);
		int result=select(hokuyo->fd+1, &serialSet, NULL, NULL, &tv);
		if (result){
			unsigned char b;
			read(hokuyo->fd, &b, 1);
			switch (packetState){
				case Header:
					if (b==packetHeader){
						*bptr++=b;
						packetState=Command;
					}
					//printf("h");
					break;
				case Command:
					if (b==connectCmd){
						*bptr++=b;
						packetState=ConnectPayload;
						residualBytes=connectRequestCoreSize;
					} else if (b==dataCmd){
						*bptr++=b;
						packetState=DataHeader;
						residualBytes=dataRequestHeaderSize;
					}
					//printf("c");
					break;
				case ConnectPayload:
					*bptr++=b;
					residualBytes--;
					if (residualBytes<=0)
						packetState=Footer;
					//printf("p");
					break;
				case DataHeader:
					*bptr++=b;
					residualBytes--;
					if (residualBytes<=0){
						packetState=DataReadings;
						residualBytes=dataReadingsPayloadSize;
					}
					//printf("H");
					break;
				case DataReadings:
					*bptr++=b;
					residualBytes--;
					if (residualBytes<=0)
						packetState=Footer;
					//printf("R");
					break;
				case Footer:
					if(b==packetFooter){
						*bptr++=b;
						packetState=Checksum;
					}else
						packetState=Header;
					//printf("F");
					break;
				case Checksum:
					c=checksum(bptr-buf, buf);
					if (c==b){
						//printf("cheksum ok\n");
						packetState=Complete;
					} else {
						//printf("cheksum error\n");
					}
						retries=0; // XXX ??
					//printf("S");
					break;
				default:
					bptr=buf;
					packetState=Header;
			} // switch(packetState)
		} else { // if(result)
			retries--;
			//printf("No answer from device in %f ms. Retries:  %d\n", hokuyoWaitTime, retries);
		}

		if (packetState==Complete) {
			hokuyo->successfullConnection = 1;
			if(buf[1]==connectCmd){
				if (buf[2]==0x30) {
					//cout  << "# Disconnected " << endl;
					hokuyo->state=Disconnected;
				} else {
					//cout << "# STATE CONN" << endl;
					hokuyo->state=Connected;
				}
			} else if(buf[1]==dataCmd){
				gettimeofday(&timestamp,0);
				hokuyo->timestamp=1000.*timestamp.tv_sec+1e-3*timestamp.tv_usec;
				parseData(hokuyo->range, buf);
				printf(".");
				fflush(stdout);
				hokuyo->new_reading=1;
				//printf("CAZZO SIII\n");
			}
		}

		if (!retries) {
		}
	} //	while(!packetState==Complete || retries>0){
}

void hokuyo_shutdown(hokuyo_p hokuyo){
	if (hokuyo->fd>=0){
		writePacket(hokuyo->fd, disconnectRequestSize, disconnectRequest);
	}
	close(hokuyo->fd);
}

