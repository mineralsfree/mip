
#include <stdio.h>        /* standard input/output library functions */
#include <stdlib.h>        /* standard library definitions (macros) */
#include <unistd.h>        /* standard symbolic constants and types */
#include <string.h>        /* string operations (strncpy, memset..) */
#include <sys/time.h>

#include <sys/epoll.h>    /* epoll */
#include <sys/socket.h>    /* sockets operations */
#include <sys/un.h>        /* definitions for UNIX domain sockets */
#include <errno.h>
#include "../include/routing_layer.h"

int main(int argc, char *argv[]) {
    uint8_t MY_MIP;
    struct sockaddr_un addr;
    struct routing_table rt;
    int sd, rc;
    char *socketLower = argv[argc - 1];
    char id_buf[1];
    char buff[245];
    char buf[245];
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketLower, sizeof(addr.sun_path) - 1);


    sd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (sd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    rc = connect(sd, (struct sockaddr *) &addr, sizeof(addr));
    if (rc < 0) {
        perror("connect");
        close(sd);
        exit(EXIT_FAILURE);
    }


    id_buf[0] = 0x04;
    rc = write(sd, id_buf, 1);
    printf("Sending identification code to MIP DAEMON (0x04)\n");

    rc = read(sd, buf, 1);
    if (rc != 1) {
        exit(EXIT_FAILURE);
    }
    MY_MIP = (int) (unsigned char) buf[0];
    printf("MY MIP: %d", MY_MIP);
    initialize_routing_table(&rt, sd, MY_MIP);
    fflush(stdout);

    char routing[] = "!HEL";
    strcpy(buff, routing);
    printf("Sending HELLO MESSAGE\n");

    rc = write(sd, buff, sizeof(buff));
    memset(buff, 0, sizeof(buff));
    while (1) {
        rc = read(sd, buff, sizeof(buff) - 1);
        if (rc < 0) {
            perror("read");
            close(sd);
            exit(EXIT_FAILURE);
        } else if (rc == 0) {
            printf("mipd closed the connection.\n");
            close(sd);
            break;
        } else {
            uint8_t source_addr = (uint8_t) buff[0];
            printf("RECIEVED: %s of length %lu from %d\n", buff, strlen(buff), source_addr);
            if (buff[1] == 'R' && buff[2] == 'E' && buff[3] == 'Q') {
                uint8_t dst_addr = (uint8_t) buff[4];
                printf("RECIEVED: REQUEST to NODE %d\n",dst_addr);
                struct routing_table_entry *entry = getRoutingTableEntry(&rt, dst_addr);
                char buffer[254];
                memset(buf, 0, sizeof(buf));
                char upd[] = "RSP";
                strcpy(buffer +  1, upd);
                buffer[0] = (char) dst_addr;
                if (entry != NULL) {
                    buffer[4] = (char) entry->next_hop;
                } else {
                    buffer[4] = (char) 255;
                }
                write(sd, buffer, sizeof(buffer));

            } else if (buff[1] == 'H' && buff[2] == 'E' && buff[3] == 'L') {
                printf("got HELLO message\n");
                uint8_t source_addr = (uint8_t) buff[0];
                send_table(&rt, source_addr);
                add_routing_tableEntry(&rt, source_addr, source_addr, 1);
                notify_change_table(&rt, source_addr);

            } else if (buff[1] == 'U' && buff[2] == 'P' && buff[3] == 'D') {
                uint8_t source_addr = (uint8_t) buff[0];
//                printf("got UPD from MIP: %s\n", buff);
                if (!getRoutingTableEntry(&rt, source_addr)) {
                    add_routing_tableEntry(&rt, source_addr, source_addr, 1);
                }
                update_routing_table(&rt, buff + 4, source_addr);
            }
        }
    }

    usleep(10000);  // Sleep for 100ms



}