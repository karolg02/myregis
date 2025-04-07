#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

static void die(const char *msg){
    int error = errno;
    fprintf(stderr, "[%d] %s\n", error, msg);
    abort();
}

static int32_t read_full(int fd,char *buf, size_t n){
    while(n>0){
        ssize_t rv = read(fd, buf, n);
        if(rv <= 0){
            return -1;
        }
        assert((size_t)rv <= n);
        buf += rv;
    }
    return 0;
}

static int32_t one_request(int connfd){

}

static void do_something(int connfd) {
    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        die("read() error");
    }
    printf("client says: %s\n", rbuf);

    char wbuf[] = "Received";
    write(connfd, wbuf, strlen(wbuf));
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    //needed
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    //bind to ip
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);        //port
    addr.sin_addr.s_addr = htonl(0);    //wildcard IP 0.0.0.0
    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) { die("bind()"); }

    //listener
    rv = listen(fd, SOMAXCONN);
    if (rv) { die("listen()"); }

    while (true) {
        //accept
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
        if (connfd < 0) {
            continue;   //error
        }

        while(true){
            int32_t error = one_request(connfd);
            if(error) break;
        }
        close(connfd);
    }

    return 0;
}