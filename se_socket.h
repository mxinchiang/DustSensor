/***********************************
 * se_socket.h                     *
 * the header files and functions  *
 ***********************************/
#ifndef __SE_SOCKET_H__
#define __SE_SOCKET_H__

#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <stdio.h> 
#include <errno.h> 
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#define bool _Bool
#define true 1
#define false 0
#define PORT 8888
#define MAXSIZE 1024
void *recv_func(void *arg);
void *build_socket(void *arg);
void send_text(void);
void send_func(const char *);
void startsocket(void);
#endif
