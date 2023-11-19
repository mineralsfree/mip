#include <stdio.h>        /* standard input/output library functions */
#include <stdlib.h>        /* standard library definitions (macros) */
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>    /* sockets operations */
#include <sys/un.h>        /* definitions for UNIX domain sockets */
#include <unistd.h>        /* standard symbolic constants and types */

#include "../include/routing_layer.h"

void initialize_routing_table(struct routing_table *table, int sd, int my_mip) {
    table->size = 0;
    table->sd = sd;
    table->my_mip = my_mip;
}

struct routing_table_entry *getRoutingTableEntry(const struct routing_table *table, uint8_t destination) {
    for (size_t i = 0; i < table->size; ++i) {
        if (table->entries[i].destination == destination) {
            return &(table->entries[i]);
        }
    }
    return NULL;
}

void print_routing_table(const struct routing_table *table) {
    printf("+---------------------------------------------+\n");
    printf("|         Routing table content:              |\n");
    printf("+---------------------------------------------+\n");
    printf("| Destination  | Next Hop         | Cost      |\n");
    printf("|--------------|------------------|-----------|\n");

    // Your data printing loop
    for (size_t i = 0; i < table->size; ++i) {
        struct routing_table_entry *entry = &table->entries[i];
        printf("| %-12u | %-16u | %-9u |\n", entry->destination, entry->next_hop, entry->cost);
    }

    printf("+---------------------------------------------+\n");
}

void send_table(struct routing_table *table, uint8_t addr) {
    char buf[245];
    char *resp;
    resp = serialize_routing_table(table);
    memset(buf, 0, sizeof(buf));
    char upd[] = "UPD";
    strcat(upd, resp);
    strcpy(buf + 1, upd);
    buf[0] = (char) addr;
    write(table->sd, buf, strlen(buf));

}

void notify_change_table(struct routing_table *table, uint8_t source_table_mip) {
    for (size_t i = 0; i < table->size; ++i) {
        if (table->entries[i].cost == 1 && table->entries[i].destination != source_table_mip) {
            send_table(table, table->entries[i].destination);
        }
    }
}


// Function to add an entry to the routing table
void add_routing_tableEntry(struct routing_table *table, uint8_t destination, uint8_t nextHop, uint8_t cost) {
    if (table->size < MAX_ENTRIES) {
        table->entries[table->size].destination = destination;
        table->entries[table->size].cost = cost;
        table->entries[table->size].next_hop = nextHop;
        table->size++;
        print_routing_table(table);
//        notify_change_table(table, destination);
    } else {
        printf("Routing table is full. Cannot add more entries.\n");
    }
}


void handle_incoming_routing_entry(struct routing_table *table, struct routing_table_entry *entry,
                                   uint8_t source_table_mip) {
    struct routing_table_entry *existant_entry;

    if (entry->destination == table->my_mip) {
        return;
    } else {
        existant_entry = getRoutingTableEntry(table, entry->destination);
        if (existant_entry && existant_entry->cost > entry->cost + 1) {
            existant_entry->cost = entry->cost + 1;
            existant_entry->next_hop = source_table_mip;
        } else {
            printf("adding routing entry, %d,%d,%d\n", entry->destination, source_table_mip, entry->cost + 1);
            add_routing_tableEntry(table, entry->destination, source_table_mip, entry->cost + 1);
            notify_change_table(table, source_table_mip);
        }
    }
}

void update_routing_table(struct routing_table *table, char *str, uint8_t source_table_mip) {
    size_t str_len = strlen(str);
    char numberStr[4]; // Buffer to store each number as a string
//    printf("StRING LENGTH: %lud", str_len);
    fflush(stdout);
    // Ensure that the string length is a multiple of the expected entry size
    if (str_len % 9 != 0) {
        fprintf(stderr, "Invalid serialized string length.\n");
        return;
    }

    for (size_t i = 0; i < str_len / 3; ++i) {
        strncpy(numberStr, str + i * 3, 3);
        // Null-terminate the buffer
        numberStr[3] = '\0';
        // Convert the string to integers and store in separate arrays
        uint8_t destination = atoi(numberStr);
        printf("DESTINATION: %s, %d", numberStr, destination);
        // Repeat the process for the next 2 numbers
        strncpy(numberStr, str + (i * 3) + 3, 3);
        numberStr[3] = '\0';
        uint8_t nextHop = atoi(numberStr);

        strncpy(numberStr, str + (i * 3) + 6, 3);
        numberStr[3] = '\0';
        uint8_t cost = atoi(numberStr);
        i += 2;
        struct routing_table_entry entry;
        entry.destination = destination;
        entry.next_hop = nextHop;
        entry.cost = cost;
        handle_incoming_routing_entry(table, &entry, source_table_mip);
    }
}

// Function to print the routing table
//void print_routing_table(const struct routing_table *table) {
//    printf("Routing Table:\n");
//    printf("%-15s %-15s\n", "Destination", "Next Hop");
//
//    for (size_t i = 0; i < table->size; ++i) {
//        printf("%-15u %-15s\n", table->entries[i].destination, table->entries[i].next_hop);
//    }
//}

char *serialize_routing_table(const struct routing_table *table) {
    size_t bufferSize = (3 + 3 + 3) * table->size;
    char *serializedString = (char *) malloc(bufferSize + 1);
    if (serializedString == NULL) {
        fprintf(stderr, "Memory allocation error.\n");
        exit(EXIT_FAILURE);
    }

    size_t offset = 0;
    for (size_t i = 0; i < table->size; ++i) {

        int written = snprintf(serializedString + offset, 10, "%03u%03u%03u", table->entries[i].destination,
                               table->entries[i].next_hop, table->entries[i].cost);
        if (written < 0 || written >= 10) {
            fprintf(stderr, "Error in snprintf.\n");
            exit(EXIT_FAILURE);
        }
        offset += written;
    }

    return serializedString;
}



// Function to free memory allocated for the routing table
//void free_routing_table(struct routing_table *table) {
//    for (size_t i = 0; i < table->size; ++i) {
//        free(table->entries[i].nextHop);
//    }
//    table->size = 0;
//}