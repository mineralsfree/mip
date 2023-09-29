#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>        /* socket */
#include <arpa/inet.h>          /* htons      */
#include <linux/if_packet.h>    /* AF_PACKET  */
#include <ifaddrs.h>
#include <string.h>
#include <time.h>
#include <sys/epoll.h>          /* epoll */
#include "../include/utils.h"
#include "../include/mipd.h"
#include "errno.h"
#include "../include/mip_pdu.h"
#include "../include/mip_arp.h"
#include "../include/ether.h"
//#include "mip_pdu.c"

void printHexDump(char *buffer, size_t length) {
    for (size_t i = 0; i < length; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
}

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
    /* Note in man getifaddrs that this function dynamically allocates
       memory. It becomes our responsability to free it! */
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

void init_ifs(struct ifs_data *ifs, int rsock) {

    /* Get some info about the local ifaces */
    get_mac_from_ifaces(ifs);

    /* We use one RAW socket per node */
    ifs->rsock = rsock;

}

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

int epoll_add_sock(int raw_socket, int unix_socket) {
    struct epoll_event ev, unix_ev;

    /* Create epoll table */
    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }
    unix_ev.events = EPOLLIN;
    unix_ev.data.fd = unix_socket;
    /* Add RAW socket to epoll table */
    ev.events = EPOLLIN;
    ev.data.fd = raw_socket;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, raw_socket, &ev) == -1) {
        perror("epoll_ctl: raw_sock");
        exit(EXIT_FAILURE);
    }
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, unix_socket, &unix_ev) == -1) {
        perror("epoll_ctl: unix_sock");
        exit(EXIT_FAILURE);
    }
    return epollfd;
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


int send_unix_buff(int sd, int src_mip, char *str) {
    int rc;
    char buf[256];
    memset(buf, 0, sizeof(buf));
    memset(buf, src_mip, 1);
    strcpy(buf + 1, str);
    printf("sending %s, to unix socket %d, message Length: %lu", str, sd, strlen(str) + 1);
    fflush(stdout);
    rc = write(sd, buf, strlen(str) + 1);
    if (rc < 0) {
        perror("write");
        close(sd);
        exit(EXIT_FAILURE);
    }
    return rc;
}
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
    printf("  SDU Type: %u\n", mip_hdr->sdu_t);
    printf("  TTL: %u\n", mip_hdr->ttl);
}

int handle_mip_packet_v2(struct ifs_data *ifs) {
    struct sockaddr_ll so_name;
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

    /* Point to hello header */
    msgvec[1].iov_base = &mip_hdr;
    msgvec[1].iov_len = sizeof(struct mip_hdr);

    /* Point to ping/pong packet */
    msgvec[2].iov_base = (void *) packet;
    /* We can read up to 256 characters. Who cares? PONG is only 5 bytes */
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
        printf("\n Packet Type = ARP\n");
        struct mip_arp_sdu *sdu = (struct mip_arp_sdu *) malloc(sizeof(struct mip_arp_sdu));
        memcpy(sdu, packet, sizeof(struct mip_arp_sdu));

        printf("target mip: %d, sdu_type: %d\n", sdu->addr, sdu->type);
        if (sdu->addr == ifs->local_mip_addr && sdu->type == ARP_REQ) {
            printf("adding to arp table: \n");
            fflush(stdout);
            add_arp_entry(&ifs->arp_table, frame_hdr.src_mac, mip_hdr.src, interfaceIndex);
            sdu->type = ARP_RES;
            send_mip_packet_v2(ifs, ifs->addr[interfaceIndex].sll_addr, frame_hdr.src_mac, ifs->local_mip_addr,
                               mip_hdr.src,
                               (uint8_t *) sdu, MIP_TYPE_ARP, interfaceIndex);
        } else if (sdu->type == ARP_RES) {

            add_arp_entry(&ifs->arp_table, frame_hdr.src_mac, sdu->addr, interfaceIndex);
            if (ifs->pendingPacket != NULL & (uint8_t) ifs->pendingPacket[0] == sdu->addr) {
                send_mip_packet_v2(ifs, ifs->addr[interfaceIndex].sll_addr, frame_hdr.src_mac, ifs->local_mip_addr,
                                   mip_hdr.src,
                                   (uint8_t *) ifs->pendingPacket + 1, MIP_TYPE_PING, interfaceIndex);
                ifs->pendingPacket = NULL;
            }

        }
    } else if (mip_hdr.sdu_t == MIP_TYPE_PING) {
        send_unix_buff(ifs->usock, mip_hdr.src, (char *) packet);
    }
    printf("\n Recieving MIP packet \n");
    printMsgInfo(&msg);
    print_arp_content(&ifs->arp_table);
    printf("\n");
    return rc;
}


int send_mip_packet_v2(struct ifs_data *ifs,
                       uint8_t *src_mac_addr,
                       uint8_t *dst_mac_addr,
                       uint8_t src_mip_addr,
                       uint8_t dst_mip_addr,
                       uint8_t *packet, uint8_t sdu_t, int interfaceIndex) {
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
    } else {
        perror("wrong_mip_sdu_t");
        return -1;
    }
    mip_hdr.sdu_l = length;
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
    print_arp_content(&ifs->arp_table);
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


