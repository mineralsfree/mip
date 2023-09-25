#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>		/* socket */
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
//#include "mip_pdu.c"

int create_raw_socket(void)
{
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
void get_mac_from_ifaces(struct ifs_data *ifs)
{
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
                   (struct sockaddr_ll*)ifp->ifa_addr,
                   sizeof(struct sockaddr_ll));
    }
    /* After the for loop, the address info of all interfaces are stored */
    /* Update the counter of the interfaces */
    ifs->ifn = i;

    /* Free the interface list */
    freeifaddrs(ifaces);
}
void init_ifs(struct ifs_data *ifs, int rsock)
{
    uint8_t rand_mip;

    /* Get some info about the local ifaces */
    get_mac_from_ifaces(ifs);

    /* We use one RAW socket per node */
    ifs->rsock = rsock;

    /* One MIP address per node; We name nodes and not interfaces like the
     * Internet does. Read about RINA Network Architecture for more info
     * about what's wrong with the current Internet.
     */

    srand(time(0));
    rand_mip = (uint8_t)(rand() % 256);

    ifs->local_hip_addr = rand_mip;
}
int epoll_add_sock(int sd)
{
    struct epoll_event ev;

    /* Create epoll table */
    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    /* Add RAW socket to epoll table */
    ev.events = EPOLLIN;
    ev.data.fd = sd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sd, &ev) == -1) {
        perror("epoll_ctl: raw_sock");
        exit(EXIT_FAILURE);
    }

    return epollfd;
}

/*
 * Print MAC address in hex format
 */
void print_mac_addr(uint8_t *addr, size_t len)
{
    size_t i;

    for (i = 0; i < len - 1; i++) {
        printf("%02x:", addr[i]);
    }
    printf("%02x\n", addr[i]);
}

int send_hip_packet(struct ifs_data *ifs,
                    uint8_t *src_mac_addr,
                    uint8_t *dst_mac_addr,
                    uint8_t src_hip_addr,
                    uint8_t dst_hip_addr,
                    const char *sdu)
{
    struct mip_pdu *pdu = alloc_pdu();
    uint8_t snd_buf[MAX_BUF_SIZE];

    if (NULL == pdu)
        return -ENOMEM;

    fill_pdu(pdu, src_mac_addr, dst_mac_addr, src_hip_addr, dst_hip_addr, sdu);

    size_t snd_len = mip_serialize_pdu(pdu, snd_buf);

    /* Send the serialized buffer via RAW socket */
    if (sendto(ifs->rsock, snd_buf, snd_len, 0,
               (struct sockaddr *) &(ifs->addr[0]),
               sizeof(struct sockaddr_ll)) <= 0) {
        perror("sendto()");
        close(ifs->rsock);
    }

    printf("Sending PDU with content (size %zu):\n", snd_len);
    print_pdu_content(pdu);

    destroy_pdu(pdu);
    return 0;
}


int handle_mip_packet(struct ifs_data *ifs, const char *app_mode)
{
    struct mip_pdu *pdu = (struct mip_pdu *)malloc(sizeof(struct mip_pdu));
    if (NULL == pdu) {
        perror("malloc");
        return -ENOMEM;
    }

    uint8_t rcv_buf[MAX_BUF_SIZE];

    /* Recv the serialized buffer via RAW socket */
    if (recvfrom(ifs->rsock, rcv_buf, MAX_BUF_SIZE, 0, NULL, NULL) <= 0) {
        perror("recvfrom()");
        close(ifs->rsock);
    }

    size_t rcv_len = mip_deserialize_pdu(pdu, rcv_buf);

    printf("Receiving PDU with content (size %zu) :\n", rcv_len);
    print_pdu_content(pdu);

    if (strcmp(app_mode, "s") == 0) {
        /* Server must greet the client back */
        send_hip_packet(ifs, ifs->addr[0].sll_addr, pdu->ethhdr->src_mac,
                        ifs->local_hip_addr, pdu->miphdr->src,
                        (const char *)pdu->sdu);
    }

    return 0;
}
