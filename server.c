/*************************************************
 *   server.c                                    *
 *   create the server for SIEMENS DustSensor    *
 *   send message to client          		     *
 *   send message to RaspberryPi                 *
 *************************************************/
#include "se_socket.h"
#include "conn_pi.h"
#include "wiringPi.h"

void Water_INT(void);
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
	wiringPiISR(8,INT_EDGE_FALLING, Water_INT);
	while(1)
	{
	//	send_text();
	}
    return 0;
}

void Water_INT(void)
{
	printf("Water INT\r\n");
	send_func("Alarm water");
}
