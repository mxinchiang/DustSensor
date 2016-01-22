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
double dust = 0.0;
double dust_two = 0.0;
int dust_three = 0.0;
double PRESSURE = 0.0;
double acc_x=0.0;
double acc_y=0.0;
double acc_z=0.0;
double gps_n_high = 0.0;
double gps_n_low = 0.0;
double gps_e_high = 0.0;
double gps_e_low = 0.0;

int nFd;          /* Uart */
time_t timep;     /* time local */
struct tm *p;     /* time */
FILE *file_T=NULL;
FILE *file_H=NULL;
FILE *file_D=NULL;
FILE *file_P=NULL;
FILE *file_ACC=NULL;
FILE *file_GPS = NULL;
FILE *file_ALARM=NULL;

char txt_name[20];
sem_t sem_uart;

int interval = 1;
int temp_interval = 1;
int humi_interval = 1;
int dust_interval = 1;
int press_interval = 1;
int acc_interval = 1;
int gps_interval = 1;

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
			dust = (double)value/100.0;
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

		case CMD_READ_PRES:
			PRESSURE = (double)(value-500)/10.0;			
		break;
		
		case CMD_READ_ACC_X:
			acc_x=(double)value;
		break;

		case CMD_READ_ACC_Y:
			acc_y=(double)value;
		break;

		case CMD_READ_ACC_Z:
			acc_z=(double)value;
		break;
	
		case CMD_READ_GPS_N_HIGH:
			gps_n_high = (double)value;
		break;

		case CMD_READ_GPS_N_LOW :
			gps_n_low = (double)value;
		break;

		case CMD_READ_GPS_E_HIGH :
			gps_e_high = (double)value;
		break;

		case CMD_READ_GPS_E_LOW :
			gps_e_low = (double)value;
		break;

		default:
			printf("RECE. CMD is Error!\n");
		break;
	}
	return 1;
}

