#include <stdbool.h>
#include "../include/mipd.h"
//
// Created by mikita on 16/09/23.
//
#include <stdio.h>        /* standard input/output library functions */
#include <stdlib.h>        /* standard library definitions (macros) */
#include <unistd.h>        /* standard symbolic constants and types */
#include <string.h>        /* string operations (strncpy, memset..) */

#include <sys/epoll.h>          /* epoll */
#include <sys/socket.h>        /* sockets operations */
#include <sys/un.h>        /* definitions for UNIX domain sockets */
#include "../include/utils.h"
#include "../include/ether.h"
//#include "utils.c"

/**
 * Prepare (create, bind, listen) the server listening socket
 *
 * Returns the file descriptor of the server socket.
 */
static int prepare_server_sock(void) {
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
    strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);

    /* Unlink the socket so that we can reuse the program.
     * This is bad hack! Better solution with a lock file,
     * or signal handling.
     * Check https://gavv.github.io/articles/unix-socket-reuse
     */
    unlink(SOCKET_NAME);

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


void mipd(char *unix_adr, uint8_t mip_addr) {
    struct ifs_data local_if;
    int raw_sock, efd, rc;
    struct epoll_event  events[MAX_EVENTS];

    /* Set up a raw AF_PACKET socket without ethertype filtering */
    raw_sock = create_raw_socket();

    /* Initialize interface data */
    init_ifs(&local_if, raw_sock);
    /* Add socket to epoll table */
    efd = epoll_add_sock(raw_sock);
    print_mac_addr(local_if.addr[0].sll_addr, 6);
    if (strcmp(unix_adr, "c") == 0) {
        /* client mode */
        /* Send greeting to the server */
        uint8_t broadcast[] = ETH_DST_MAC;
        send_hip_packet(&local_if, local_if.addr[0].sll_addr, broadcast,
                        local_if.local_hip_addr, 0xff, "hi");
    }

    while (1) {
        rc = epoll_wait(efd, events, MAX_EVENTS, -1);
        if (rc == -1) {
            perror("epoll_wait");
        } else if (events->data.fd == raw_sock) {
            rc = handle_mip_packet(&local_if, unix_adr);
            if (rc < 0) {
                perror("handle_hip_packet");
                exit(EXIT_FAILURE);
            }
        }
    }
}