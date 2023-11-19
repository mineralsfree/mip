#include  <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef V2IMPL_MIP_H
#define V2IMPL_MIP_H


#define MAX_CONNS 1
#define MIP_TYPE_ARP 0x01
#define MIP_TYPE_PING 0x02
#define MIP_TYPE_ROUTING 0x04

#define ARP_REQ 0x00
#define ARP_RES 0x01

#define MIP_HDR_LEN sizeof(struct mip_hdr)

struct mip_hdr {
    uint8_t dst: 8;
    uint8_t src: 8;
    uint8_t ttl: 4;
    size_t sdu_l: 9;
    uint8_t sdu_t: 3;
}__attribute__((packed));

void mipd(char *unix_path, uint8_t mip_addr);


#endif //V2IMPL_MIP_H
