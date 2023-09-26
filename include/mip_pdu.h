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

struct mip_pdu *alloc_pdu(void);

void fill_pdu(struct mip_pdu *pdu,
              uint8_t *src_mac_addr,
              uint8_t *dst_mac_addr,
              uint8_t src_hip_addr,
              uint8_t dst_hip_addr,
              uint8_t *sdu, uint8_t sdu_size);


size_t mip_deserialize_pdu(struct mip_pdu *, uint8_t *);

void print_pdu_content(struct mip_pdu *pdu);

size_t mip_serialize_pdu(struct mip_pdu *pdu, uint8_t *snd_buf);

void destroy_pdu(struct mip_pdu *);

#endif //V2IMPL_MIP_PDU_H
