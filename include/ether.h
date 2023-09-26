//
// Created by mikita on 24/09/23.
//

#ifndef V2IMPL_ETHER_H
#define V2IMPL_ETHER_H

#include <stdint.h>
#define ETH_MAC_LEN 6
#define ETH_DST_MAC {0xff, 0xff, 0xff, 0xff, 0xff, 0xff} // broadcast MAC addr.
#define ETH_P_HIP 0xFFFF
#define ETH_HDR_LEN sizeof(struct eth_hdr)

struct eth_hdr {
    uint8_t  dst_mac[ETH_MAC_LEN];
    uint8_t  src_mac[ETH_MAC_LEN];
    uint16_t ethertype;
    uint8_t  payload[];
} __attribute__((packed));

#endif //V2IMPL_ETHER_H
