#include "msg_decoder.h"
#include "conn_pi.h"

int msg_decoder(char *recv_text)
{
	int i;
	char *msg[2];
    char buf1[64];
	char buf2[64];
	char result[1024] = {'\0'};
	sprintf(result , "%s" , recv_text);
	sscanf(result , "%[^ ]%*s" , buf1);
	msg[0]=buf1;
	sscanf(result , "%*s %[^ ]" , buf2);
	msg[1]=buf2;
	if(strstr(msg[0],"changeTEMP")!=NULL)
	{
		sscanf( msg[1], "%d", &i );
		temp_interval = i;
	}
	if (strstr(msg[0], "changeHUMI") != NULL)
	{
		sscanf(msg[1], "%d", &i);
		humi_interval = i;
	}
	if (strstr(msg[0], "changeDUST") != NULL)
	{
		sscanf(msg[1], "%d", &i);
		dust_interval = i;
	}
	if (strstr(msg[0], "changePRESS") != NULL)
	{
		sscanf(msg[1], "%d", &i);
		press_interval = i;
	}
	if (strstr(msg[0], "changeACC") != NULL)
	{
		sscanf(msg[1], "%d", &i);
		acc_interval = i;
	}
	if (strstr(msg[0], "changeGPS") != NULL)
	{
		sscanf(msg[1], "%d", &i);
		gps_interval = i;
	}
	return 0;
}
