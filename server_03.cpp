//we implement event loop 
#include <vector>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

const size_t K_MAX_MSG = 4098;

enum {
    STATE_REQ = 0, STATE_RES, STATE_END
};

struct conn{
    int fd = -1;
    uint32_t state = 0;
    size_t read_buffer_size = 0;
    uint8_t read_buffer[4 + K_MAX_MSG]; //ehy 8 and not 32?
    size_t write_buffer_size = 0;
    size_t write_buffer_sent = 0;
    uint8_t write_buffer[4 + K_MAX_MSG];
};

static void msg(const char *msg){
	fprintf(stderr, "%s\n", msg);
}

static void die(const char *msg){
	int err = errno;
	fprintf(stderr, "[%d] %s\n", err, msg);
	abort();
}

static void fd_set_nb(int fd){
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if(errno){
        die("fcntl error");
        return;
    }
    flags |= O_NONBLOCK;    

    errno = 0;
    (void)fcntl(fd, F_SETFL, flags);
    if(errno){
        die("fcntl error");
    }
}
static void state_res(conn *conn);

static void conn_put(std::vector<conn *> &fd2conn, struct conn *conn){
    if(fd2conn.size()<=(size_t)conn->fd){
        fd2conn.resize(conn->fd + 1);
    }
    fd2conn[conn->fd] = conn;
}
static int32_t accept_new_conn(std::vector<conn *> &fd2conn, int fd){
    //accept
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr*)&client_addr, &socklen);
    if(connfd<0){
        msg("accept error");
        return -1;
    }

    //set new connection to nonblocking mode
    fd_set_nb(connfd);
    //create conn
    struct conn *conn = (struct conn *)malloc(sizeof(struct conn));
    if(!conn){
        close(connfd);
        return -1;
    }
    conn->fd = connfd;
    conn->state = STATE_REQ;
    conn->read_buffer_size = 0;
    conn->write_buffer_size = 0;
    conn->write_buffer_sent = 0;
    conn_put(fd2conn, conn);
    return 0;
}
static bool try_one_request(conn *conn){
    //parse one request from buffer
    if(conn->read_buffer_size < 4){ //not enough data currently, try later
        return false;
    }
    uint32_t len = 0;
    memcpy(&len, &conn->read_buffer[0], 4);
    if(len > K_MAX_MSG){
        msg("too long");
        conn->state = STATE_END;
        return false;
    }
    if(4 + len > conn->read_buffer_size){
        //not enough data
        return false;
    }

    //got one request, do something with it
    printf("client says: %.*s \n",len, &conn->read_buffer[4]);

    //generate echoing response
    memcpy(&conn->write_buffer[0], &len, 4);
    memcpy(&conn->write_buffer[4], &conn->read_buffer[4], len);
    conn->write_buffer_size = 4 + len;
    
    //remove request from buffer
    size_t remain = conn->read_buffer_size - 4 - len;
    if(remain){
        memmove(conn->read_buffer, &conn->read_buffer[4+len], remain); //frequent use of memmove is inefficient and should not be done
    }
    conn->read_buffer_size = remain;
    //change state
    conn->state = STATE_RES;
    state_res(conn);

    //continie outer loop if request is completly processed
    return (conn->state == STATE_REQ);
}

static bool try_flush_buffer(conn *conn){
    ssize_t rv = 0;
    do{
        size_t remain = conn->write_buffer_size - conn -> write_buffer_sent;
        rv = write(conn->fd, &conn->write_buffer[conn->write_buffer_sent], remain);
    } while ((rv < 0) && (errno = EINTR));
    if ((rv < 0) && (errno = EAGAIN)){ //why brackets?
        return false;
    }
    if(rv<0){
        msg("write() error");
        conn->state = STATE_END;
        return false;
    }
    conn->write_buffer_sent += (size_t)rv;
    assert(conn->write_buffer_sent <= conn -> write_buffer_size);
    if(conn->write_buffer_sent == conn -> write_buffer_size){
        //response fully sent
        conn->state = STATE_REQ;
        conn-> write_buffer_sent = 0;
        conn-> write_buffer_size = 0;
        return false;
    }
    return true;
}

static bool try_fill_buffer(conn *conn){
    //try to fill buffer
    assert(conn->read_buffer_size<sizeof(conn->read_buffer));
    ssize_t rv = 0;
    do{
        size_t cap = sizeof(conn->read_buffer) - conn->read_buffer_size;
        rv = read(conn->fd, &conn->read_buffer[conn->read_buffer_size], cap);
    } while ((rv<0)&&(errno = EINTR));
    if((rv<0)&&(errno = EAGAIN)){
        return false;
    }
    if(rv < 0){
        msg("read() error");
        conn->state = STATE_END;
        return false;
    }
    if(rv == 0){
        if(conn->read_buffer_size > 0){
            msg("unexpected EOF");
        } else {
            msg("EOF");
        }
        conn->state = STATE_END;
        return false;
    }

    conn->read_buffer_size += (size_t)rv;
    assert(conn->read_buffer_size <= sizeof(conn->read_buffer) - conn->read_buffer_size);

    //try to process request one by one
    while(try_one_request(conn)){}
    return(conn->state == STATE_REQ);
}

static void state_res(conn *conn){
    while (try_flush_buffer(conn)){}
}

static void state_req(conn *conn){
    while(try_fill_buffer(conn)){}
}

static void connection_io(conn *conn){
    if (conn->state == STATE_REQ){
        state_req(conn);
    } else if(conn->state = STATE_RES){
        state_res(conn);
    } else {
        assert(0);
    }
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

    //Map of all client conections
    std::vector<conn *> fd2conn;

    //set listen to non blocking mode
    fd_set_nb(fd);

    //THE event loop
    std::vector<struct pollfd> poll_args;
    while(true){
        //prepare args for poll()
        poll_args.clear();
        //we will put listening fd in first position
        struct pollfd pfd = {fd, POLLIN, 0};
        poll_args.push_back(pfd);
        //connection fds
        for(conn *conn : fd2conn){
            if(!conn){
                continue;
            }
            struct pollfd pfd = {};
            pfd.fd = conn->fd;
            pfd.events = (conn->state == STATE_REQ) ? POLLIN : POLLOUT;
            pfd.events  = pfd.events | POLLERR;
            poll_args.push_back(pfd);
        }

        //poll for active fds
        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), 1000);
        if(rv<0){
            die("poll");
        }
        //process active connections
        for(size_t i = 1; i < poll_args.size(); ++i){
            if(poll_args[i].revents){
                conn *conn = fd2conn[poll_args[i].fd];
                connection_io(conn);
                if(conn->state == STATE_END){
                    //client closed or something bad happened, anyways destrou connection
                    fd2conn[conn->fd] = NULL;
                    (void)close(conn->fd);
                    free(conn);
                }
            }
        }

        //try to accept new connection if listening fd is active
        if(poll_args[0].revents){
            (void)accept_new_conn(fd2conn, fd);
        }
    }
    return 0;
}
