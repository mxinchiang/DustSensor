all:server

OBJ=server.o se_socket.o conn_pi.o msg_decoder.o

# Option for development
CFLAGS= -g

# Option for release
#CFLAGS= -o 
 
server:server.o se_socket.o conn_pi.o msg_decoder.o
	gcc -Wall $(OBJ) $(CFLAGS) -o server  -l pthread
server.o:server.c se_socket.h conn_pi.h
	gcc -Wall $(CFLAGS) -c $<
se_socket.o:se_socket.c se_socket.h msg_decoder.h
	gcc -Wall $(CFLAGS) -c $<
conn_pi.o:conn_pi.c conn_pi.h
	gcc -Wall $(CFLAGS) -c $<
msg_decoder.o:msg_decoder.c msg_decoder.h conn_pi.h
	gcc -Wall $(CFLAGS) -c $<

clean:
	rm *.o server
