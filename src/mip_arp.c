#include <malloc.h>
#include <string.h>
#include "../include/mip_arp.h"

/**
 * Prints the content of the ARP (Address Resolution Protocol) table.
 *
 * This function displays the ARP table entries, including MIP addresses, MAC addresses,
 * and associated interface indices.
 *
 * table: Pointer to the ARP table to be printed.
 */
void print_arp_content(struct arp_table *table) {
    printf("+---------------------------------------------+\n");
    printf("|         Arp table content:                  |\n");
    printf("+---------------------------------------------+\n");
    printf("| MIP Address   | MAC Address      | Interface |\n");
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

/**
 * Adds an entry to the ARP (Address Resolution Protocol) table.
 *
 * This function creates or updates an ARP table entry with the specified hardware (MAC) address,
 * MIP address, and associated interface index. If an entry for the same MIP address already exists,
 * it will be updated with the new information.
 *
 * table: Pointer to the ARP table where the entry will be added or updated.
 * hw_addr: Pointer to the hardware (MAC) address to associate with the MIP address.
 * mip_addr: The MIP address to add or update in the ARP table.
 * interfaceIndex: The interface index associated with the MIP address.
 */
void add_arp_entry(struct arp_table *table, uint8_t *hw_addr, uint8_t mip_addr, int interfaceIndex) {
    printf("Adding entry for mip: %d, interface index %d\n", mip_addr, interfaceIndex);
    struct arp_entry *entry = &table->entries[mip_addr];
    memcpy(entry->hw_addr, hw_addr, ETH_MAC_LEN);
    entry->mip_addr = mip_addr;
    entry->interfaceIndex = interfaceIndex;
}
/**
 * Creates and fills an ARP (Address Resolution Protocol) Service Data Unit (SDU).
 *
 * This function allocates memory for an ARP SDU, initializes its fields, and sets
 * the MIP address in the SDU. It is used to construct ARP packets.
 *
 * mip_addr: The MIP address to be set in the ARP SDU.
 *
 * Returns a pointer to the newly created ARP SDU.
 * The caller is responsible for freeing the memory allocated for the SDU.
 */
struct mip_arp_sdu *fill_arp_sdu(uint8_t mip_addr){
    struct mip_arp_sdu* sdu = (struct mip_arp_sdu *) malloc(sizeof (struct mip_arp_sdu));
    memset(sdu, 0, sizeof(struct mip_arp_sdu));
    sdu->type = 0x00;
    sdu->addr = mip_addr;
    return sdu;
}
