#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "my_socket.h"
#include "readn.h"
#include "logUtil.h"
#include "set_timer.h"
#include "my_signal.h"

struct len_request_data {
    unsigned char signature;
    unsigned char dummy1;
    unsigned char dummy2;
    unsigned char dummy3;
    unsigned int le;
};

int usage()
{
    char msg[] = "Usage: ./sicat2 ip_address";
    fprintf(stderr, "%s\n", msg);

    return 0;
}

volatile sig_atomic_t has_alarm = 0;

void sig_alarm(int signo)
{
    has_alarm = 1;
    return;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        usage();
        exit(0);
    }
    char *ip_address = argv[1];
    int   port = 23;

    struct len_request_data len_request_data = { 0xa3, 0, 0, 0, htonl(4096*4) };

    int sockfd = tcp_socket();
    if (connect_tcp(sockfd, ip_address, port) < 0) {
        errx(1, "connect_tcp");
    }
    
    my_signal(SIGALRM, sig_alarm);

    set_timer(1, 0, 1, 0);

    int request_num        = 0;
    long interval_bytes    = 0;
    long interval_requests = 0;

    struct timeval start;
    gettimeofday(&start, NULL);
    for ( ; ; ) {
        int n;
        unsigned char len_buf[4];
        unsigned char data_buf[32*1024];

        if (has_alarm) {
            has_alarm = 0;
            struct timeval now, elapsed;
            gettimeofday(&now, NULL);
            timersub(&now, &start, &elapsed);
            printf("%ld.%06ld requests: %ld, bytes: %ld bytes\n",
                elapsed.tv_sec, elapsed.tv_usec,
                interval_requests, interval_bytes);
            interval_requests = 0;
            interval_bytes    = 0;
        }

        n = write(sockfd, &len_request_data, sizeof(len_request_data));
        if (n < 0) {
            err(1, "write len_request_data");
        }
        //fprintfwt(stderr, "write done\n");

        n = readn(sockfd, len_buf, sizeof(len_buf));
        if (n < 0) {
            err(1, "read len_buf");
        }
        if (n == 0) {
            /* EOF from remote */
            close(sockfd);
            exit(0);
        }
        //fprintfwt(stderr, "read len done\n");

        unsigned int *len = (unsigned int *)len_buf;
        unsigned data_len_word = ntohl(*len);
        int data_len = data_len_word * 2;
        
        //fprintfwt(stderr, "len: %u\n", data_len);

        //fprintfwt(stdout, "request # %d, %d bytes\n", request_num, data_len);
        request_num ++;
        interval_requests ++;
        if (data_len == 0) {
            continue;
        }
        else {
            n = readn(sockfd, data_buf, data_len);
            if (n < 0) {
                err(1, "read");
            }
            if (n == 0) {
                exit(0);
            }
            if (data_buf[0] != 0x5a && data_buf[0] != 0x5b) {
                warnx("invalid signature: %02x", data_buf[0]);
                for (int i = 0; i < 8; ++i) {
                    fprintf(stderr, "%02x\n", data_buf[i]);
                }
                exit(1);
            }
            interval_bytes += n;
        }
    }

    return 0;
}
