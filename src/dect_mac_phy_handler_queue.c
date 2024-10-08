#include "dect_mac_phy_handler_queue.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(phy_queue, 3);

K_THREAD_DEFINE(dect_phy_queue_handler_exec_thread_id, DECT_MAC_PHY_HANDLER_QUEUE_THREAD_STACK_SIZE, dect_mac_phy_handler_queue_exec_thread, NULL, NULL, NULL, 10, 0, 0);
K_SEM_DEFINE(phy_layer_sem, 1, 1);
K_SEM_DEFINE(queue_item_sem, 0, DECT_MAC_PHY_HANDLER_QUEUE_MAX_ITEMS);
K_MUTEX_DEFINE(queue_mutex);
K_TIMER_DEFINE(dect_mac_phy_handler_queue_operation_failed_timer, dect_mac_phy_handler_queue_timer_callback, NULL);
K_THREAD_DEFINE(dect_phy_queue_handler_put_thread_id, DECT_MAC_PHY_HANDLER_QUEUE_THREAD_STACK_SIZE, dect_mac_phy_handler_queue_put_thread, NULL, NULL, NULL, 9, 0, 0);
K_MSGQ_DEFINE(dect_mac_phy_handler_queue_msgq, sizeof(struct dect_mac_phy_handler_queue_item *), DECT_MAC_PHY_HANDLER_QUEUE_MAX_ITEMS, 1);

K_MEM_SLAB_DEFINE(dect_mac_phy_handler_queue_item_slab, sizeof(struct dect_mac_phy_handler_queue_item), DECT_MAC_PHY_HANDLER_QUEUE_MAX_ITEMS, 1);
K_MEM_SLAB_DEFINE(dect_mac_phy_handler_queue_node_slab, sizeof(struct dect_mac_phy_handler_queue_node), DECT_MAC_PHY_HANDLER_QUEUE_MAX_ITEMS, 1);

sys_slist_t dect_mac_phy_handler_queue = SYS_SLIST_STATIC_INIT(&dect_mac_phy_handler_queue);
struct dect_mac_phy_handler_queue_node current_node = {0};
uint32_t dect_mac_phy_handler_queue_operation_failed_counter = 0;

int dect_phy_queue_put(enum dect_mac_phy_function function, union dect_mac_phy_handler_params *params, uint32_t priority)
{

    /* create the item */
    struct dect_mac_phy_handler_queue_item *dect_mac_phy_handler_queue_item = NULL;
    int ret = k_mem_slab_alloc(&dect_mac_phy_handler_queue_item_slab, &dect_mac_phy_handler_queue_item, K_NO_WAIT);
    if (ret)
    {
        LOG_ERR("Failed to allocate memory for the item");
        return NO_MEM_SLAB;
    }

    /* create the item context */
    dect_mac_phy_handler_queue_item->function = function;
    if (params != NULL)
    {
        dect_mac_phy_handler_queue_item->params = *params;
    }
    else
    {
        dect_mac_phy_handler_queue_item->params = (union dect_mac_phy_handler_params){0};
    }
    dect_mac_phy_handler_queue_item->priority = priority;

    LOG_DBG("item: %p, function: %d, params: %x, priority: %u", dect_mac_phy_handler_queue_item, function, params, priority);

    /* submit the request */
    ret = k_msgq_put(&dect_mac_phy_handler_queue_msgq, &dect_mac_phy_handler_queue_item, K_FOREVER);
    if (ret)
    {
        LOG_ERR("Failed to put the item in the queue");
        return ret;
    }

    return OK;
}

