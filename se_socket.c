/************************************************
 * se_socket.c                                  *
 * function:                                  	*
 *    create a socket                        	*
 *    create a new thread,for receive function  *
 *    send message                             	*
 ************************************************/
#include "se_socket.h"
#include "msg_decoder.h"

socklen_t len;
bool from_host;
int sockfd;
int cl_sockfd;
struct sockaddr_in saddr,caddr;
bool socket_start;

void *build_socket(void *arg)
{
	int res;
	pthread_t recv_thread;
	pthread_attr_t thread_attr;
	len=sizeof(caddr);
	/* Set status of thread */
	res=pthread_attr_init(&thread_attr);
	if(res!=0)
	{
		printf("Setting detached attribute failed");
		exit(EXIT_FAILURE);
	}
	/* Create a socket */
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))<0)
	{
		printf("Socket Error");
		exit(1);
	}
	
	bzero(&saddr,sizeof(struct sockaddr));
	saddr.sin_family=AF_INET;
	saddr.sin_addr.s_addr=htonl(INADDR_ANY);
	saddr.sin_port=htons(PORT);
	if(bind(sockfd,(struct sockaddr *)&saddr,sizeof(struct sockaddr_in))==-1)
	{
		printf("Bind Error");
		exit(1);
	}
	/* Set the status of thread,don't wait for return of the subthread */
	res=pthread_attr_setdetachstate(&thread_attr,PTHREAD_CREATE_DETACHED);
	if(res!=0)
	{
		printf("Setting detached attribute failed");
		exit(EXIT_FAILURE);
	}
	if(listen(sockfd,5)<-1)
	{
		printf("Listen Error");
		exit(1);
	}    
        while(1)
	{
			cl_sockfd=accept(sockfd,(struct sockaddr *)&caddr,&len);
        	if (cl_sockfd < 0)
        	{
            	printf("Server Accept Failed!\n");
            	exit(EXIT_FAILURE);
        	}
           /* Create a sub thread,call recv_func() */
	       res=pthread_create(&recv_thread,&thread_attr,recv_func,NULL); 
	       if(res!=0)
	       {
		     printf("Thread create error");
		     exit(EXIT_FAILURE);
	       }
               from_host=true;
	       /* Callback the attribute of thread */
	       (void)pthread_attr_destroy(&thread_attr);
	}
}
/* Receive function */
void *recv_func(void *arg)
{
    int nu=0;
	char recv_text[MAXSIZE];
	while(1)
	{	
		nu=recv(cl_sockfd,recv_text,MAXSIZE,0);
        /* To Receive message from client and get the address infomation */
		if(nu<0)  
		{
			printf("--> Client disconnect!\n");
			close(cl_sockfd);
			from_host=false;
			pthread_exit(&recv_func);
		}
	   else if( from_host==true && nu>0)
		msg_decoder(recv_text);
		printf("-> %s\n",recv_text);
	}
}
/* Send function,send message to client */
void send_func(const char * send_text)
{
	/* If there is no text,continue */
	if(strlen(send_text)==1)
		return;
	if( !from_host ) printf("--> Waiting the client to connect!\n");
	/* Send message */
	if(from_host && send(cl_sockfd,send_text,strlen(send_text),0)<0)
	{
		printf("S send error");
		exit(1);
	}
}

void send_text()
{
	char *text;
	 /* Socket creating has succeed ,so send message */
	text=(char *)malloc(MAXSIZE);
    scanf("%s",text);
	if(text==NULL)
	{
		printf("Malloc error!\n");
		exit(1);
	}
	/* If there is no input,do nothing but return */
	if(strcmp(text,"")!=0)
	{
		send_func(text);
	}
	else
		printf("The message can not be empty ...\n");
	free(text);
}

void startsocket(void)
{
	//if( !socket_start )
	//{
	//	build_socket();
	//	socket_start=true;
	//	return;
	//}
	//printf("The server has been started !\n");

    int res;     
    pthread_t listen_thread;
    res=pthread_create(&listen_thread,NULL,build_socket,NULL); 
	if(res!=0)
	{
            printf("Thread create error");
            exit(EXIT_FAILURE);
	}
	printf("<*********************************>\n");
    printf("<                                 >\n");
    printf("<   Function: Se_socket start     >\n");
    printf("<     Company: SIEMENS of SH      >\n");
	printf("<          Editor: Mxin           >\n");
    printf("<   Data: %s %s    >\n", __DATE__, __TIME__);
    printf("<                                 >\n");
    printf("<*********************************>\n");
    return;
}
