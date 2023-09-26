//
// Created by mikita on 25/09/23.
//
#include <malloc.h>
#include <netinet/in.h>
#include <string.h>
#include "../include/mip_arp.h"


void print_arp_content(struct arp_table *table) {
    printf("%s", "Arp table content: \n");
    printf("%s", "IP Address\t\tMAC Adress\n");
    for (int i = 0; i < MIP_MAX_ENTRIES; ++i) {
        struct arp_entry *entry = &table->entries[i];
        if (entry->mip_addr) {
            printf("%d\t\t", entry->mip_addr);
            print_mac_addr(entry->hw_addr, 6);
        }
    }
}
void add_arp_entry(struct arp_table *table, uint8_t *hw_addr, uint8_t mip_addr) {
    struct arp_entry *entry = &table->entries[mip_addr];
    memcpy(entry->hw_addr, hw_addr, ETH_MAC_LEN);
    entry->mip_addr = mip_addr;
    print_arp_content(table);
}

struct mip_arp_sdu *fill_arp_sdu(uint8_t mip_addr){
    struct mip_arp_sdu* sdu = (struct mip_arp_sdu *) malloc(sizeof (struct mip_arp_sdu));
    memset(sdu, 0, sizeof(struct mip_arp_sdu));
    sdu->type = 0x00;
    sdu->addr = mip_addr;
    return sdu;
}

int handle_arp_packet(struct ifs_data *ifs, struct mip_pdu *pdu) {
    uint8_t *sdu = pdu->sdu;
    struct mip_arp_sdu *arp_sdu = (struct mip_arp_sdu *) malloc(sizeof(struct mip_arp_sdu));
    uint32_t *tmp = (uint32_t *) (sdu);

    uint32_t sdu_content = ntohl(*tmp);
    arp_sdu->type = (uint8_t) (sdu_content >> 31);
    arp_sdu->addr = (uint8_t) ((sdu_content >> 23) & 0xFF);
    if (arp_sdu->type == 0x01) {
        add_arp_entry(&ifs->arp_table, pdu->ethhdr->src_mac, arp_sdu->addr);
        return 0;
    } else if (arp_sdu->type == 0x00) {
        if (arp_sdu->addr == ifs->local_mip_addr) {
            memset(&sdu, 0, sizeof(struct mip_arp_sdu));
        } else {
            return -1;
        }
    }
    return 0;
}