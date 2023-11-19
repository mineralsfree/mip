#ifndef MY_UTILS_H
#define MY_UTILS_H
#include <stdint.h>		/* uint8_t */
#include <unistd.h>		/* size_t */
#include <linux/if_packet.h>	/* struct sockaddr_ll */
#include <stdbool.h>


#define MAX_EVENTS 10
#define MAX_BUF_SIZE	1024

#define MAX_IF     3

struct arp_entry{
    uint8_t hw_addr[6];
    int interfaceIndex;
    uint8_t mip_addr;
};

struct arp_table {
    struct arp_entry entries[255];
};
struct pending_packets_entry {
    uint8_t src_mip;
    uint8_t packet[255];
};
struct ifs_data {
    struct sockaddr_ll addr[MAX_IF];
    struct arp_table arp_table;
    int rsock;
    int usock;
    int routing_sock;
    uint8_t local_mip_addr;
    int ifn;
    struct pending_packets_entry pending_packets[255];
    uint8_t *pendingPackets[255];
    uint8_t *pendingPacket;

};
int create_raw_socket(void);
int handle_mip_packet_v2(struct ifs_data *ifs);
int add_to_epoll_table(int efd, int sd);
void get_mac_from_ifaces(struct ifs_data *);
int send_routing_request(int sd, uint8_t dst_mip, uint8_t src_mip);
void print_mac_addr(uint8_t *addr, size_t len);
int send_unix_buff(int sd, int src_mip, char *str);
int send_mip_packet_v2(struct ifs_data *ifs,
                       uint8_t *src_mac_addr,
                       uint8_t *dst_mac_addr,
                       uint8_t src_mip_addr,
                       uint8_t dst_mip_addr,
                       uint8_t *packet,
                       uint8_t sdu_t, int interfaceIndex);


void init_ifs(struct ifs_data *, int rsock);
#endif /* MY_UTILS_H */
