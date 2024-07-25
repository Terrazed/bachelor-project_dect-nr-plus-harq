#include "dect_mac_phy_handler_queue.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(phy_queue);

K_THREAD_DEFINE(dect_phy_queue_thread_id, DECT_MAC_PHY_HANDLER_QUEUE_THREAD_STACK_SIZE, dect_mac_phy_queue_thread, NULL, NULL, NULL, 10, 0, 0);
K_SEM_DEFINE(phy_layer_sem, 0, 1);
K_SEM_DEFINE(queue_item_sem, 0, DECT_MAC_PHY_HANDLER_QUEUE_MAX_ITEMS);
K_MUTEX_DEFINE(queue_mutex);

sys_slist_t dect_mac_phy_handler_queue;
struct dect_mac_phy_handler_queue_item current_item = {0};

int dect_phy_queue_put(enum dect_mac_phy_function function, union dect_mac_phy_handler_params *params, uint32_t priority)
{

    LOG_DBG("Adding item to the queue - function: %d, priority: %d", function, priority);

    /* locks the list while working on it */
    k_mutex_lock(&queue_mutex, K_FOREVER);
    {
        /* create the item */
        struct dect_mac_phy_handler_queue_item *item;
        item = k_malloc(sizeof(struct dect_mac_phy_handler_queue_item)); // TODO: use memory pool instead of malloc

        /* check if the item was allocated */
        if (item == NULL)
        {
            LOG_ERR("Failed to allocate memory for the queue item");
            return -1; // TODO: return a proper error code
        }

        /* init the item */
        item->function = function;
        item->params = *params;
        item->priority = priority;

        /* check if the maximum number of item in the queue is already reached */
        if (sys_slist_len(&dect_mac_phy_handler_queue) >= DECT_MAC_PHY_HANDLER_QUEUE_MAX_ITEMS) // warning! this operation is O(n) which means that if the queue max item is big it should probably be handled differently
        { 
            LOG_ERR("The queue is full, cannot add more items");
            k_mutex_unlock(&queue_mutex);
            return -1; // TODO: return a proper error code
        }

        /* if the list is empty, add the item to the head */
        if (sys_slist_is_empty(&dect_mac_phy_handler_queue))
        {
            sys_slist_prepend(&dect_mac_phy_handler_queue, &item->node);
        }
        /* if the list is not empty, add the item to the correct position */
        else
        {
            bool item_inserted = false; // flag to check if the item was inserted
            sys_snode_t *previous_node = NULL; // previous node to insert the item
            for (sys_snode_t *node = sys_slist_peek_head(&dect_mac_phy_handler_queue); // create a node to iterate through the list
                 node != NULL;                                                         // iterate until the end of the list (but will break if the item is inserted before the end of the list)
                 previous_node = node, node = sys_slist_peek_next(node))               // set the previous node to the current node and the current node to the next node
            { 
                /* get the current item */
                struct dect_mac_phy_handler_queue_item *current_item = CONTAINER_OF(node, struct dect_mac_phy_handler_queue_item, node);
                /* if the current item has a lower priority than the item to insert, insert the item before the current item */
                if (current_item->priority < item->priority)
                {
                    sys_slist_insert(&dect_mac_phy_handler_queue, previous_node, &item->node);
                    item_inserted = true;
                    break;
                }
            }
            /* if the item was not inserted, add it to the end of the list */
            if (!item_inserted)
            {
                sys_slist_append(&dect_mac_phy_handler_queue, &item->node);
            }
        }
    }
    k_mutex_unlock(&queue_mutex);

    /* notify the thread that there is an item in the queue */
    k_sem_give(&queue_item_sem);

    return 0;
}

int dect_mac_phy_queue_function_execute(enum dect_mac_phy_function function, union dect_mac_phy_handler_params *params)
{
    /* saving the current operation in case of a failed operation */
    current_item.function = function;
    current_item.params = *params;

    /* execute the operation */
    switch(function)
    {
        case CAPABILITY_GET:
            dect_mac_phy_handler_capability_get();
            break;
        case INIT:
            dect_mac_phy_handler_init();
            break;
        case DEINIT:
            dect_mac_phy_handler_deinit();
            break;
        case RX:
            dect_mac_phy_handler_rx(params->rx_params);
            break;
        case TX:
            dect_mac_phy_handler_tx(params->tx_params);
            break;
        case TX_HARQ:
            dect_mac_phy_handler_tx_harq(params->tx_harq_params);
            break;
        case TX_RX:
            dect_mac_phy_handler_tx_rx(params->tx_rx_params);
            break;
        case RSSI:
            dect_mac_phy_handler_rssi(params->rssi_params);
            break;
        case RX_STOP:
            dect_mac_phy_handler_rx_stop(params->rx_stop_params);
            break;
        case LINK_CONFIG:
            dect_mac_phy_handler_link_config();
            break;
        case TIME_GET:
            dect_mac_phy_handler_time_get();
            break;
        default:
            LOG_ERR("Unknown function: %d", item.function);
            return -1; // TODO: return a proper error code
    }

}

void dect_mac_phy_queue_thread()
{

    while (1)
    {
        /* wait for the phy layer to be free */
        k_sem_take(&phy_layer_sem, K_FOREVER);

        current_state = IDLING;
        LOG_DBG("waiting for a new item in the queue");

        /* wait for a new item in the queue */
        k_sem_take(&queue_item_sem, K_FOREVER);

        struct dect_mac_phy_handler_queue_item local_item;

        /* locks the list while working on it */
        k_mutex_lock(&queue_mutex, K_FOREVER);
        {
            /* get the first item in the list */
            sys_snode_t *node = sys_slist_get(&dect_mac_phy_handler_queue);
            struct dect_mac_phy_handler_queue_item *item = CONTAINER_OF(node, struct dect_mac_phy_handler_queue_item, node);

            /* copy the item on the stack */
            local_item = *item;

            /* put the node back in the queue if is a permanent item */
            if (item->priority == PRIORITY_PERMANENT)
            {
                sys_slist_append(&dect_mac_phy_handler_queue, node);
                k_sem_give(&queue_item_sem);
            }
            else // free the item if it is not permanent
            {
                k_free(item); // TODO: use memory pool instead of free
            }

            LOG_DBG("Got item from the queue - function: %d, priority: %d", local_item.function, local_item.priority);
        }
        k_mutex_unlock(&queue_mutex);

        /* execute the function */
        dect_mac_phy_queue_function_execute(local_item.function, &local_item.params);
    }
}