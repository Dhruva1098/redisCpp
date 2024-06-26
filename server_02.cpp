//protocol parsing
//this code is not standard, and should not be done like this. However this is for sake of undrstanding things easier
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

static void msg(const char *msg){
	fprintf(stderr, "%s\n", msg);
}

static void die(const char *msg){
	int err = errno;
	fprintf(stderr, "[%d] %s\n", err, msg);
	abort();
}

static int32_t read_full(int fd, char *buf, size_t n){
    while(n>0){
        ssize_t rv = read(fd, buf, n);
        if(rv<=0){
            return -1;
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t write_all(int fd, const char *buf, size_t n){
    while(n>0){
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

static int32_t one_request(int connfd){
    char rbuffer[4 + K_MAX_MSG +1]; //4 bytes header
    errno = 0;
    int32_t err = read_full(connfd, rbuffer, 4);
    if(err){
        if (errno = 0){
            msg("EOF");
        }
        else{
            msg("read() error");
        }
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, rbuffer, 4);
    if(len > K_MAX_MSG){
        msg("too long");
        return -1;
    }

    //request body
    err = read_full(connfd, &rbuffer[4], len);
    if(err){
        msg("read() error");
        return err;
    }

    //do something
    rbuffer[4 + len] = '\0';
    printf("client says: %s\n", &rbuffer[4]);

    //reply
    const char reply[] = "world";
    char wbuffer[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wbuffer, &len, 4);
    memcpy(&wbuffer[4], reply, len);
    return write_all(connfd, wbuffer, 4 + len);
}

int main(){
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd<0){
		die("socket()");
	}	
	int val = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));  //set socket option
	struct sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = ntohs(1234);
	addr.sin_addr.s_addr = ntohl(0); 
	int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
	if (rv){
		die("bind()");
	}
	rv = listen(fd, SOMAXCONN);
	if(rv){
		die("listen()");
	}
	while(true){
    //accept
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
        // only serves one client connection at a time
        while(true){
            int32_t err = one_request(connfd);
            if(err){
                break;
            }
        }
        return 0;
	}
}