void dect_mac_phy_handler_queue_put_thread()
{
    while (1)
    {
        /* get the item */
        struct dect_mac_phy_handler_queue_item *dect_mac_phy_handler_queue_item;
        int ret = k_msgq_get(&dect_mac_phy_handler_queue_msgq, &dect_mac_phy_handler_queue_item, K_FOREVER);
        if (ret)
        {
            LOG_ERR("Failed to get the item from the queue");
        }

        /* get the context */
        enum dect_mac_phy_function function = dect_mac_phy_handler_queue_item->function;
        union dect_mac_phy_handler_params params = dect_mac_phy_handler_queue_item->params;
        uint32_t priority = dect_mac_phy_handler_queue_item->priority;

        /* free the item */
        k_mem_slab_free(&dect_mac_phy_handler_queue_item_slab, dect_mac_phy_handler_queue_item);

        /* add the item to the queue */
        LOG_DBG("Adding item to the queue - function: %d, priority: %u", function, priority);

        /* locks the list while working on it */
        k_mutex_lock(&queue_mutex, K_FOREVER);
        {
            /* create the node */
            struct dect_mac_phy_handler_queue_node *node = NULL;
            LOG_DBG("Allocating memory for the queue item");

            /* allocate the memory for the node */
            int ret = k_mem_slab_alloc(&dect_mac_phy_handler_queue_node_slab, &node, K_NO_WAIT);
            if (ret)
            {
                LOG_ERR("Failed to allocate memory for the queue item");
                return;
            }

            /* init the node */
            LOG_DBG("Initializing the queue item");
            node->item.function = function;
            node->item.params = params;
            node->item.priority = priority;

            /* check if the maximum number of node in the queue is already reached */
            if (sys_slist_len(&dect_mac_phy_handler_queue) >= DECT_MAC_PHY_HANDLER_QUEUE_MAX_ITEMS) // warning! this operation is O(n) which means that if the queue max item is big it should probably be handled differently
            {
                LOG_ERR("The queue is full, cannot add more items");
                k_mutex_unlock(&queue_mutex);
                return;
            }

            /* if the list is empty, add the item to the head */
            if (sys_slist_is_empty(&dect_mac_phy_handler_queue))
            {
                sys_slist_prepend(&dect_mac_phy_handler_queue, &node->node);
            }
            /* if the list is not empty, add the item to the correct position */
            else
            {
                bool node_inserted = false;                                                    // flag to check if the item was inserted
                sys_snode_t *previous_node = NULL;                                             // previous node to insert the item
                for (sys_snode_t *sys_node = sys_slist_peek_head(&dect_mac_phy_handler_queue); // create a node to iterate through the list
                     sys_node != NULL;                                                         // iterate until the end of the list (but will break if the item is inserted before the end of the list)
                     previous_node = sys_node, sys_node = sys_slist_peek_next(sys_node))       // set the previous node to the current node and the current node to the next node
                {
                    /* get the current node */
                    struct dect_mac_phy_handler_queue_node *current_node = CONTAINER_OF(sys_node, struct dect_mac_phy_handler_queue_node, node);
                    /* if the current item has a lower priority than the item to insert, insert the item before the current item */
                    if (current_node->item.priority < node->item.priority)
                    {
                        sys_slist_insert(&dect_mac_phy_handler_queue, previous_node, &node->node);
                        node_inserted = true;
                        break;
                    }
                }
                /* if the node was not inserted, add it to the end of the list */
                if (!node_inserted)
                {
                    sys_slist_append(&dect_mac_phy_handler_queue, &node->node);
                }
            }
        }
        k_mutex_unlock(&queue_mutex);

        /* notify the thread that there is an item in the queue */
        k_sem_give(&queue_item_sem);
    }
}

