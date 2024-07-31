#ifndef DECT_MAC_NODE
#define DECT_MAC_NODE

#include <zephyr/kernel.h>
#include <zephyr/sys/hash_map.h>

#include "dect_mac_phy_handler.h"

/* define the minimum time that a node will kept if not used 
(the node will be deleted after two cycle so it can 
take up to double the time with a average at 1.5 times the time) */
#define DECT_MAC_NODE_DELETE_TIMEOUT K_MINUTES(10)

/* struct that represent a node */
struct node{
    uint32_t address;
    bool used;
    uint8_t tx_power;
    int8_t mcs;
    uint32_t buffer_size;
};



/* hashmap that contains all the nodes, the keys are the nodes addresses */
extern struct sys_hashmap node_hashmap;

/* mutex to protect the node_hashmap */
extern struct k_mutex node_mutex;


/* thread that handle the deletion of the not used nodes */
void dect_mac_node_thread();



/* function that cleans all the nodes not used in the past #DECT_MAC_NODE_DELETE_TIMEOUT */
void dect_mac_node_clean_unused_nodes();

/* function that get the tx power of a node */
uint8_t dect_mac_node_get_tx_power(uint32_t address);

/* function that get the mcs of a node */
int8_t dect_mac_node_get_mcs(uint32_t address);

/* function that get the cqi of a node */
uint8_t dect_mac_node_get_cqi(uint32_t address);

/* function that create a node and put it in the hashmap */
int dect_mac_node_create_node(uint32_t address, uint8_t tx_power, int8_t mcs);

/* function that delete a node from the hashmap */
int dect_mac_node_delete_node(uint32_t address);

/* function that check if a node is in the hashmap */
bool dect_mac_node_contains_node(uint32_t address);

/* function that set the tx power of a node */
int dect_mac_node_set_tx_power(uint32_t address, uint8_t tx_power);

/* function that set the mcs of a node */
int dect_mac_node_set_mcs(uint32_t address, int8_t mcs);

/* function that optimize the transmission power and modulation to the lowest while still ensuring communication. */
int dect_mac_node_optimize(uint32_t address, int32_t rssi2, int32_t snr, uint8_t received_tx_power, int8_t received_mcs);

/* function that add power to a node */
int dect_mac_node_add_power(uint32_t address, int8_t power_to_add);

/* function that reduce the mcs level of a node */
int dect_mac_node_reduce_mcs(uint32_t address, int8_t mcs_to_reduce);

/* function that maxes the power of a node */
int dect_mac_node_full_power(uint32_t address);

#endif // DECT_MAC_NODE