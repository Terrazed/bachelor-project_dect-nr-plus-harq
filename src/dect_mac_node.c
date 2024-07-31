#include "dect_mac_node.h"

#include <zephyr/logging/log.h>
#include <zephyr/drivers/hwinfo.h>
LOG_MODULE_REGISTER(node, 3);

SYS_HASHMAP_DEFINE(node_hashmap); // create a hashmap for the nodes

K_MUTEX_DEFINE(node_mutex); // create a mutex for the nodes

K_THREAD_DEFINE(dect_mac_node_thread_id, 512, dect_mac_node_thread, NULL, NULL, NULL, 1, 0, 0); // create a thread for cleaning unused nodes

void dect_mac_node_thread()
{
    while (1)
    {
        k_sleep(DECT_MAC_NODE_DELETE_TIMEOUT);
        dect_mac_node_clean_unused_nodes();
    }
}

void dect_mac_node_clean_unused_nodes()
{
    // LOG_DBG("cleaning unused nodes");

    struct sys_hashmap_iterator iterator;
    struct sys_hashmap_iterator delete_iterator;
    struct node *node_ptr;

    // LOG_DBG("gettin the iterator");
    node_hashmap.api->iter(&node_hashmap, &iterator); // initialize iterator
    if (sys_hashmap_iterator_has_next(&iterator))
    {
        iterator.next(&iterator); // move to first node
    }

    // LOG_DBG("starting while loop");
    while (sys_hashmap_iterator_has_next(&iterator))
    {

        // LOG_DBG("copying iterator");
        memcpy(&delete_iterator, &iterator, sizeof(struct sys_hashmap_iterator)); // save last iterator

        // LOG_DBG("getting next node");
        iterator.next(&iterator); // move to next node (this has to be done before deleting the node to avoid iterator corruption)

        // LOG_DBG("getting node address");
        node_ptr = (struct node *)delete_iterator.value; // get node

        // LOG_DBG("checking if node is used, node: %d, value: %d", node_ptr, delete_iterator.value);
        if (!node_ptr->used)
        {
            LOG_INF("deleting node %d", node_ptr->address);
            dect_mac_node_delete_node(node_ptr->address); // delete node
        }
        else
        {
            LOG_INF("node %d is used, unsetting the flag", node_ptr->address);
            node_ptr->used = false; // reset used flag to see if node is used until next iteration
        }
    }
    if (!sys_hashmap_is_empty(&node_hashmap))
    {                                             // deleting last node if needed
        node_ptr = (struct node *)iterator.value; // get node

        if (!node_ptr->used)
        {
            LOG_INF("deleting node %d", node_ptr->address);
            dect_mac_node_delete_node(node_ptr->address); // delete node
        }
        else
        {
            LOG_INF("node %d is used, unsetting the flag", node_ptr->address);
            node_ptr->used = false; // reset used flag to see if node is used until next iteration
        }
    }
}

uint8_t dect_mac_node_get_tx_power(uint32_t address)
{
    uint64_t node_addr;
    struct node *node_ptr;
    uint8_t result;

    bool should_create_node = false;

    k_mutex_lock(&node_mutex, K_FOREVER);
    {
        if (sys_hashmap_get(&node_hashmap, address, (uint64_t *)&node_addr))
        {
            node_ptr = (struct node *)node_addr;
            node_ptr->used = true; // mark node as used
            result = node_ptr->tx_power;
            LOG_DBG("tx_power: %d, key: %d", result, address);
        }
        else
        {
            LOG_INF("tx_power: node %d not found, creating the node", address);
            should_create_node = true;
        }
    }
    k_mutex_unlock(&node_mutex);

    if (should_create_node)
    {
        int ret = dect_mac_node_create_node(address, CONFIG_TX_POWER, capabilities.mcs_max);
        if (ret)
        {
            LOG_ERR("tx_power: cannot create node");
        }
        result = CONFIG_TX_POWER;
    }

    return result;
}

int8_t dect_mac_node_get_mcs(uint32_t address)
{
    uint64_t node_addr;
    struct node *node_ptr;
    int8_t result;

    bool should_create_node = false;

    k_mutex_lock(&node_mutex, K_FOREVER);
    {
        if (sys_hashmap_get(&node_hashmap, address, (uint64_t *)&node_addr))
        {
            node_ptr = (struct node *)node_addr;
            node_ptr->used = true; // mark node as used
            result = node_ptr->mcs;
            LOG_DBG("mcs: %d, key: %d", result, address);
        }
        else
        {
            LOG_INF("mcs: node %d not found, creating the node", address);
            should_create_node = true;
        }
    }
    k_mutex_unlock(&node_mutex);

    if (should_create_node)
    {
        int ret = dect_mac_node_create_node(address, CONFIG_TX_POWER, capabilities.mcs_max);
        if(ret)
        {
            LOG_ERR("mcs: cannot create node");
        }
        
        result = capabilities.mcs_max;
    }

    return result;
}

