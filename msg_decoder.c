#include "msg_decoder.h"
#include "conn_pi.h"

int msg_decoder(char *recv_text)
{
	char *msg[2];
    char buf1[64];
	char buf2[64];
	char result[1024] = {'\0'};
	sprintf(result , "%s" , recv_text);
	sscanf(result , "%[^ ]%*s" , buf1);
	msg[0]=buf1;
	sscanf(result , "%*s %[^ ]" , buf2);
	msg[1]=buf2;
	if(strstr(msg[0],"change_interval")!=NULL)
	{
		int i;
		sscanf( msg[1], "%d", &i );
		interval = i;
	}
	return 0;
}
