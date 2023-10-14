#include "../include/mipd.h"
#include <stdio.h>        /* standard input/output library functions */
#include <stdlib.h>        /* standard library definitions (macros) */
#include <unistd.h>        /* standard symbolic constants and types */
#include <string.h>        /* string operations (strncpy, memset..) */

#include <sys/epoll.h>          /* epoll */
#include <sys/socket.h>        /* sockets operations */
#include "../include/utils.h"
#include "../include/ether.h"
#include "../include/app_layer.h"
#include "../include/mip_arp.h"

/**
 * Main MIP daemon function.
 *
 * This function initializes and manages the MIP daemon, which handles both raw packet
 * reception and communication over a Unix socket. It uses epoll to efficiently monitor
 * multiple file descriptors for events.
 *
 * unix_path: Pointer to the Unix socket .
 * mip_addr: The MIP address of the daemon.
 */
void mipd(char *unix_path, uint8_t mip_addr) {
    struct ifs_data local_if;
    memset(&local_if, 0, sizeof(struct ifs_data));
    for (int i = 0; i < 255; ++i) {
        local_if.pendingPackets[i] = NULL;
    }
    int unix_sock, accept_sd, raw_sock, efd, rc;
    struct epoll_event events[MAX_EVENTS];
    local_if.pendingPacket = NULL;
    char ping_request_buf[256];

    raw_sock = create_raw_socket();
    unix_sock = prepare_unix_sock(unix_path);
    /* Initialize interface data */
    init_ifs(&local_if, raw_sock);
    /* Add socket to epoll table */
    int epollfd = epoll_create1(0);

    efd = add_to_epoll_table(epollfd, unix_sock);
    efd = add_to_epoll_table(efd, raw_sock);
    local_if.local_mip_addr = mip_addr;

    while (1) {
        rc = epoll_wait(efd, events, MAX_EVENTS, -1);
        if (rc == -1) {
            perror("epoll_wait");
        } else if (events->data.fd == raw_sock) {
            printf("Recieved a RAW packet \n");
            rc = handle_mip_packet_v2(&local_if);
            if (rc < 0) {
                perror("handle_mip_packet");
                exit(EXIT_FAILURE);
            }
        } else if (events->data.fd == unix_sock) {
            //New unix socket connection
            printf("UNIX socket connected\n");
            accept_sd = accept(unix_sock, NULL, NULL);
            local_if.usock = accept_sd;
            if (accept_sd == -1) {
                perror("accept");
                continue;
            }
            rc = add_to_epoll_table(epollfd, accept_sd);
            if (rc == -1) {
                close(unix_sock);
                exit(EXIT_FAILURE);
            }
        } else {

            /* existing unix socket is sending data */
            int sdu_size = handle_unix_socket(events->data.fd, ping_request_buf, 256);

            //socket close
            if (sdu_size <=0){
                continue;
            }
            int dst_mip_address = (int)(unsigned char)ping_request_buf[0];
            printf("\nlocal arp table mip: %d  =  value: %d\n",dst_mip_address, local_if.arp_table.entries[dst_mip_address].mip_addr );
            /* mip address is in the arp table - we send PING packet*/
            if (local_if.arp_table.entries[dst_mip_address].mip_addr == dst_mip_address) {
                struct arp_entry entry = local_if.arp_table.entries[dst_mip_address];
                send_mip_packet_v2(&local_if, local_if.addr[entry.interfaceIndex].sll_addr,
                                   entry.hw_addr,
                                   local_if.local_mip_addr, dst_mip_address, (uint8_t *) ping_request_buf + 1,
                                   MIP_TYPE_PING, entry.interfaceIndex);
            } else {
                /* mip address is not in the arp table - we save the PING request and send ARP request packet*/
                local_if.pendingPackets[dst_mip_address] = (uint8_t *) ping_request_buf;
                uint8_t broadcast[] = ETH_DST_MAC;
                for (int i = 0; i < local_if.ifn; ++i) {
                    uint8_t *packet = (uint8_t *) fill_arp_sdu(dst_mip_address);
                    send_mip_packet_v2(&local_if, local_if.addr[i].sll_addr, broadcast,
                                       local_if.local_mip_addr, dst_mip_address, packet,
                                       MIP_TYPE_ARP, i);
                    free(packet);
                }
            }

        }
    }
}