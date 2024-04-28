//this code is not standard, and should not be done like this. However this is for sake of undrstanding things easier
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>


/*
the typical workflow of a server goes something like this:
fd = socket()
bind(fd, address)
while true:
	conn_fd = accept(fd)
	do_something_with(conn_fd)
	close(conn_fd)
and for the client:
fd = socket()
connect(fd, address)
do_something_with(fd)
close(fd)
*/

static void msg(const char *msg){
	fprintf(stderr, "%s\n", msg);
}
static void die(const char *msg){
	int err = errno;
	fprintf(stderr, "[%d] %s\n", err, msg);
	abort();
}
static void do_something(int connfd) {
	char rbuffer[64] = {};
	ssize_t n = read(connfd, rbuffer, sizeof(rbuffer) - 1); //read upto sizeof(rbuf) - 1 bytes starting at rbuf
	if (n<0){
		msg("read() error");
		return;
	}
	printf("client says: %s\n", rbuffer);	
		
	char wbuffer[] = "world";
	write(connfd, wbuffer, strlen(wbuffer));
}
int main(){
	
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd<0){
		die("socket()");
	}	
	
	//fd is like an integer that reffers to something in kernel, like port or tcp or disk file etc.
	// AF_INET is for ipv4, you can use AF_INET6 for ipv6 socket
	// SOCK_STREAM is for TCP
	//lets introduce a new sysCall
	//needed for most server applications
	int val = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)); 
	//setsockopt() configures a socket. SO_REUSEADDR server binds to same address if restarted
	//now lets bind and listen, using address 0.0.0.0:1234
	struct sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = ntohs(1234);
	addr.sin_addr.s_addr = ntohl(0); //0.0.0.0
	int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
	if (rv){
		die("bind()");
	}
	//listen 
	rv = listen(fd, SOMAXCONN);
	if(rv){
		die("listen()");
	}

	//do something (simple read and write while(true){
	while(true){

	struct sockaddr_in client_addr = {};
	socklen_t socklen = sizeof(client_addr);
	int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
	do_something(connfd);
	close(connfd);
	return 0;
	}
}













		



