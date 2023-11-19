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
        local_if.pending_packets[i].packet[0] = 0;
        local_if.pending_packets[i].src_mip = -1;
    }
    int unix_sock, accept_sd, raw_sock, efd, rc;
    struct epoll_event events[MAX_EVENTS];
    local_if.pendingPacket = NULL;
    char unix_incom_buf[256];
    char routing_local_buf[256];

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
            printf("\nUNIX socket connected\n");
            accept_sd = accept(unix_sock, NULL, NULL);
            printf("\nUNIX socket descriptor: %d, socket_d: %d\n", accept_sd, unix_sock);
            rc = read(accept_sd, unix_incom_buf, 1);
            if (rc == 1) {
//                printf("\n\nWARNIGN: %d Came from socket %d\n\n", (int)(unsigned char)unix_incom_buf[0], accept_sd);
                //routing socket
                if ((int) (unsigned char) unix_incom_buf[0] == 0x04) {
                    local_if.routing_sock = accept_sd;
                    write(accept_sd, &local_if.local_mip_addr, 1);
                } else if ((int) (unsigned char) unix_incom_buf[0] == 0x02) {
                    local_if.usock = accept_sd;
                }
            } else {
                perror("node_registering_itself");
                exit(EXIT_FAILURE);
            }
            printf("\nCURRENT STATE: \n usock:  %d \n routing_sock:  %d\n", local_if.usock, local_if.routing_sock);
            rc = add_to_epoll_table(epollfd, accept_sd);

//            local_if.usock = accept_sd;
            if (accept_sd == -1) {
                perror("accept");
                continue;
            }
            if (rc == -1) {
                close(unix_sock);
                exit(EXIT_FAILURE);
            }
        } else {
//            printf("\nCURRENT STATE: \n usock:  %d \n routing_sock:  %d\n, message from: %d\n", local_if.usock,
//                   local_if.routing_sock, events->data.fd);
            /* existing unix socket is sending data */
            if (events->data.fd == local_if.usock) {
                int sdu_size = handle_unix_socket(events->data.fd, unix_incom_buf, 256);
                if (sdu_size <= 0) {
                    continue;
                }
                int dst_mip_address = (int) (unsigned char) unix_incom_buf[0];
                send_routing_request(local_if.routing_sock, dst_mip_address, local_if.local_mip_addr);
//                printf("\nWRITTEN;%d\n", a);
//                printf("\nlocal arp table mip: %d  =  value: %d\n",dst_mip_address, local_if.arp_table.entries[dst_mip_address].mip_addr );
                /* mip address is in the arp table - we send PING packet*/
//                if (local_if.arp_table.entries[dst_mip_address].mip_addr == dst_mip_address) {
//                    struct arp_entry entry = local_if.arp_table.entries[dst_mip_address];
//                    send_mip_packet_v2(&local_if, local_if.addr[entry.interfaceIndex].sll_addr,
//                                       entry.hw_addr,
//                                       local_if.local_mip_addr, dst_mip_address, (uint8_t *) unix_incom_buf + 1,
//                                       MIP_TYPE_PING, entry.interfaceIndex);
//                } else {
                /* mip address is not in the arp table - we save the PING request and send ARP request packet*/
                local_if.pendingPackets[dst_mip_address] = (uint8_t *) unix_incom_buf;
                memcpy(local_if.pending_packets[dst_mip_address].packet , (uint8_t *) unix_incom_buf, sizeof(unix_incom_buf));
                local_if.pending_packets[dst_mip_address].src_mip = local_if.local_mip_addr;
//                    uint8_t broadcast[] = ETH_DST_MAC;
//                    for (int i = 0; i < local_if.ifn; ++i) {
//                        uint8_t *packet = (uint8_t *) fill_arp_sdu(dst_mip_address);
//                        send_mip_packet_v2(&local_if, local_if.addr[i].sll_addr, broadcast,
//                                           local_if.local_mip_addr, dst_mip_address, packet,
//                                           MIP_TYPE_ARP, i);
//                        free(packet);
//                    }
//                }
            } else if (events->data.fd == local_if.routing_sock) {
                int sdu_size = handle_unix_socket(events->data.fd, routing_local_buf, 256);
                if (sdu_size < 0) {
                    continue;
                }
                char *temp_str = (char *) routing_local_buf;
                if (temp_str[1] == 'R' && temp_str[2] == 'E' && temp_str[3] == 'S') {
                    int next_hop = (int) (unsigned char) routing_local_buf[4];
                    int dst_host = (int) (unsigned char) routing_local_buf[0];
                    if (local_if.pending_packets[dst_host].packet[0] != 0) {
                        struct arp_entry entry = local_if.arp_table.entries[next_hop];
                        printf("\n\n\n\n\nRESENDING CACHED: %s\n\n\n", (char * )local_if.pending_packets[dst_host].packet);
                        printf("\n\n\n\n\nDATA CACHE LENGTH: %lu  \n\n\n", strlen((char * )local_if.pending_packets[dst_host].packet));
                        printf("\n\n\n\n\nDST HOST: %d  \n\n\n", dst_host);

                        send_mip_packet_v2(&local_if, local_if.addr[entry.interfaceIndex].sll_addr,
                                           entry.hw_addr,
                                           local_if.pending_packets[dst_host].src_mip, dst_host, (uint8_t *) local_if.pending_packets[dst_host].packet,
                                           MIP_TYPE_PING, entry.interfaceIndex);
                        local_if.pending_packets[dst_host].packet[0]=0;
                        local_if.pending_packets[dst_host].src_mip = -1;

                    }

//                    printf("got response");
                } else if (temp_str[1] == 'H' && temp_str[2] == 'E' && temp_str[3] == 'L') {
                    routing_local_buf[0] = local_if.local_mip_addr;
                    uint8_t broadcast[] = ETH_DST_MAC;
                    for (int i = 0; i < local_if.ifn; ++i) {
                        printf("\n\n\n\n\nSending HELLO REQUEST BROADCAST: %s\n\n\n", routing_local_buf);

                        send_mip_packet_v2(&local_if, local_if.addr[i].sll_addr, broadcast,
                                           local_if.local_mip_addr, 255, (uint8_t *) routing_local_buf,
                                           MIP_TYPE_ROUTING, i);
                    }
                } else if (temp_str[1] == 'U' && temp_str[2] == 'P' && temp_str[3] == 'D') {
                    printf("\n\n\n\n\nSENDING UPD REQUEST RESPONSE: %s\n\n\n", routing_local_buf);
                    int dst_mip_address = (int) (unsigned char) routing_local_buf[0];
                    routing_local_buf[0] = (char) local_if.local_mip_addr;
                    struct arp_entry entry = local_if.arp_table.entries[dst_mip_address];
                    send_mip_packet_v2(&local_if, local_if.addr[entry.interfaceIndex].sll_addr,
                                       entry.hw_addr,
                                       local_if.local_mip_addr, dst_mip_address, (uint8_t *) routing_local_buf,
                                       MIP_TYPE_ROUTING, entry.interfaceIndex);
                }

//                printf("ROUTING");w
            }
        }
    }
}