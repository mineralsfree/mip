#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>        /* socket */
#include <arpa/inet.h>          /* htons      */
#include <linux/if_packet.h>    /* AF_PACKET  */
#include <ifaddrs.h>
#include <string.h>
#include <sys/epoll.h>          /* epoll */
#include "../include/utils.h"
#include "../include/mipd.h"
#include "../include/mip_pdu.h"
#include "../include/mip_arp.h"
#include <math.h>

/**
 * Creates a RAW socket for handling MIP packets.
 *
 * Returns the socket descriptor on success.
 * Exits the program with an error message on failure.
 */
int create_raw_socket(void) {
    int sd;
    short unsigned int protocol = ETH_P_MIP;

    /* Set up a raw AF_PACKET socket without ethertype filtering */
    sd = socket(AF_PACKET, SOCK_RAW, htons(protocol));
    if (sd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    return sd;
}

/*
 * This function stores struct sockaddr_ll addresses for all interfaces of the
 * node (except loopback interface)
 */
void get_mac_from_ifaces(struct ifs_data *ifs) {
    struct ifaddrs *ifaces, *ifp;
    int i = 0;

    /* Enumerate interfaces: */
    if (getifaddrs(&ifaces)) {
        perror("getifaddrs");
        exit(-1);
    }

    /* Walk the list looking for ifaces interesting to us */
    for (ifp = ifaces; ifp != NULL; ifp = ifp->ifa_next) {
        /* We make sure that the ifa_addr member is actually set: */
        if (ifp->ifa_addr != NULL &&
            ifp->ifa_addr->sa_family == AF_PACKET &&
            strcmp("lo", ifp->ifa_name))
            /* Copy the address info into the array of our struct */
            memcpy(&(ifs->addr[i++]),
                   (struct sockaddr_ll *) ifp->ifa_addr,
                   sizeof(struct sockaddr_ll));
    }
    /* After the for loop, the address info of all interfaces are stored */
    /* Update the counter of the interfaces */
    ifs->ifn = i;

    /* Free the interface list */
    freeifaddrs(ifaces);
}

/**
 * Initializes interface data.
 *
 * This function retrieves information about the local network interfaces.
 *
 * ifs: Pointer to the structure that holds interface data.
 * rsock: The raw socket descriptor to be associated with the daemon.
 */
void init_ifs(struct ifs_data *ifs, int rsock) {

    /* Get some info about the local ifaces */
    get_mac_from_ifaces(ifs);

    /* We use one RAW socket per node */
    ifs->rsock = rsock;

}

/**
 * Adds a file descriptor to the epoll event table for monitoring read events.
 *
 * efd: The epoll file descriptor representing the event table.
 * sd: The file descriptor to be added to the event table.
 *
 * Returns the epoll file descriptor on success.
 * Exits the program with an error message on failure.
 */
int add_to_epoll_table(int efd, int sd) {
    struct epoll_event ev;

    ev.events = EPOLLIN;
    ev.data.fd = sd;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, sd, &ev) == -1) {
        perror("epoll_ctl: unix_sock");
        exit(EXIT_FAILURE);
    }
    return efd;
}


/*
 * Print MAC address in hex format
 */
void print_mac_addr(uint8_t *addr, size_t len) {
    size_t i;

    for (i = 0; i < len - 1; i++) {
        printf("%02x:", addr[i]);
    }
    printf("%02x", addr[i]);
}

/**
 * Sends a message over a Unix socket.
 *
 *
 * sd: The Unix socket descriptor for sending the message.
 * src_mip: The source MIP address to be included in the message.
 * str: The string message to be sent.
 *
 * Returns the number of bytes written on success.
 * Exits the program with an error message on failure.
 */
int send_unix_buff(int sd, int src_mip, char *str) {
    int rc;
    char buf[256];
    memset(buf, 0, sizeof(buf));
    memset(buf, src_mip, 1);
    strcpy(buf + 1, str);
    printf("sending %s, to unix socket %d, message Length: %lu\n", str, sd, strlen(str) + 1);
    fflush(stdout);
    rc = write(sd, buf, strlen(str) + 1);
    if (rc < 0) {
        perror("write");
        close(sd);
        exit(EXIT_FAILURE);
    }
    return rc;
}

