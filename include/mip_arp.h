//
// Created by mikita on 25/09/23.
//
#include  <stdint.h>

#ifndef V2IMPL_MIP_ARP_H
#define V2IMPL_MIP_ARP_H

#include "mip_pdu.h"
#include "utils.h"

#define MIP_MAX_ENTRIES 255


struct mip_arp_sdu *fill_arp_sdu(uint8_t mip_addr);
void add_arp_entry(struct arp_table *table, uint8_t *hw_addr, uint8_t mip_addr, int interfaceIndex);
void print_arp_content(struct arp_table *table);
struct mip_arp_sdu {
    uint8_t type: 1;
    uint8_t addr: 8;
    uint32_t pad: 23;
}__attribute__((packed));

#endif //V2IMPL_MIP_ARP_H
