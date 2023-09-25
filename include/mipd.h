//
// Created by mikita on 16/09/23.
//
#include  <stdint.h>
#include <stdbool.h>

#ifndef V2IMPL_MIP_H
#define V2IMPL_MIP_H
#include <stddef.h>


#define SOCKET_NAME "mipd.socket"
#define MAX_CONNS 1
#define MIP_TYPE_ARP 0x01
#define MIP_TYPE_PING 0x02
#define ETH_P_MIP 0x88B5
#define MIP_DST_ADDR    0xff

#define MIP_HDR_LEN sizeof(struct mip_hdr)

struct mip_hdr {
    uint8_t dst: 8;
    uint8_t src: 8;
    uint8_t ttl: 4;
    size_t sdu_l: 9;
    uint8_t sdu_t: 3;
};

void mipd(char *str, uint8_t mip_addr);


#endif //V2IMPL_MIP_H