int send_routing_buff(int sd, int src_mip, int *buff) {
    int rc;
    char buf[256];
//    memset(buf, 0, sizeof(buf));
//    memset(buf, src_mip, 1);
//    strcpy(buf + 1, str);
//    printf("sending %s, to unix socket %d, message Length: %lu\n", str, sd, strlen(str) + 1);
    fflush(stdout);
    rc = write(sd, buff, 256);
    if (rc < 0) {
        perror("write");
        close(sd);
        exit(EXIT_FAILURE);
    }
    return rc;
}

int send_routing_request(int sd, uint8_t dst_mip, uint8_t src_mip) {
    int rc;
    char req[] = "REQ";
    char buf[254];
    memset(buf, 0, sizeof(buf));
    strcpy(buf + 2, req);
    buf[0] = (char) src_mip;
    buf[5] = (char) dst_mip;
//    printf("SENDING from %d to %d content %s\n", src_mip,dst_mip, buf);
    rc = write(sd, buf, sizeof(buf));
    return rc;
}

/**
 * Prints information from a message header (struct msghdr).
 *
 * msg: Pointer to the message header structure to be printed.
 */
void printMsgInfo(struct msghdr *msg) {
    printf("Message Information:\n");

    printf("  Source MAC Address: ");
    print_mac_addr(((struct eth_hdr *) msg->msg_iov[0].iov_base)->src_mac, 6);

    printf("  Target MAC Address: ");
    print_mac_addr(((struct eth_hdr *) msg->msg_iov[0].iov_base)->dst_mac, 6);

    printf("  Ethertype: 0x%04X\n", ((struct eth_hdr *) msg->msg_iov[0].iov_base)->ethertype);

    struct mip_hdr *mip_hdr = (struct mip_hdr *) msg->msg_iov[1].iov_base;
    printf("  Source MIP Address: %u\n", mip_hdr->src);
    printf("  Destination MIP Address: %u\n", mip_hdr->dst);
    printf("  SDU Length: %u\n", mip_hdr->sdu_l);
    printf("  SDU Type: %u (%s)\n", mip_hdr->sdu_t,
           (mip_hdr->sdu_t == 1) ? "MIP-ARP" : (mip_hdr->sdu_t == 2) ? "PING" : "ROUTING");
    printf("  TTL: %u\n", mip_hdr->ttl);
}

/**
 * Handles incoming MIP packets.
 *
 * This function receives and processes MIP packets from a raw socket, extracts
 * relevant information from the packet, and takes appropriate actions based on the
 * packet type. It manages Address Resolution Protocol (ARP) requests and responses
 * and sends PING requests to Unix sockets.
 *
 * ifs: Pointer to the structure that holds interface and socket data.
 *
 * Returns the number of bytes received on success, or -1 on failure.
 */
