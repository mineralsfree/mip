
#include <stdio.h>		/* standard input/output library functions */
#include <stdlib.h>		/* standard library definitions (macros) */
#include <unistd.h>		/* standard symbolic constants and types */
#include <string.h>		/* string operations (strncpy, memset..) */

#include <sys/epoll.h>	/* epoll */
#include <sys/socket.h>	/* sockets operations */
#include <sys/un.h>		/* definitions for UNIX domain sockets */

#define TTL 0x06

/**
 * Entry point for the MIP ping server.
 *
 * argc: The number of command-line arguments.
 * argv: An array of command-line argument strings, where the last argument is the Unix domain socket path.
 *
 * Returns 0 on successful completion, or an error code on failure.
 */
int main(int argc, char* argv[]){
    struct sockaddr_un addr;
    int	   sd, rc;
    char   buf[256];
    char *socketUpper = argv[argc - 1];
    sd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (sd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketUpper, sizeof(addr.sun_path) - 1);
    rc = connect(sd, (struct sockaddr *)&addr, sizeof(addr));
    if ( rc < 0) {
        perror("connect");
        close(sd);
        exit(EXIT_FAILURE);
    }
    printf("Connected to unix socket %s\n", socketUpper);
    printf("Client: sending Identification code (0x02) to MIP daemon\n");
    char id_buf[1];
    id_buf[0] = 0x02;

    rc = write(sd, id_buf, 1);
//    usleep(10000000);  // Sleep for 100ms
//    usleep(10000000);  // Sleep for 100ms

    if (rc < 0) {
        perror("sending id code");
        close(sd);
        exit(EXIT_FAILURE);
    }
    fflush(stdout);

    // Receive and print the server's response
    while(1){
        rc = read(sd, buf, sizeof(buf) - 1);
        if (rc < 0) {
            perror("read");
            close(sd);
            exit(EXIT_FAILURE);
        } else if (rc == 0) {
            printf("mipd closed the connection.\n");
            close(sd);
            break;
        } else {
            int dst_mip_address = (uint8_t) buf[0];
            buf[rc] = '\0';  // Null-terminate the received data
            printf("Received: '%s' from %d, message length: %d\n", buf+2, dst_mip_address, rc);
            buf[4] = 'O';
            buf[1] = TTL;
            printf("Sending: '%s' to %d, message length: %d\n", buf+1, dst_mip_address, rc);
            write(sd, buf, 256);
        }
    }



    close(sd);
}