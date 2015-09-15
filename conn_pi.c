/************************************************
 * conn_pi.c                                    *
 * function:                                  	*
 *    connect to RasberryPi                     *
 *    Read the data and send commands           *
 ************************************************/
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>

#include "conn_pi.h"
#include "se_socket.h"
#define STREAM stdout

struct termios stNew;
struct termios stOld;
double temperature = 0.0;
double humidity = 0.0;
double dust_one = 0.0;
double dust_two = 0.0;
int    dust_three = 0;

int nFd;          /* Uart */
time_t timep;     /* time local */
struct tm *p;     /* time */
FILE *fp = NULL;  /* test file */
char txt_name[20];
sem_t sem_uart;

int interval = 1;

/* Open Port & Set Port */
int SerialInit()
{
	int nFd = 0;
    nFd = open(DEVICE, O_RDWR|O_NOCTTY|O_NDELAY);
    if(-1 == nFd)
    {
        perror("Open Serial Port Error!\n");
        return -1;
    }
	/* set uart as ublock */
    if( (fcntl(nFd, F_SETFL, 0)) < 0 )
    {
        perror("Fcntl F_SETFL Error!\n");
        return -1;
    }
	/* obtain the parameter of the terminal */
    if(tcgetattr(nFd, &stOld) != 0)
    {
        perror("tcgetattr error!\n");
        return -1;
    }

    stNew = stOld;
	/* set uart to Original pattern */
    cfmakeraw(&stNew);

    /* set speed */
    cfsetispeed(&stNew, BAUDRATE);
    cfsetospeed(&stNew, BAUDRATE);

    /* set databits */  
    stNew.c_cflag |= (CLOCAL|CREAD);
    stNew.c_cflag &= ~CSIZE;
    stNew.c_cflag |= CS8;

    /* set parity */
    stNew.c_cflag &= ~PARENB;  
    stNew.c_iflag &= ~INPCK;

    /* set stopbits */ 
    stNew.c_cflag &= ~CSTOPB;
    stNew.c_cc[VTIME]=0;	
    stNew.c_cc[VMIN]=1;	
    
	/* clear unfinished I/O data of the terminal 
	   <! TCIFLUSH   //clear Input
	   <! TCOFLUSH   //clear Output
	   <! TCIOFLUSH  //clear I/O
	*/
    tcflush(nFd,TCIOFLUSH);
	/* set terminal
	   <! TCSANOW
	   <! TCSDRAIN
	   <! TCSAFLUSH
	*/
    if( tcsetattr(nFd,TCSANOW,&stNew) != 0 )
    {
        perror("tcsetattr Error!\n");
        return -1;
    }
    return nFd;
}

void dustsensor_send_msg(int nFd, char addr, char cmd)
{
	char buff[4];
	int ret = 0;
	buff[SEND_ADDR] = addr;
	buff[SEND_CMD] = cmd;
	buff[SEND_CHECK] = buff[0]+buff[1];
	buff[SEND_END] = 0xFF;
	ret = write(nFd, buff, sizeof(buff));
	if(-1 == ret)
	{
		perror("uart write is error!\n");
		exit(1);
	}
}

char dustsensor_rec_msg(char buff[6])
{
	int value;
	if(buff[RECE_ADDR] != 0x5A)
	{
		printf("RECE. ADDR is Error!\n");
		return -1;
	}
	if(buff[RECE_END] != 0xFF)
	{
		printf("RECE. END is Error!\n");
		return -1;
	}
	if(buff[RECE_CHECK] != (char)(buff[RECE_ADDR] + buff[RECE_CMD] + buff[RECE_DATAH] + buff[RECE_DATAL]))
	{
		printf("RECE. CHECK is Error!\n");
		return -1;
	}
	value = (buff[RECE_DATAH] << 8) | buff[RECE_DATAL];
	
	switch(buff[RECE_CMD])
	{
		case CMD_READ_TEMP:
		#if 0
			printf("CMD_TEMP  .. %d\n", value);
		#endif
		    temperature = (double)value/100.0;
		break;

		case CMD_READ_HUMI:
		#if 0
			printf("CMD_HUMI  .. %d\n", value);
		#endif
			humidity = (double)value/100.0;
		break;

		case CMD_READ_DUST1:
		#if 0
			printf("CMD_DUST1 .. %d\n", value);
		#endif
			dust_one = (double)value/100.0;
		break;

		case CMD_READ_DUST2:
		#if 0
			printf("CMD_DUST2 .. %d\n", value);
		#endif
			dust_two = (double)value/100.0;
		break;

		case CMD_READ_DUST3:
			dust_three = value;
		break;

		default:
			printf("RECE. CMD is Error!\n");
		break;
	}
	return 1;
}