void dustsensor_uart()
{
	int temp_cnt = 0;
	int humi_cnt = 0;
	int dust_cnt = 0;
	int press_cnt = 0;
	int acc_cnt = 0;
	int gps_cnt = 0;
	double gps_e = 0.0;
	double gps_n = 0.0;

	char buff[6];
	char buff_nu=0;
	unsigned char number = 0;
	char cmd[13] = {CMD_READ_TEMP, CMD_READ_HUMI, CMD_READ_DUST1, CMD_READ_DUST2, CMD_READ_DUST3, CMD_READ_PRES,CMD_READ_ACC_X,CMD_READ_ACC_Y,CMD_READ_ACC_Z,CMD_READ_GPS_N_HIGH,CMD_READ_GPS_N_LOW,CMD_READ_GPS_E_HIGH,CMD_READ_GPS_E_LOW};
	int nRet = 0;
	char msg[64];
 	bzero(msg,sizeof(msg));
	bzero(buff, sizeof(buff));
    
	while(1)
    {
		/* wait for the sem of uart */
		sem_wait(&sem_uart);
		printf("sem is ok\n");
	
		for(number=0; number<sizeof(cmd); number++)
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
		printf("dust       ..%0.2lf\n", dust);
		printf("dust_two   ..%0.2lf\n", dust_two);
		printf("dust_three ..%d\n",     dust_three);
		printf("Pressure   ..%0.2f\n",  PRESSURE);
		printf("acc_x      ..%0.2lf\n", acc_x);
		printf("acc_y      ..%0.2lf\n", acc_y);
		printf("acc_z      ..%0.2lf\n", acc_z);
		printf("gps_e_high ..%0.2lf\n", gps_e_high);
		printf("gps_e_low  ..%0.2lf\n", gps_e_low);
		printf("gps_n_high ..%0.2lf\n", gps_n_high);
		printf("gps_n_low  ..%0.2lf\n", gps_n_low);

		gps_e = gps_e_high + gps_e_low / 10000;
		gps_n = gps_n_high + gps_n_low / 10000;

		temp_cnt++;
		humi_cnt++;
		dust_cnt++;
		press_cnt++;
		acc_cnt++;
		gps_cnt++;

		if (temp_cnt == temp_interval) 
		{
			if ((file_T = fopen("Temp.csv", "a")) == NULL)
			{
				printf("can not open file Temp.csv!\n");
			}
			fprintf(file_T, "%02d:%02d:%02d,%0.2lf\r", p->tm_hour, p->tm_min, p->tm_sec, temperature);
			fclose(file_T);
			temp_cnt = 0;
		}

		if (humi_cnt == humi_interval)
		{
			if ((file_H = fopen("Humi.csv", "a")) == NULL)
			{
				printf("can not open file Humi.csv!\n");
			}
			fprintf(file_H, "%02d:%02d:%02d,%0.2lf\r", p->tm_hour, p->tm_min, p->tm_sec, humidity);
			fclose(file_H);
			humi_cnt = 0;
		}

		if (dust_cnt == dust_interval)
		{
			if ((file_D = fopen("Dust.csv", "a")) == NULL)
			{
				printf("can not open file Dust.csv!\n");
			}
			fprintf(file_D, "%02d:%02d:%02d,%0.2lf\r", p->tm_hour, p->tm_min, p->tm_sec, dust);
			fclose(file_D);
			dust_cnt = 0;
		}

		if (press_cnt == press_interval)
		{
			if ((file_P = fopen("Press.csv", "a")) == NULL)
			{
				printf("can not open file Press.csv!\n");
			}
			fprintf(file_P, "%02d:%02d:%02d,%0.2lf\r", p->tm_hour, p->tm_min, p->tm_sec, PRESSURE);
			fclose(file_P);
			press_cnt = 0;
		}

		if (acc_cnt == acc_interval)
		{
			if ((file_ACC = fopen("Acc.csv", "a")) == NULL)
			{
				printf("can not open file Acc.csv!\n");
			}
			fprintf(file_ACC, "%02d:%02d:%02d,%0.2lf,%0.2lf,%0.2lf\r", p->tm_hour, p->tm_min, p->tm_sec, acc_x, acc_y, acc_z);
			fclose(file_ACC);
			acc_cnt = 0;
		}

		if (gps_cnt == gps_interval)
		{
			if ((file_GPS = fopen("GPS.csv", "a")) == NULL)
			{
				printf("can not open file GPS.csv!\n");
			}
			fprintf(file_GPS, "%02d:%02d:%02d,%0.2lf,%.2lf\r", p->tm_hour, p->tm_min, p->tm_sec, gps_e, gps_n);
			fclose(file_GPS);
			gps_cnt = 0;
		}

		sprintf(msg, "%02d:%02d:%02d %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f\n", 
					p->tm_hour, p->tm_min, p->tm_sec, temperature, humidity, dust, PRESSURE, acc_x, acc_y, acc_z, gps_e, gps_n);
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
	//sprintf(txt_name, "../TestFile/%d-%d-%d.txt", REF_TIME.year_ref, REF_TIME.mon_ref, REF_TIME.mday_ref);
	chdir("../");
	mkdir("LogFiles/",777);
	chdir("LogFiles/");
	sprintf(txt_name, "%d-%d-%d", REF_TIME.year_ref, REF_TIME.mon_ref, REF_TIME.mday_ref);
	mkdir(txt_name,777);
	chdir(txt_name);
	sprintf(txt_name, "Temp.csv");//Temp
	if( (file_T = fopen(txt_name, "w+")) == NULL)
	{
		printf("can not open file_T.!\n");
	}	
	//fprintf(fp, "DATE....\t\t\tTEMP\t\tHUMI\t\tDUST1\t\tDUST2\t\tDUST3\t\tPRESSURE\r\n");
	fprintf(file_T, "DATE,TEMP\r");
	fclose(file_T);

	sprintf(txt_name, "Humi.csv");//Humi
	if ((file_H = fopen(txt_name, "w+")) == NULL)
	{
		printf("can not open file_H.!\n");
	}
	fprintf(file_H, "DATE,HUMI\r");
	fclose(file_H);

	sprintf(txt_name, "Dust.csv");//Dust
	if ((file_D = fopen(txt_name, "w+")) == NULL)
	{
		printf("can not open file_D.!\n");
	}
	fprintf(file_D, "DATE,DUST\r");
	fclose(file_D);

	sprintf(txt_name, "Press.csv");//Press
	if ((file_P = fopen(txt_name, "w+")) == NULL)
	{
		printf("can not open file_P.!\n");
	}
	fprintf(file_P, "DATE,PRESSURE\r");
	fclose(file_P);

	sprintf(txt_name, "Acc.csv");//Acc
	if ((file_ACC = fopen(txt_name, "w+")) == NULL)
	{
		printf("can not open file_ACC.!\n");
	}
	fprintf(file_ACC, "DATE,ACC_X,ACC_Y,ACC_Z\r");
	fclose(file_ACC);

	sprintf(txt_name, "GPS.csv");//GPS
	if ((file_GPS = fopen(txt_name, "w+")) == NULL)
	{
		printf("can not open file_GPS.!\n");
	}
	fprintf(file_GPS, "DATE,GPS_E,GPS_N\r");
	fclose(file_GPS);

	sprintf(txt_name, "Alarm.csv");//Alarm
	if ((file_ALARM = fopen(txt_name, "w+")) == NULL)
	{
		printf("can not open file_ALARM.!\n");
	}
	fprintf(file_ALARM, "DATE,ALAEM\r");
	fclose(file_ALARM);

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

			sprintf(txt_name, "../LogFiles/%d-%d-%d", REF_TIME.year_ref, REF_TIME.mon_ref, REF_TIME.mday_ref);
			chdir("../");
			mkdir("LogFiles/", 777);
			chdir("LogFiles/");
			sprintf(txt_name, "%d-%d-%d", REF_TIME.year_ref, REF_TIME.mon_ref, REF_TIME.mday_ref);
			mkdir(txt_name,777);
			chdir(txt_name);
			sprintf(txt_name, "Temp.csv");//Temp
			if ((file_T = fopen(txt_name, "w+")) == NULL)
			{
				printf("can not open file_T.!\n");
			}
			//fprintf(fp, "DATE....\t\t\tTEMP\t\tHUMI\t\tDUST1\t\tDUST2\t\tDUST3\t\tPRESSURE\r\n");
			fprintf(file_T, "DATE,TEMP\r");
			fclose(file_T);

			sprintf(txt_name, "Humi.csv");//Humi
			if ((file_H = fopen(txt_name, "w+")) == NULL)
			{
				printf("can not open file_H.!\n");
			}
			fprintf(file_H, "DATE,HUMI\r");
			fclose(file_H);

			sprintf(txt_name, "Dust.csv");//Dust
			if ((file_D = fopen(txt_name, "w+")) == NULL)
			{
				printf("can not open file_D.!\n");
			}
			fprintf(file_D, "DATE,DUST\r");
			fclose(file_D);

			sprintf(txt_name, "Press.csv");//Press
			if ((file_P = fopen(txt_name, "w+")) == NULL)
			{
				printf("can not open file_P.!\n");
			}
			fprintf(file_P, "DATE,PRESSURE\r");
			fclose(file_P);

			sprintf(txt_name, "Acc.csv");//Acc
			if ((file_ACC = fopen(txt_name, "w+")) == NULL)
			{
				printf("can not open file_ACC.!\n");
			}
			fprintf(file_ACC, "DATE,ACC_X,ACC_Y,ACC_Z\r");
			fclose(file_ACC);

			sprintf(txt_name, "GPS.csv");//GPS
			if ((file_GPS = fopen(txt_name, "w+")) == NULL)
			{
				printf("can not open file_GPS.!\n");
			}
			fprintf(file_GPS, "DATE,GPS_E,GPS_N\r");
			fclose(file_GPS);

			sprintf(txt_name, "Alarm.csv");//Alarm
			if ((file_ALARM = fopen(txt_name, "w+")) == NULL)
			{
				printf("can not open file_ALARM.!\n");
			}
			fprintf(file_ALARM, "DATE,ALAEM\r");
			fclose(file_ALARM);
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
