#ifndef _UTILS_H
#define _UTILS_H
#include <stdint.h>		/* uint8_t */
#include <unistd.h>		/* size_t */
#include <linux/if_packet.h>	/* struct sockaddr_ll */

#define MAX_EVENTS 10
#define MAX_BUF_SIZE	1024

#define MAX_IF     3
struct ifs_data {
    struct sockaddr_ll addr[MAX_IF];
    int rsock;
    uint8_t local_hip_addr;
    int ifn;
};
int create_raw_socket(void);

int epoll_add_sock(int sd);

void get_mac_from_ifaces(struct ifs_data *);

void print_mac_addr(uint8_t *addr, size_t len);
int handle_mip_packet(struct ifs_data *ifs, const char *app_mode);

int send_hip_packet(struct ifs_data *ifs,
                    uint8_t *src_mac_addr,
                    uint8_t *dst_mac_addr,
                    uint8_t src_hip_addr,
                    uint8_t dst_hip_addr,
                    const char *sdu);

void init_ifs(struct ifs_data *, int);
#endif /* _UTILS_H */
