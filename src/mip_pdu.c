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

struct mip_pdu *alloc_pdu(void) {
    struct mip_pdu *pdu = (struct mip_pdu *) malloc(sizeof(struct mip_pdu));
    pdu->ethhdr = (struct eth_hdr *) malloc(sizeof(struct mip_pdu));
    pdu->ethhdr->ethertype = htons(0xFFFF);
    pdu->miphdr = (struct mip_hdr *) malloc(sizeof(struct mip_pdu));
    pdu->miphdr->dst = MIP_DST_ADDR;
    pdu->miphdr->src = MIP_DST_ADDR;
    pdu->miphdr->sdu_l = (1u << 8) - 1;
    pdu->miphdr->sdu_t = MIP_TYPE_ARP;
    pdu->miphdr->ttl = 1;
    return pdu;
}
size_t mip_deserialize_pdu(struct mip_pdu *pdu, uint8_t *rcv_buf)
{
    /* pdu = (struct pdu *)malloc(sizeof(struct pdu)); */
    size_t rcv_len = 0;

    /* Unpack ethernet header */
    pdu->ethhdr = (struct eth_hdr *)malloc(ETH_HDR_LEN);
    memcpy(pdu->ethhdr, rcv_buf + rcv_len, ETH_HDR_LEN);
    rcv_len += ETH_HDR_LEN;

    pdu->miphdr = (struct mip_hdr *)malloc(MIP_HDR_LEN);
    uint32_t *tmp = (uint32_t *) (rcv_buf + rcv_len);
    uint32_t header = ntohl(*tmp);
    pdu->miphdr->dst = (uint8_t) (header >> 24);
    pdu->miphdr->src = (uint8_t) (header >> 16);
    pdu->miphdr->ttl = (uint8_t) (header >> 12 & 0xf);
    pdu->miphdr->sdu_l = (size_t) (((header >> 3) & 0x1ff));
    pdu->miphdr->sdu_t = (uint8_t) (header & 0x7);
    rcv_len += MIP_HDR_LEN;

    pdu->sdu = (uint8_t *)calloc(1, pdu->miphdr->sdu_l * 4);
    memcpy(pdu->sdu, rcv_buf + rcv_len, pdu->miphdr->sdu_l * 4);
    rcv_len += pdu->miphdr->sdu_l * 4;

    return rcv_len;
}
size_t mip_serialize_pdu(struct mip_pdu *pdu, uint8_t *snd_buf)
{
    size_t snd_len = 0;

    /* Copy ethernet header */
    memcpy(snd_buf + snd_len, pdu->ethhdr, sizeof(struct eth_hdr));
    snd_len += ETH_HDR_LEN;

    /* Copy MIP header */
    uint32_t mip_hdr = 0;
    mip_hdr |= (uint32_t) pdu->miphdr->dst << 24;
    mip_hdr |= (uint32_t) pdu->miphdr->src << 16;
    mip_hdr |= (uint32_t) (pdu->miphdr->ttl & 0xf) << 12;
    mip_hdr |= (uint32_t) (pdu->miphdr->sdu_l & 0x1ff) << 3;
    mip_hdr |= (uint32_t) (pdu->miphdr->sdu_t & 0x7);
    mip_hdr = htonl(mip_hdr);

    memcpy(snd_buf + snd_len, &mip_hdr, MIP_HDR_LEN);
    snd_len += MIP_HDR_LEN;

    /* Attach SDU */
    memcpy(snd_buf + snd_len, pdu->sdu, pdu->miphdr->sdu_l * 4);
    snd_len += pdu->miphdr->sdu_l * 4;

    return snd_len;
}

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
void fill_pdu(struct mip_pdu *pdu,
              uint8_t *src_mac_addr,
              uint8_t *dst_mac_addr,
              uint8_t src_hip_addr,
              uint8_t dst_hip_addr,
              uint8_t *sdu, uint8_t sdu_size)
{
    size_t slen = sdu_size;

    memcpy(pdu->ethhdr->dst_mac, dst_mac_addr, 6);
    memcpy(pdu->ethhdr->src_mac, src_mac_addr, 6);
    pdu->ethhdr->ethertype = htons(ETH_P_HIP);
    
    pdu->miphdr->dst = dst_hip_addr;
    pdu->miphdr->src = src_hip_addr;
    pdu->miphdr->ttl = 1;

    if (slen % 4 != 0)
        slen = slen + (4 - (slen % 4));

    pdu->miphdr->sdu_l = slen / 4;

    pdu->sdu = (uint8_t *)calloc(1, slen);
    memcpy(pdu->sdu, sdu, slen);
}
void destroy_pdu(struct mip_pdu *pdu)
{
    free(pdu->ethhdr);
    free(pdu->miphdr);
    free(pdu->sdu);
    free(pdu);
}
