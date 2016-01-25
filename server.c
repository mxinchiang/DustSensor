/*************************************************
 *   server.c                                    *
 *   create the server for SIEMENS DustSensor    *
 *   send message to client          		     *
 *   send message to RaspberryPi                 *
 *************************************************/
#include "se_socket.h"
#include "conn_pi.h"
#include "wiringPi.h"
struct tm *p;     /* time */

void Water_INT(void);
void Acc_INT(void);
int main(int argc,char **argv)
{
	startsocket();
	connect_to_pi();
	if (wiringPiSetup () == -1) 
	{
		printf("wiringPi Error\r\n");
		exit(1);
	}
	pullUpDnControl(8, PUD_UP);	
	pullUpDnControl(9, PUD_UP);
	wiringPiISR(8,INT_EDGE_FALLING, Water_INT);
	wiringPiISR(9,INT_EDGE_FALLING, Acc_INT);
	while(1)
	{
	//	send_text();
	}
    return 0;
}

void Water_INT(void)
{
	printf("Water INT\r\n");
	if ((file_ALARM = fopen("Alarm.csv", "a")) == NULL)
	{
		printf("can not open file Alarm.csv!\n");
	}
	fprintf(file_ALARM, "%02d:%02d:%02d,Flooding\r", p->tm_hour, p->tm_min, p->tm_sec);
	fclose(file_ALARM);
	send_func("Alarm water");
}

void Acc_INT(void)
{
	printf("Acc INT\r\n");
	if ((file_ALARM = fopen("Alarm.csv", "a")) == NULL)
	{
		printf("can not open file Alarm.csv!\n");
	}
	fprintf(file_ALARM, "%02d:%02d:%02d,Vibration\r", p->tm_hour, p->tm_min, p->tm_sec);
	fclose(file_ALARM);
	send_func("Alarm acc");
}
