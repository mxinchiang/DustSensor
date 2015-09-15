/***********************************
 * conn_pi.h                       *
 * the header files and functions  *
 ***********************************/
#ifndef __CONN_PI_H__
#define __CONN_PI_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <fcntl.h>

#define BAUDRATE B115200
#define DEVICE "/dev/ttyAMA0"
#define SIZE 1024

enum SEND_FORM
{
	SEND_ADDR = 0x00,
	SEND_CMD  = 0x01,
	SEND_CHECK= 0x02,
	SEND_END  = 0x03,
};

enum RECE_FORM
{
	RECE_ADDR = 0x00,
	RECE_CMD  = 0x01,
	RECE_DATAH= 0x02,
	RECE_DATAL= 0x03,
	RECE_CHECK= 0x04,
	RECE_END  = 0x05,
};

enum CMD_FORM
{
	CMD_READ_TEMP = 0x00,
	CMD_READ_HUMI = 0x01,
	CMD_READ_DUST1= 0x02,
	CMD_READ_DUST2= 0x03,
};

extern double temperature;
extern double humidity;
extern double dust_one;
extern double dust_two;
extern int interval;

int SerialInit();
void dustsensor_send_msg(int nFd, char addr, char cmd);
char dustsensor_rec_msg(char buff[6]);
extern void dustsensor_uart();
void dustsensor_time();
void dustsensor_flush();
int connect_to_pi();
#endif
