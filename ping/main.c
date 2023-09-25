
#include <stdio.h>		/* standard input/output library functions */
#include <stdlib.h>		/* standard library definitions (macros) */
#include <unistd.h>		/* standard symbolic constants and types */
#include <string.h>		/* string operations (strncpy, memset..) */

#include <sys/epoll.h>	/* epoll */
#include <sys/socket.h>	/* sockets operations */
#include <sys/un.h>		/* definitions for UNIX domain sockets */



int main(int argc, char* argv[]){
    struct sockaddr_un addr;
    int	   sd, rc;
    char   buf[256];
    char *socketUpper = argv[argc - 2];
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



        memset(buf, 0, sizeof(buf));

//        fgets(buf, sizeof(buf), stdin);
        strcpy(buf, "Hello from the other side");
        rc = write(sd, buf, strlen(buf));
        if (rc < 0) {
            perror("write");
            close(sd);
            exit(EXIT_FAILURE);
        }
    printf("sent");

    fgets(buf, sizeof(buf), stdin);
}