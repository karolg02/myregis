//g++ -Wall -Wextra -O2 -g 03_server.cpp -o server

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

static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

static int32_t read_full(int fd,char *buf, size_t n){
    while(n>0){
        ssize_t rv = read(fd, buf, n);
        if(rv <= 0){
            return -1;
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t write_full(int fd, const char *buf, size_t n){
    while(n>0){
        ssize_t rv = write(fd,buf, n);
        if(rv<=0){
            return -1;
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

const size_t k_max_msg = 4096;

//connfd deskryptor
static int32_t one_request(int connfd) {
    // k_max_msg to maksymalna zawartosc wiadomosci, a 4 odpowiada za naglowek czyli ile danych przyjdzie
    char rbuf[4 + k_max_msg];
    //errno jest do obslugi bledow
    errno = 0;
    int32_t err = read_full(connfd, rbuf, 4);
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }
    uint32_t len = 0;
    memcpy(&len, rbuf, 4); //assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }
    //request body
    err = read_full(connfd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }
    //do something
    printf("client says: %.*s\n", len, &rbuf[4]);
    //reply using the same protocol
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    //memcpy kopiuje N bajtow z src do dst
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);
    return write_full(connfd, wbuf, 4 + len);
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

    //nasluchiwanie wiecej niz jednego requesta
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