int dect_mac_phy_handler_queue_function_execute(enum dect_mac_phy_function function, union dect_mac_phy_handler_params *params)
{
    /* saving the current operation in case of a failed operation */
    current_node.item.function = function;
    current_node.item.params = *params;

    /* execute the operation */
    switch (function)
    {
    case CAPABILITY_GET:
        LOG_DBG("capability get");
        dect_mac_phy_handler_capability_get();
        break;
    case INIT:
        LOG_DBG("init");
        dect_mac_phy_handler_init();
        break;
    case DEINIT:
        LOG_DBG("deinit");
        dect_mac_phy_handler_deinit();
        break;
    case RX:
        LOG_DBG("rx");
        params->rx_params.handle |= (1 << 27); // set in the handle that the operation comes from the queue
        dect_mac_phy_handler_rx(params->rx_params);
        break;
    case TX:
        LOG_DBG("tx");
        params->tx_params.handle |= (1 << 27); // set in the handle that the operation comes from the queue
        dect_mac_phy_handler_tx(params->tx_params);
        break;
    case TX_HARQ:
        LOG_DBG("tx harq");
        params->tx_harq_params.handle |= (1 << 27); // set in the handle that the operation comes from the queue
        dect_mac_phy_handler_tx_harq(params->tx_harq_params);
        break;
    case TX_RX:
        LOG_DBG("tx rx");
        params->tx_rx_params.handle |= (1 << 27); // set in the handle that the operation comes from the queue
        dect_mac_phy_handler_tx_rx(params->tx_rx_params);
        break;
    case RSSI:
        LOG_DBG("rssi");
        dect_mac_phy_handler_rssi(params->rssi_params);
        break;
    case RX_STOP:
        LOG_DBG("rx stop");
        dect_mac_phy_handler_rx_stop(params->rx_stop_params);
        break;
    case LINK_CONFIG:
        LOG_DBG("link config");
        dect_mac_phy_handler_link_config();
        break;
    case TIME_GET:
        LOG_DBG("time get");
        dect_mac_phy_handler_time_get();
        break;
    case PLACEHOLDER:
        LOG_DBG("placeholder");
        current_state = UNKNOWN;
        break;
    default:
        LOG_ERR("Unknown function: %d", function);
        return FUNC_NOT_EXIST;
    }
    return OK;
}

void dect_mac_phy_handler_queue_exec_thread()
{
    while (1)
    {
        /* wait for the phy layer to be free */
        k_sem_take(&phy_layer_sem, K_FOREVER);

        current_state = IDLING;
        LOG_DBG("waiting for a new item in the queue");

        /* wait for a new item in the queue */
        k_sem_take(&queue_item_sem, K_FOREVER);
        LOG_DBG("Got a new item in the queue");

        struct dect_mac_phy_handler_queue_node local_node;

        /* locks the list while working on it */
        k_mutex_lock(&queue_mutex, K_FOREVER);
        {
            /* get the first node in the list */
            sys_snode_t *sys_node = sys_slist_get(&dect_mac_phy_handler_queue);
            struct dect_mac_phy_handler_queue_node *node = CONTAINER_OF(sys_node, struct dect_mac_phy_handler_queue_node, node);

            /* copy the node on the stack */
            local_node = *node;

            /* put the node back in the queue if is a permanent item */
            if (node->item.priority == PRIORITY_PERMANENT)
            {
                sys_slist_append(&dect_mac_phy_handler_queue, sys_node);
                k_sem_give(&queue_item_sem);
            }
            else // free the item if it is not permanent
            {
                k_mem_slab_free(&dect_mac_phy_handler_queue_node_slab, node);
            }

            LOG_DBG("Got item from the queue - function: %d, priority: %d", local_node.item.function, local_node.item.priority);
        }
        k_mutex_unlock(&queue_mutex);

        /* reset fail counter and execute the function */
        LOG_DBG("Executing the function");
        dect_mac_phy_handler_queue_operation_failed_counter = 0;

        dect_mac_phy_handler_queue_function_execute(local_node.item.function, &local_node.item.params);
    }
}

void dect_mac_phy_handler_queue_operation_failed_retry(struct k_timer *timer)
{
    LOG_DBG("Retrying operation");

    /* increment the counter */
    dect_mac_phy_handler_queue_operation_failed_counter++;

    /* check if over the operation max retry limit */
    if (dect_mac_phy_handler_queue_operation_failed_counter > DECT_MAC_PHY_HANDLER_QUEUE_MAX_RETRY)
    {
        LOG_WRN("Operation failed too many times, aborting");
        k_sem_give(&phy_layer_sem);
    }
    else
    {
        k_timer_start(&dect_mac_phy_handler_queue_operation_failed_timer, DECT_MAC_PHY_HANDLER_QUEUE_RETRY_DELAY, K_NO_WAIT);
    }
}

void dect_mac_phy_handler_queue_timer_callback(struct k_timer *timer_id)
{
    dect_mac_phy_handler_queue_function_execute(current_node.item.function, &current_node.item.params);
}