void dustsensor_uart()
{
	char buff[6];
	char buff_nu=0;
	unsigned char number = 0;
	char cmd[5] = {CMD_READ_TEMP, CMD_READ_HUMI, CMD_READ_DUST1, CMD_READ_DUST2, CMD_READ_DUST3};
	int nRet = 0;
	char msg[64];
 	bzero(msg,sizeof(msg));
	bzero(buff, sizeof(buff));
    
	while(1)
    {
		/* wait for the sem of uart */
		sem_wait(&sem_uart);
		printf("sem is ok\n");
	
		for(number=0; number<5; number++)
		{
			bzero(buff, sizeof(buff));
			buff_nu = 0;
			dustsensor_send_msg(nFd, 0x5A, cmd[number]);
			
			while(buff_nu != 6)
			{
				nRet = read(nFd, buff+buff_nu, sizeof(buff)-buff_nu);
				buff_nu += nRet;
			}
			
			dustsensor_rec_msg(buff);
		}

		printf("temperature..%0.2lf\n", temperature);		
		printf("humidity   ..%0.2lf\n", humidity);
		printf("dust_one   ..%0.2lf\n", dust_one);
		printf("dust_two   ..%0.2lf\n", dust_two);
		printf("dust_three ..%d\n",     dust_three);
		if( (fp = fopen(txt_name, "a")) == NULL)
		{
			printf("can not open file.!\n");
		}
		fprintf(fp, "%02d:%02d:%02d\t\t%0.2lf\t\t%0.2lf\t\t%0.2lf\t\t%0.2lf\t\t%d\r\n",
				    p->tm_hour, p->tm_min, p->tm_sec, temperature, humidity, dust_one, dust_two, dust_three);
		fclose(fp);
		sprintf(msg,"%02d:%02d:%02d %.2f %.2f %.2f %.2f %d\n",p->tm_hour, p->tm_min, p->tm_sec, temperature, humidity, dust_one, dust_two, dust_three);
        send_func(msg);
        sleep(1);
    }
}

void dustsensor_time()
{
	//char *wday[]={"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	int ref_sec;

	struct ref_time
	{
		int mday_ref;
		int mon_ref;
		int year_ref;
	}REF_TIME;

	time(&timep);
	p=localtime(&timep);
	printf("time at start is :\n");
	printf("%d-%d-%d\n", (1900+p->tm_year), (1+p->tm_mon), (p->tm_mday));

	ref_sec = p->tm_sec;
	REF_TIME.mday_ref = p->tm_mday;
	REF_TIME.mon_ref  = p->tm_mon+1;
	REF_TIME.year_ref = 1900+p->tm_year;
	
	printf("%d %d %d\n", REF_TIME.mday_ref, REF_TIME.mon_ref, REF_TIME.year_ref);
    /* Pay attention the default path */
	sprintf(txt_name, "../TestFile/%d-%d-%d.txt", REF_TIME.year_ref, REF_TIME.mon_ref, REF_TIME.mday_ref);

	if( (fp = fopen(txt_name, "w+")) == NULL)
	{
		printf("can not open file.!\n");
	}	
	fprintf(fp, "DATE....\t\t\tTEMP\t\tHUMI\t\tDUST1\t\tDUST2\t\tDUST3\r\n");
	fclose(fp);

	while(1)
    {
		time(&timep);
		p=localtime(&timep);
		printf("%d %d %d ", (1900+p->tm_year), (1+p->tm_mon), (p->tm_mday));
		printf("%d:%d:%d\n", p->tm_hour, p->tm_min, p->tm_sec);
	
		/* Whether need create a new file */
		if( REF_TIME.mday_ref != p->tm_mday )
		{
			REF_TIME.mday_ref = p->tm_mday;
			REF_TIME.mon_ref  = 1+p->tm_mon;
			REF_TIME.year_ref = 1900+p->tm_year;

			sprintf(txt_name, "../TestFile/%d-%d-%d.txt", REF_TIME.year_ref, REF_TIME.mon_ref, REF_TIME.mday_ref);
			if( (fp = fopen(txt_name, "w+")) == NULL)
			{
				printf("can not open file.!\n");
			}
			fprintf(fp, "DATE\t\t\tTEMP\t\tHUMI\t\tDUST1\t\tDUST2\t\tDUST3\n");
			fclose(fp);
		}

		/* sample data every 20 sec */
		if( abs(ref_sec - ((p->tm_sec > ref_sec)?(p->tm_sec):(60+p->tm_sec)) ) >= interval )
		{
			ref_sec = p->tm_sec;
			/* release the sem of uart */
			sem_post(&sem_uart);
		}

        sleep(1);       
    }
}

/* Flush the output stream */
void dustsensor_flush()
{
	while(1)
	{
		fflush(stdout);
		sleep(1);
	}
}

/* main functin to connect to pi */
int connect_to_pi()
{
    pthread_t uart_id;
    pthread_t time_id;
	pthread_t flush_id;
    int ret = 0;
  	
	/* Initialize the sem of uart */
	sem_init(&sem_uart, 0, 0);

	printf("<*********************************>\n");
    printf("<                                 >\n");
    printf("<     Function: Connect to Pi     >\n");
    printf("<     Company: SIEMENS of SH      >\n");
	printf("<          Editor: Tsui           >\n");
    printf("<   Data: %s %s    >\n", __DATE__, __TIME__);
    printf("<                                 >\n");
    printf("<*********************************>\n");

	printf("System is runing.\n");
	/* Initialize the Usart1 */
    nFd = SerialInit();
	if(-1 == nFd)
	{
		perror("Open uart is error!\n");
		return 0;
	}
    /* send and receive data from sensor */
	ret = pthread_create(&uart_id, NULL, (void *)dustsensor_uart, NULL);
    if(ret!=0)
    {
		printf("Creat pthread error!\n");
        return 0;
    }
	/* get time */
    ret = pthread_create(&time_id, NULL, (void *)dustsensor_time, NULL);
	/* clear read abd write buffer */
	ret = pthread_create(&flush_id, NULL, (void *)dustsensor_flush, NULL);

	return 0;
}