uint8_t dect_mac_node_get_cqi(uint32_t address)
{
    uint64_t node_addr;
    struct node *node_ptr;
    uint8_t result;

    bool should_create_node = false;

    k_mutex_lock(&node_mutex, K_FOREVER);
    {
        if (sys_hashmap_get(&node_hashmap, address, (uint64_t *)&node_addr))
        {
            node_ptr = (struct node *)node_addr;
            node_ptr->used = true; // mark node as used
            result = node_ptr->mcs + 1;
            LOG_DBG("cqi: %d, key: %d", result, address);
        }
        else
        {
            LOG_INF("cqi: node %d not found, creating the node", address);
            should_create_node = true;
            result = 0;
        }
    }
    k_mutex_unlock(&node_mutex);

    if (should_create_node)
    {
        int ret = dect_mac_node_create_node(address, CONFIG_TX_POWER, capabilities.mcs_max);
        if(ret)
        {
            LOG_ERR("cqi: cannot create node");
        }
        
        result = capabilities.mcs_max + 1;
    }

    return result;
}

int dect_mac_node_create_node(uint32_t address, uint8_t tx_power, int8_t mcs)
{

    struct node *node_ptr;
    node_ptr = (struct node *)k_malloc(sizeof(struct node));
    // LOG_WRN("node_ptr: %p", (void*)node_ptr);

    node_ptr->address = address;
    node_ptr->tx_power = tx_power;
    node_ptr->mcs = mcs;
    node_ptr->used = false;

    int result;

    k_mutex_lock(&node_mutex, K_FOREVER);
    {
        int ret = sys_hashmap_insert(&node_hashmap, address, (uintptr_t)node_ptr, NULL);
        switch (ret)
        {
        case 0:
            LOG_WRN("node already exists, overwriting");
            result = 0;
            break;
        case 1:
            LOG_DBG("node created : %d", address);
            result = 0;
            break;
        case -ENOMEM:
            LOG_ERR("not enough memory to create node");
            result = -1; // TODO: true error code
            break;
        case -ENOSPC:
            LOG_ERR("too many nodes already, cannot create node");
            result = -1; // TODO: true error code
            break;
        default:
            LOG_ERR("unexpected result from sys_hashmap_insert");
            result = -1; // TODO: true error code
            break;
        }
    }
    k_mutex_unlock(&node_mutex);

    return result;
}

int dect_mac_node_delete_node(uint32_t address)
{

    struct node *node_ptr;

    int result;

    k_mutex_lock(&node_mutex, K_FOREVER);
    {
        bool ret = sys_hashmap_remove(&node_hashmap, address, (uint64_t *)&node_ptr);

        if (ret)
        {
            LOG_DBG("node %p deleted", (void *)node_ptr);
            k_free(node_ptr);
            result = 0;
        }
        else
        {
            LOG_ERR("node not found");
            result = -1; // TODO: true error code
        }
    }
    k_mutex_unlock(&node_mutex);

    return result;
}

int dect_mac_node_set_tx_power(uint32_t address, uint8_t tx_power)
{
    struct node *node_ptr;
    int result;

    k_mutex_lock(&node_mutex, K_FOREVER);
    {
        if (sys_hashmap_get(&node_hashmap, address, (uint64_t *)&node_ptr))
        {
            node_ptr->used = true; // mark node as used
            node_ptr->tx_power = tx_power;
            result = 0;
        }
        else
        {
            LOG_ERR("node not found");
            result = -1; // TODO: true error code
        }
    }
    k_mutex_unlock(&node_mutex);

    return result;
}

int dect_mac_node_set_mcs(uint32_t address, int8_t mcs)
{
    struct node *node_ptr;
    int result;

    k_mutex_lock(&node_mutex, K_FOREVER);
    {
        if (sys_hashmap_get(&node_hashmap, address, (uint64_t *)&node_ptr))
        {
            node_ptr->used = true; // mark node as used
            node_ptr->mcs = mcs;
            result = 0;
        }
        else
        {
            LOG_ERR("node not found");
            result = -1; // TODO: true error code
        }
    }
    k_mutex_unlock(&node_mutex);

    return result;
}

