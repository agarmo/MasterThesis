#ifndef _HOKUYO_DEV_H_
#define _HOKUYO_DEV_H_

#include <math.h>
#include <sys/time.h>

typedef enum {Disconnected, Connected} State;

typedef struct hokuyo_t{
	int num_readings;
	char device_name[1024];
	double range[1024];
	double timestamp;
	int fd;
	int opened;
	int running;
	int new_reading;
	int connected;
	int successfullConnection;
	State state;
	int flipped;
} hokuyo_t, *hokuyo_p;

void hokuyo_start(hokuyo_p hokuyo);
void hokuyo_run(hokuyo_p hokuyo);
void hokuyo_shutdown(hokuyo_p hokuyo);

#endif
