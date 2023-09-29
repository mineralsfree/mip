//
// Created by mikita on 25/09/23.
//
#include <malloc.h>
#include <string.h>
#include "../include/mip_arp.h"


void print_arp_content(struct arp_table *table) {
    printf("+---------------------------------------------+\n");
    printf("|         Arp table content:                  |\n");
    printf("+---------------------------------------------+\n");
    printf("| IP Address   | MAC Address      | Interface |\n");
    printf("|--------------|------------------|-----------|\n");

    // Your data printing loop
    for (int i = 0; i < MIP_MAX_ENTRIES; ++i) {
        struct arp_entry *entry = &table->entries[i];
        if (entry->mip_addr) {
            printf("| %-13d| ", entry->mip_addr);
            print_mac_addr(entry->hw_addr, 6);
            printf("| %-9d |\n", entry->interfaceIndex);
        }
    }

    printf("+---------------------------------------------+\n");

}
void add_arp_entry(struct arp_table *table, uint8_t *hw_addr, uint8_t mip_addr, int interfaceIndex) {
    printf("Adding entry for mip: %d, interface index %d", mip_addr, interfaceIndex);
    struct arp_entry *entry = &table->entries[mip_addr];
    memcpy(entry->hw_addr, hw_addr, ETH_MAC_LEN);
    entry->mip_addr = mip_addr;
    entry->interfaceIndex = interfaceIndex;
    print_arp_content(table);
}

struct mip_arp_sdu *fill_arp_sdu(uint8_t mip_addr){
    struct mip_arp_sdu* sdu = (struct mip_arp_sdu *) malloc(sizeof (struct mip_arp_sdu));
    memset(sdu, 0, sizeof(struct mip_arp_sdu));
    sdu->type = 0x00;
    sdu->addr = mip_addr;
    return sdu;
}

