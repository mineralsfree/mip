#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>		/* standard library definitions (macros) */
#include <unistd.h>		/* standard symbolic constants and types */
#include "../include/app_layer.h"
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
        printf("<%d> left the chat...\n", fd);
        return -1;
    }

    printf("<%d>: %s\n", fd, buf);
    return rc;
}
