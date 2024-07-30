
#include "dect_mac_node.h"

struct sys_hashmap node_hashmap;
struct k_mutex node_mutex;


void dect_mac_node_thread()
{
    
}

void dect_mac_node_clean_unused_nodes()
{

}

int dect_mac_node_get_tx_power(uint32_t address)
{
    return -1;
}

int dect_mac_node_get_mcs(uint32_t address)
{
    return -1;
}

int dect_mac_node_create_node(uint32_t address, uint32_t tx_power, uint32_t mcs)
{
    return -1;
}

int dect_mac_node_delete_node(uint32_t address)
{
    return -1;
}

int dect_mac_node_contains_node(uint32_t address)
{
    return -1;
}

int dect_mac_node_set_tx_power(uint32_t address, uint32_t tx_power)
{
    return -1;
}

int dect_mac_node_set_mcs(uint32_t address, uint32_t mcs)
{
    return -1;
}

int dect_mac_node_optimize(uint32_t address, int32_t rssi2, int32_t snr, uint32_t received_tx_power, uint32_t received_mcs)
{
    return -1;
}

int dect_mac_node_add_power(uint32_t address, int32_t power_to_add)
{
    return -1;
}

int dect_mac_node_reduce_mcs(uint32_t address, int32_t mcs_to_reduce)
{
    return -1;
}


int dect_mac_node_full_power(uint32_t address)
{
    return -1;
}