bool dect_mac_node_contains_node(uint32_t address)
{

    return sys_hashmap_contains_key(&node_hashmap, address);
}

int dect_mac_node_optimize(uint32_t address, int32_t rssi2, int32_t snr, uint8_t received_tx_power, int8_t received_mcs)
{

    int result = 0;

    k_mutex_lock(&node_mutex, K_FOREVER);
    {
        LOG_DBG("checking if node exists");
        if (!dect_mac_node_contains_node(address))
        {
            int ret = dect_mac_node_create_node(address, CONFIG_TX_POWER, capabilities.mcs_max);
            if (ret)
            {
                LOG_ERR("cannot create node");
                result = -1; // TODO: true error code
            }
        }

        LOG_INF("received_tx_power: %d, received_mcs: %d, rssi: %d, snr: %d", received_tx_power, received_mcs, rssi2, snr);

        /* -----------=| tx_power |=----------- */

        LOG_DBG("received_tx_power: %d, rssi: %d", received_tx_power, rssi2);

        int received_tx_power_dbm = 20;
        switch (received_tx_power)
        {
        case 0:
            received_tx_power_dbm = -40;
            break;
        case 1:
            received_tx_power_dbm = -30;
            break;
        case 2:
            received_tx_power_dbm = -20;
            break;
        case 3:
            received_tx_power_dbm = -13;
            break;
        case 4:
            received_tx_power_dbm = -6;
            break;
        case 5:
            received_tx_power_dbm = -3;
            break;
        case 6:
            received_tx_power_dbm = 0;
            break;
        case 7:
            received_tx_power_dbm = 3;
            break;
        case 8:
            received_tx_power_dbm = 6;
            break;
        case 9:
            received_tx_power_dbm = 10;
            break;
        case 10:
            received_tx_power_dbm = 14;
            break;
        case 11:
            received_tx_power_dbm = 19;
            break;
        default:
            LOG_WRN("received_tx_power not in range");
            result = -1; // TODO: true error code
            break;
        }

        /* compute wanted power output */
        int8_t power_margin = CONFIG_TX_POWER_MARGIN_DBM;                                              // offset to ensure good communication
        int wanted_tx_power_dbm = received_tx_power_dbm - (rssi2 - CONFIG_RSSI_TARGET - power_margin); // set tx power to rssi2
        LOG_DBG("wanted_tx_power_dbm: %d", wanted_tx_power_dbm);

        /* convert from dBm to transmit power (table 6.2.1-3 etsi dect-2020 mac layer) */
        int wanted_tx_power;
        if (wanted_tx_power_dbm <= -40)
        {
            // wanted_tx_power = 0;
            wanted_tx_power = 2; // after measuring, it seems that the power requiered to send the power "2" is lower than the power "0"
        }
        else if (wanted_tx_power_dbm <= -30)
        {
            // wanted_tx_power = 1;
            wanted_tx_power = 2; // after measuring, it seems that the power requiered to send the power "2" is lower than the power "1"
        }
        else if (wanted_tx_power_dbm <= -20)
        {
            wanted_tx_power = 2;
        }
        else if (wanted_tx_power_dbm <= -13)
        {
            wanted_tx_power = 3;
        }
        else if (wanted_tx_power_dbm <= -6)
        {
            wanted_tx_power = 4;
        }
        else if (wanted_tx_power_dbm <= -3)
        {
            wanted_tx_power = 5;
        }
        else if (wanted_tx_power_dbm <= 0)
        {
            wanted_tx_power = 6;
        }
        else if (wanted_tx_power_dbm <= 3)
        {
            wanted_tx_power = 7;
        }
        else if (wanted_tx_power_dbm <= 6)
        {
            wanted_tx_power = 8;
        }
        else if (wanted_tx_power_dbm <= 10)
        {
            wanted_tx_power = 9;
        }
        else if (wanted_tx_power_dbm <= 14)
        {
            wanted_tx_power = 10;
        }
        else
        {
            wanted_tx_power = 11;
        }

        LOG_DBG("wanted_tx_power: %d", wanted_tx_power);

        /* put wanted power into the node */
        dect_mac_node_set_tx_power(address, wanted_tx_power);
        LOG_DBG("tx_power set");

        /* -----------=| mcs |=----------- */

        LOG_DBG("received_mcs: %d, snr: %d", received_mcs, snr);

        /* compute wanted mcs */
        int8_t wanted_mcs;
        if (snr >= 14)
        {
            wanted_mcs = 4; // modulation 16QAM, Coding rate 3/4 -> 14dB min for <10% PER
        }
        else if (snr >= 10)
        {
            wanted_mcs = 3; // modulation 16QAM, Coding rate 1/2 -> 10dB min for <10% PER
        }
        else if (snr >= 8)
        {
            wanted_mcs = 2; // modulation QPSK, Coding rate 3/4 -> 8dB min for <10% PER
        }
        else if (snr >= 6)
        {
            wanted_mcs = 1; // modulation QPSK, Coding rate 1/2 -> 6dB min for <10% PER
        }
        else if (snr >= 4)
        {
            wanted_mcs = 0; // modulation BPSK, Coding rate 1/2 -> 4dB min for <10% PER
        }
        else
        {
            wanted_mcs = -1; // communication with <10% PER not possible
            LOG_WRN("channel is too noisy, cannot garantee communication with <10% PER");
            result = -1; // TODO: true error code
        }

        LOG_DBG("wanted_mcs: %d", wanted_mcs);

        /* put wanted mcs into the node */
        dect_mac_node_set_mcs(address, wanted_mcs);
        LOG_DBG("mcs set");

        LOG_DBG("wanted_tx_power_dbm: %d, wanted_tx_power: %d, wanted_mcs: %d", wanted_tx_power_dbm, wanted_tx_power, wanted_mcs);
    }
    k_mutex_unlock(&node_mutex);

    return result;
}