int handle_mip_packet_v2(struct ifs_data *ifs) {
    struct sockaddr_ll so_name = {0};
    struct eth_hdr frame_hdr;
    struct mip_hdr mip_hdr;
    struct msghdr msg = {0};
    struct iovec msgvec[3];
    uint8_t packet[256];
    int rc;
    uint8_t dst_mac[6];

    /* Point to frame header */
    msgvec[0].iov_base = &frame_hdr;
    msgvec[0].iov_len = sizeof(struct eth_hdr);

    /* Point to mip header */
    msgvec[1].iov_base = &mip_hdr;
    msgvec[1].iov_len = sizeof(struct mip_hdr);

    /* Point to ping/pong packet */
    msgvec[2].iov_base = (void *) packet;
    msgvec[2].iov_len = 256;

    /* Fill out message metadata struct */
    msg.msg_name = &so_name;
    msg.msg_namelen = sizeof(struct sockaddr_ll);
    msg.msg_iovlen = 3;
    msg.msg_iov = msgvec;
    for (int i = 0; i < ifs->ifn; i++) {
        if (ifs->addr[i].sll_ifindex == so_name.sll_ifindex)
            memcpy(dst_mac, ifs->addr[i].sll_addr, 6);
    }
    rc = recvmsg(ifs->rsock, &msg, 0);
    if (rc <= 0) {
        perror("sendmsg");
        return -1;
    }
    int interfaceIndex = -1;
    for (int i = 0; i < ifs->ifn; i++) {
        if (ifs->addr[i].sll_ifindex == so_name.sll_ifindex)
            interfaceIndex = i;
    }
    if (interfaceIndex == -1) {
        exit(interfaceIndex);
    }
    if (mip_hdr.sdu_t == MIP_TYPE_ARP) {
        struct mip_arp_sdu *sdu = (struct mip_arp_sdu *) malloc(sizeof(struct mip_arp_sdu));
        memcpy(sdu, packet, sizeof(struct mip_arp_sdu));

        if (sdu->addr == ifs->local_mip_addr && sdu->type == ARP_REQ) {
            printf("adding to arp table: \n");
            fflush(stdout);
            add_arp_entry(&ifs->arp_table, frame_hdr.src_mac, mip_hdr.src, interfaceIndex);
            sdu->type = ARP_RES;
            send_mip_packet_v2(ifs, ifs->addr[interfaceIndex].sll_addr, frame_hdr.src_mac, ifs->local_mip_addr,
                               mip_hdr.src,
                               (uint8_t *) sdu, MIP_TYPE_ARP, interfaceIndex, ARP_DEFAULT_TTL);
        } else if (sdu->type == ARP_RES) {
            add_arp_entry(&ifs->arp_table, frame_hdr.src_mac, sdu->addr, interfaceIndex);
            printf("\n\n\nARP RES DESTINGATION JPST: %d, pending pack: %s, pending ADDR: %d\n", mip_hdr.src,
                   ifs->pendingPackets[mip_hdr.src], ifs->pendingPackets[mip_hdr.src][0]);
            if (ifs->pendingPackets[mip_hdr.src] != NULL &&
                (uint8_t) ifs->pendingPackets[mip_hdr.src][0] == sdu->addr) {
                send_mip_packet_v2(ifs, ifs->addr[interfaceIndex].sll_addr, frame_hdr.src_mac, ifs->local_mip_addr,
                                   mip_hdr.src,
                                   (uint8_t *) ifs->pendingPackets[mip_hdr.src] + 1, MIP_TYPE_PING, interfaceIndex,
                                   ARP_DEFAULT_TTL);
                ifs->pendingPackets[mip_hdr.src] = NULL;
            }

        }
    } else if (mip_hdr.sdu_t == MIP_TYPE_PING) {
        printf("\n\n\nMIP_TYPE_PING REQEUST RECIEVED %s \n\n\n", (char *) packet);

        if (ifs->local_mip_addr == mip_hdr.dst) {
            send_unix_buff(ifs->usock, mip_hdr.src, (char *) packet);
        } else {
            if (mip_hdr.ttl != 1){
                printf("\n\n\nRESENDING  PING REQUEST \n\n\n");
                ifs->pendingPackets[mip_hdr.dst] = packet;
                memcpy(ifs->pending_packets[mip_hdr.dst].packet, packet, 255);
                ifs->pending_packets[mip_hdr.dst].src_mip = mip_hdr.src;
                ifs->pending_packets[mip_hdr.dst].ttl = mip_hdr.ttl;
                send_routing_request(ifs->routing_sock, mip_hdr.dst, ifs->local_mip_addr);
                fflush(stdout);
            } else {
                printf("\n\n\nDROPPED PACKET, REASON: TTL\n\n\n");

            }

        }
    } else if (mip_hdr.sdu_t == MIP_TYPE_ROUTING) {
        add_arp_entry(&ifs->arp_table, frame_hdr.src_mac, mip_hdr.src, interfaceIndex);

        send_routing_buff(ifs->routing_sock, mip_hdr.src, (int *) packet);
//
//        add_arp_entry(&ifs->arp_table, frame_hdr.src_mac, mip_hdr.src, interfaceIndex);
//        struct route_sdu *sdu = (struct route_sdu *) malloc(sizeof(struct route_sdu));
//        memcpy(sdu, packet, sizeof(struct route_sdu));
//
//        if (sdu->type == ROUTING_HELLO){
//
//        } else if (sdu->type == ROUTING_UPDATE){
//        }
//


    }
    printf("\n Recieving MIP packet \n");
    printMsgInfo(&msg);
    print_arp_content(&ifs->arp_table);
    return rc;
}

