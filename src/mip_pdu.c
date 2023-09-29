//
// Created by mikita on 24/09/23.
//
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include "../include/mip_pdu.h"
#include "../include/mipd.h"
#include "../include/utils.h"


void print_pdu_content(struct mip_pdu *pdu)
{
    printf("====================================================\n");
    printf("\t Source MAC address: ");
    print_mac_addr(pdu->ethhdr->src_mac, 6);
    printf("\t Destination MAC address: ");
    print_mac_addr(pdu->ethhdr->dst_mac, 6);
    printf("\t Ethertype: 0x%04x\n", pdu->ethhdr->ethertype);

    printf("\t Source MIP address: %u\n", pdu->miphdr->src);
    printf("\t Destination MIP address: %u\n", pdu->miphdr->dst);
    printf("\t TTL: %u\n", pdu->miphdr->ttl);
    printf("\t SDU length: %d\n", pdu->miphdr->sdu_l * 4);
    printf("\t PDU type: 0x%02x\n", pdu->miphdr->sdu_t);

    printf("\t SDU: %s\n", pdu->sdu);
    printf("====================================================\n");
}
