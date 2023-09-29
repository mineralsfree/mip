
#include <stdio.h>        /* standard input/output library functions */
#include <stdlib.h>        /* standard library definitions (macros) */
#include <unistd.h>        /* standard symbolic constants and types */
#include <string.h>        /* string operations (strncpy, memset..) */
#include <sys/time.h>

#include <sys/epoll.h>    /* epoll */
#include <sys/socket.h>    /* sockets operations */
#include <sys/un.h>        /* definitions for UNIX domain sockets */
#include <errno.h>

#define TIMEOUT 1


/**
 * Entry point for the MIP ping_client program.
 *
 * This function handles the command-line arguments, creates a Unix domain socket,
 * sends a MIP PING packet containing a message to a specified destination host,
 * and prints the ping server's response along with roundtrip time.
 *
 * argc: The number of command-line arguments.
 * argv: An array of command-line argument strings.
 *
 * Returns 0 on success, or an error code on failure.
 */

int main(int argc, char *argv[]) {
    struct timeval send_time, recv_time;
    gettimeofday(&send_time, NULL);
    int opt, hflag = 0;
    struct sockaddr_un addr;
    int sd, rc;
    char buf[256];
    char *err;
    while ((opt = getopt(argc, argv, "h")) != -1) {
        if (opt == 'h') {
            hflag = 1;
        }
    }

    //errno - number of last error
    errno = 0;
    long mip_addr = strtol(argv[argc - 2], &err, 10);
    if (hflag || errno || err == argv[argc - 2] || mip_addr > 254 || mip_addr < 0 || argc < 3) {
        printf("Usage: %s "
               "[-h] prints help and exits the program <socket_lower> <destination_host> <message> ", argv[0]);
        return errno || 0;
    }
    char ping[] = "PING: ";
    memset(buf, 0, sizeof(buf));

    strcat(ping, argv[argc - 1]);
    strcpy(buf + 1, ping);
    buf[0] = (char) mip_addr;
    char *socketLower = argv[argc - 3];
    sd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (sd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketLower, sizeof(addr.sun_path) - 1);
    struct timeval tv;
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;
    setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof tv);
    rc = connect(sd, (struct sockaddr *) &addr, sizeof(addr));
    if (rc < 0) {
        perror("connect");
        close(sd);
        exit(EXIT_FAILURE);
    }


    rc = write(sd, buf, 256);

    if (rc < 0) {
        perror("write");
        close(sd);
        exit(EXIT_FAILURE);
    }
    printf("sent");
    // Receive and print the server's response
    rc = read(sd, buf, sizeof(buf) - 1);
    if (rc < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            perror("TIMEOUT");
        } else {
            perror("read");
        }
        close(sd);
        exit(EXIT_FAILURE);
    } else if (rc == 0) {
        printf("Server closed the connection.\n");
    } else {
        buf[rc] = '\0';  // Null-terminate the received data
        gettimeofday(&recv_time, NULL);
        long long roundtrip_time_us =
                (recv_time.tv_sec - send_time.tv_sec) * 1000000LL + (recv_time.tv_usec - send_time.tv_usec);
        printf("Received: %s from node with MIP %d\n", buf + 1, (uint8_t) buf[0]);
        printf("Roundtrip time: %lld microseconds\n", roundtrip_time_us);
    }

    close(sd);
}