/**
 * Sends a MIP packet over a RAW socket.
 *
 * This function constructs and sends a MIP packet with the provided parameters.
 * It prepares the Ethernet and MIP headers, specifies the packet data, and sends
 * the packet over the RAW socket.
 *
 * ifs: Pointer to the structure that holds interface and socket data.
 * src_mac_addr: Pointer to the source MAC address.
 * dst_mac_addr: Pointer to the destination MAC address.
 * src_mip_addr: Source MIP address.
 * dst_mip_addr: Destination MIP address.
 * packet: Pointer to the packet data.
 * sdu_t: Type of the MIP SDU.
 * interfaceIndex: Index of the network interface.
 *
 * Returns the number of bytes sent on success, or -1 on failure.
 */

int send_mip_packet_v2(struct ifs_data *ifs,
                       uint8_t *src_mac_addr,
                       uint8_t *dst_mac_addr,
                       uint8_t src_mip_addr,
                       uint8_t dst_mip_addr,
                       uint8_t *packet, uint8_t sdu_t, int interfaceIndex, int ttl) {
    struct eth_hdr frame_hdr;
    struct mip_hdr mip_hdr;
    struct msghdr *msg;
    struct iovec msgvec[3];
    int rc;

    /* Fill in Ethernet header */
    memcpy(frame_hdr.dst_mac, dst_mac_addr, 6);
    memcpy(frame_hdr.src_mac, src_mac_addr, 6);
    /* Match the ethertype in packet_socket.c: */
    frame_hdr.ethertype = ETH_P_MIP;

    /* Fill in MIP header */
    mip_hdr.dst = dst_mip_addr;
    mip_hdr.src = src_mip_addr;
    mip_hdr.sdu_l = sizeof(*packet);
    mip_hdr.sdu_t = sdu_t;
    mip_hdr.ttl = ttl;

    /* Point to frame header */
    msgvec[0].iov_base = &frame_hdr;
    msgvec[0].iov_len = sizeof(struct eth_hdr);

    /* Point to mip header */
    msgvec[1].iov_base = &mip_hdr;
    msgvec[1].iov_len = sizeof(struct mip_hdr);

    /* Point to sdu  */
    msgvec[2].iov_base = (void *) packet;
    size_t length;
    if (sdu_t == MIP_TYPE_ARP) {
        length = sizeof(struct mip_arp_sdu);
    } else if (sdu_t == MIP_TYPE_PING) {
        length = strlen((const char *) packet) + 1;
    } else if (sdu_t == MIP_TYPE_ROUTING) {
        length = strlen((const char *) packet) + 1;
    } else {
        perror("wrong_mip_sdu_t");
        return -1;
    }
    mip_hdr.sdu_l = ceil(length / 4);
    msgvec[2].iov_len = length;

    /* Allocate a zeroed-out message info struct */
    msg = (struct msghdr *) calloc(1, sizeof(struct msghdr));

    /* Fill out message metadata struct */
    msg->msg_name = &(ifs->addr[interfaceIndex]);
    msg->msg_namelen = sizeof(struct sockaddr_ll);
    msg->msg_iovlen = 3;
    msg->msg_iov = msgvec;

    printf("Sending a MIP packet  \n");
    printMsgInfo(msg);
//    print_arp_content(&ifs->arp_table);
    /* Send message via RAW socket */
    rc = sendmsg(ifs->rsock, msg, 0);
    if (rc == -1) {
        perror("sendmsg");
        free(msg);
        return -1;
    }

    /* Remember that we allocated this on the heap; free it */
    free(msg);

    return rc;
}


