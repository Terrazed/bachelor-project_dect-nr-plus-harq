#ifndef DECT_MAC_PHY_HANDLER_QUEUE_H
#define DECT_MAC_PHY_HANDLER_QUEUE_H

#include <zephyr/kernel.h>

#include "dect_mac_phy_handler_types.h"
#include "dect_mac_phy_handler.h"

/* stack size in bytes of the thread that reads the list */
#define DECT_MAC_PHY_HANDLER_QUEUE_THREAD_STACK_SIZE 1024

/* maximum number of item that the queue can contains */
#define DECT_MAC_PHY_HANDLER_QUEUE_MAX_ITEMS 32

enum dect_mac_phy_handler_queue_priority{
    PRIORITY_PERMANENT = 0,
    PRIORITY_LOW = 10000,
    PRIORITY_MEDIUM = 20000,
    PRIORITY_HIGH = 30000,
    PRIORITY_CRITICAL = UINT32_MAX,
};

/* struct that represents an item in the queue */
struct dect_mac_phy_handler_queue_item {
    sys_snode_t node;
    enum dect_mac_phy_function function;
    union dect_mac_phy_handler_params params;
    enum dect_mac_phy_handler_queue_priority priority;
};

/* function to put an operation in the waiting queue of the dect phy api */
int dect_phy_queue_put(enum dect_mac_phy_function function, union dect_mac_phy_handler_params *params, uint32_t priority);

/* function to execute an operation from the waiting queue of the dect phy api */
int dect_mac_phy_queue_function_execute(enum dect_mac_phy_function function, union dect_mac_phy_handler_params *params);

/* thread where the list is read whenever something is in it */
void dect_mac_phy_queue_thread();

/* single linked list that to handle the planification of the phy layer actions (declared in dect_mac_phy_handler_queue.c) */
extern sys_slist_t dect_mac_phy_handler_queue;

/* semaphore that restrain access to the dect phy layer (declared in dect_mac_phy_handler_queue.c) */
extern struct k_sem phy_layer_sem;

/* semaphore that notify the thread that an element is waiting in the queue */
extern struct k_sem queue_item_sem;

/* mutex to protect the linked list that is not thread-safe (declared in dect_mac_phy_handler_queue.c) */
extern struct k_mutex queue_mutex;


#endif // DECT_MAC_PHY_HANDLER_QUEUE_H