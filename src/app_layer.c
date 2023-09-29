#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>		/* standard symbolic constants and types */
#include <sys/un.h>
#include "../include/app_layer.h"
#include "mipd.h"

/**
 * Creates a unix socket, binds, and listen to it
 * socket_upper - pathname of the socket that the MIP daemon uses to communicate with upper layers
 * Returns the file descriptor of the unix socket.
 */
int prepare_unix_sock(char *socket_upper) {
    struct sockaddr_un addr;
    int sd = -1, rc = -1;

    /* 1. Create local UNIX socket of type SOCK_SEQPACKET. */

    sd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (sd == -1) {
        perror("socket()");
        return rc;
    }

    /*
     * For portability clear the whole structure, since some
     * implementations have additional (nonstandard) fields in
     * the structure.
     */

    memset(&addr, 0, sizeof(struct sockaddr_un));

    /* 2. Bind socket to socket name. */

    addr.sun_family = AF_UNIX;

    /* Why `-1`??? Check character arrays in C!
     * 's' 'e' 'r' 'v' 'e' 'r' '.' 's' 'o' 'c' 'k' 'e' 't' '\0'
     * sizeof() counts the null-terminated character ('\0') as well.
     */
    printf("%s", socket_upper);
    strncpy(addr.sun_path, socket_upper, sizeof(addr.sun_path) - 1);

    /* Unlink the socket so that we can reuse the program.
     * This is bad hack! Better solution with a lock file,
     * or signal handling.
     * Check https://gavv.github.io/articles/unix-socket-reuse
     */
    unlink(socket_upper);

    rc = bind(sd, (const struct sockaddr *) &addr, sizeof(addr));
    if (rc == -1) {
        perror("bind");
        close(sd);
        return rc;
    }

    /*
     * 3. Prepare for accepting incomming connections.
     * The backlog size is set to MAX_CONNS.
     * So while one request is being processed other requests
     * can be waiting.
     */

    rc = listen(sd, MAX_CONNS);
    if (rc == -1) {
        perror("listen()");
        close(sd);
        return rc;
    }

    return sd;
}
/**
 * reads incoming message from unix socket
 * fd: socket descriptor
 * buf: buffer to write message to
 * length: length of the message to read
 *  Return the number read, -1 for errors or 0 for EOF
 */
int handle_unix_socket(int fd, char* buf, int length)
{
    int rc;

    /* The memset() function fills the first 'sizeof(buf)' bytes
     * of the memory area pointed to by 'buf' with the constant byte 0.
     */
    memset(buf, 0, length);

    /* read() attempts to read up to 'sizeof(buf)' bytes from file
     * descriptor fd into the buffer starting at buf.
     */
    rc = read(fd, buf, length);
    if (rc <= 0) {
        close(fd);
        printf("<%d>: PING CLIENT/SERVER DISCONNECTED\n", fd);
        return -1;
    }

    printf("<%d>: %s\n", fd, buf);
    return rc;
}
