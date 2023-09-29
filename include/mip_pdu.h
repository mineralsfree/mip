//
// Created by mikita on 24/09/23.
//

#ifndef V2IMPL_MIP_PDU_H
#define V2IMPL_MIP_PDU_H

#include <stdint.h>
#include <stddef.h>
#include "ether.h"
#include "mipd.h"

struct mip_pdu {
    struct eth_hdr *ethhdr;
    struct mip_hdr *miphdr;
    uint16_t ethertype;
    uint8_t *sdu;
} __attribute__((packed));




#endif //V2IMPL_MIP_PDU_H
