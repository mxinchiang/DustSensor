/*************************************************
 *   server.c                                    *
 *   create the server for SIEMENS DustSensor    *
 *   send message to client          		     *
 *   send message to RaspberryPi                 *
 *************************************************/
#include "se_socket.h"
#include "conn_pi.h"

int main(int argc,char **argv)
{
	startsocket();
	connect_to_pi();
	while(1)
	{
		send_text();
	}
    return 0;
}
