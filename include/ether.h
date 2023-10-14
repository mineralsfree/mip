#ifndef V2IMPL_ETHER_H
#define V2IMPL_ETHER_H

#include <stdint.h>
#define ETH_MAC_LEN 6
#define ETH_DST_MAC {0xff, 0xff, 0xff, 0xff, 0xff, 0xff} // broadcast MAC addr.
// For some unclear reason, changing this value from (RAW ETHERNET FRAME) 0xFFFF to (MIP) 0x88B5 causes loss of packages.
#define ETH_P_MIP 0xFFFF
#define ETH_HDR_LEN sizeof(struct eth_hdr)

struct eth_hdr {
    uint8_t  dst_mac[ETH_MAC_LEN];
    uint8_t  src_mac[ETH_MAC_LEN];
    uint16_t ethertype;
    uint8_t  payload[];
} __attribute__((packed));

#endif //V2IMPL_ETHER_H
