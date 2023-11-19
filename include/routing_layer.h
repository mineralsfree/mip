#ifndef ROUT_LAYER
#define ROUT_LAYER

#define MAX_ENTRIES 256

struct routing_table_entry{
    uint8_t destination;
    uint8_t cost;
    uint8_t neighbour;
    uint8_t next_hop;
};

struct routing_table {
    struct routing_table_entry entries[MAX_ENTRIES];
    size_t size;
    int sd;
    int my_mip;
};

void add_routing_tableEntry(struct routing_table* table, uint8_t destination, uint8_t nextHop, uint8_t cost);
char* serialize_routing_table(const struct routing_table* table);
void initialize_routing_table(struct routing_table* table, int sd, int my_mip);
void print_routing_table(const struct routing_table* table);
void update_routing_table(struct routing_table *table, char *str, uint8_t source_table_mip);
void notify_change_table(struct routing_table *table, uint8_t source_table_mip);
void send_table(struct routing_table *table, uint8_t addr);
struct routing_table_entry *getRoutingTableEntry(const struct routing_table *table, uint8_t destination);
//void free_routing_table(struct routing_table* table);

#endif // ROUT_LAYER