int dect_mac_node_add_power(uint32_t address, int8_t power_to_add)
{
    uint64_t node_addr;
    struct node *node_ptr;
    int result;

    k_mutex_lock(&node_mutex, K_FOREVER);
    {
        if (sys_hashmap_get(&node_hashmap, address, (uint64_t *)&node_addr))
        {
            node_ptr = (struct node *)node_addr;
            node_ptr->used = true; // mark node as used
            node_ptr->tx_power += power_to_add;

            if (node_ptr->tx_power < 0)
            {
                LOG_DBG("cannot make power below 0, setting to 0");
                node_ptr->tx_power = 0;
            }
            else if (node_ptr->tx_power > CONFIG_TX_POWER)
            {
                LOG_DBG("cannot make power above %d, setting to %d", CONFIG_TX_POWER, CONFIG_TX_POWER);
                node_ptr->tx_power = CONFIG_TX_POWER;
            }
            else
            {
                LOG_INF("new power: %d", node_ptr->tx_power);
            }

            result = 0;
        }
        else
        {
            LOG_ERR("node not found");
            result = -1; // TODO: true error code
        }
    }
    k_mutex_unlock(&node_mutex);

    return result;
}

int dect_mac_node_reduce_mcs(uint32_t address, int8_t mcs_to_reduce)
{
    uint64_t node_addr;
    struct node *node_ptr;
    int result;

    k_mutex_lock(&node_mutex, K_FOREVER);
    {
        if (sys_hashmap_get(&node_hashmap, address, (uint64_t *)&node_addr))
        {
            node_ptr = (struct node *)node_addr;
            node_ptr->used = true; // mark node as used
            node_ptr->mcs -= mcs_to_reduce;

            if (node_ptr->mcs < 0)
            {
                LOG_DBG("cannot reduce mcs below 0, setting to 0");
                node_ptr->mcs = 0;
            }
            else if (node_ptr->mcs > capabilities.mcs_max)
            {
                LOG_DBG("cannot reduce mcs above %d, setting to %d", capabilities.mcs_max, capabilities.mcs_max);
                node_ptr->mcs = capabilities.mcs_max;
            }
            else
            {
                LOG_INF("new mcs: %d", node_ptr->mcs);
            }

            result = 0;
        }
        else
        {
            LOG_ERR("node not found");
            result = -1; // TODO: true error code
        }
    }
    k_mutex_unlock(&node_mutex);

    return result;
}

int dect_mac_node_full_power(uint32_t address)
{
    uint64_t node_addr;
    struct node *node_ptr;
    int result;

    k_mutex_lock(&node_mutex, K_FOREVER);
    {
        if (sys_hashmap_get(&node_hashmap, address, (uint64_t *)&node_addr))
        {
            node_ptr = (struct node *)node_addr;
            node_ptr->used = true; // mark node as used

            node_ptr->tx_power = CONFIG_TX_POWER;

            result = 0;
        }
        else
        {
            LOG_ERR("node not found");
            result = -1; // TODO: true error code
        }
    }
    k_mutex_unlock(&node_mutex);

    return result;
}