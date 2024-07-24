#ifndef DECT_MAC_PHY_HANDLER_QUEUE_H
#define DECT_MAC_PHY_HANDLER_QUEUE_H

#include <zephyr/kernel.h>

#include "dect_mac_phy_handler_types.h"
#include "dect_mac_phy_handler.h"

int dect_phy_queue_put(enum dect_mac_phy_function function, union dect_mac_phy_handler_params *params, uint32_t priority);

void dect_phy_queue_thread();

/* single linked list that to handle the planification of the phy layer actions (declared in dect_mac_phy_handler_queue.c) */
extern sys_slist_t dect_mac_phy_handler_queue;

/* semaphore that restrain access to the dect phy layer (declared in dect_mac_phy_handler_queue.c) */
extern struct k_sem phy_layer_sem;

/* mutex to protect the linked list that is not thread-safe (declared in dect_mac_phy_handler_queue.c) */
extern struct k_mutex queue_mutex;


#endif // DECT_MAC_PHY_HANDLER_QUEUE_H