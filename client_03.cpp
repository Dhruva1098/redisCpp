#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

const size_t K_MAX_MSG = 4096;

static void die(const char *msg){
	int err = errno;
	fprintf(stderr, "[%d] %s\n", err, msg);
	abort();
}

static void msg(const char *msg){
	fprintf(stderr, "%s\n", msg);
}

static int32_t read_full(int fd, char *buf, size_t n){
	while (n>0){
		ssize_t rv = read(fd, buf, n);
		if(rv<0){
			return -1;
		}
		assert((size_t)rv <= n);
		n -= (size_t)rv;
		buf += rv;
	}
	return 0;
}

static int32_t write_all(int fd, const char *buf, size_t n){
	while (n>0){
		ssize_t rv = write(fd, buf, n);
		if(rv<=0){
			return -1;
		}
		assert((size_t)rv <= n);
		n -= (size_t)rv;
		buf += rv;
	}
	return 0;
}
 
 //split query into send_req and read_res
static int32_t send_req(int fd, const char *text){
	uint32_t len = (uint32_t)strlen(text);
	if(len>K_MAX_MSG){
		return -1;
	}
	char write_buffer[4 + K_MAX_MSG];
	memcpy(write_buffer, &len, 4);
	memcpy(&write_buffer[4], text, len);
	return write_all(fd, write_buffer, 4 + len);
}

static int32_t read_res(int fd){
	char read_buffer[4 + K_MAX_MSG + 1];
	uint32_t len  = 0;
	errno = 0;
	int32_t err = read_full(fd, read_buffer, 4);
	if(err){
		if(errno == 0){
			msg("EOF");
		}
		else{
			msg("read() error");
		}
		return err;
	}
	memcpy(&len, read_buffer, 4);
	if(len>K_MAX_MSG){
		msg("too long");
		return -1;
	}

	err = read_full(fd, &read_buffer[4], len);
	if(err){
		msg("read() errpr");
		return err;
	}
	read_buffer[4 + len] = '\0';
	printf("server says: %s\n", &read_buffer[4]);
	return 0;
}

int main(){
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd<0){
		die("socket()");
	}
	struct sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = ntohs(1234);
	addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); //Loopback address 127.0.0.1
	int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
	if(rv){
		die("connect");
	}
	//multiple pipelined requests
	const char *query_list[5] = {"hello1","hello3","hello3","hello4","hello5"};
	for(size_t i = 0; i < 5; i++){
		int32_t err = send_req(fd, query_list[i]);
		if(err){
			goto DONE;
		}
	}
	for(size_t i = 0; i < 5; i++){
		int32_t err = read_res(fd);
		if(err){
			goto DONE;
		}
	}

DONE:
	close(fd);
	return 0;	
}
