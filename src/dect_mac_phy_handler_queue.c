#include "dect_mac_phy_handler_queue.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(phy_queue,3);

K_THREAD_DEFINE(dect_phy_queue_thread_id, DECT_MAC_PHY_HANDLER_QUEUE_THREAD_STACK_SIZE, dect_mac_phy_queue_thread, NULL, NULL, NULL, 10, 0, 0);
K_SEM_DEFINE(phy_layer_sem, 1, 1);
K_SEM_DEFINE(queue_item_sem, 0, DECT_MAC_PHY_HANDLER_QUEUE_MAX_ITEMS);
K_MUTEX_DEFINE(queue_mutex);
K_TIMER_DEFINE(dect_mac_phy_handler_queue_operation_failed_timer, dect_mac_phy_handler_queue_timer_callback, NULL);
K_THREAD_STACK_DEFINE(dect_mac_phy_handler_queue_work_queue_stack, DECT_MAC_PHY_HANDLER_QUEUE_THREAD_STACK_SIZE);

sys_slist_t dect_mac_phy_handler_queue;
struct dect_mac_phy_handler_queue_item current_item = {0};
uint32_t dect_mac_phy_handler_queue_operation_failed_counter = 0;
struct k_work_q dect_mac_phy_handler_queue_work_queue;

int dect_phy_queue_put(enum dect_mac_phy_function function, union dect_mac_phy_handler_params *params, uint32_t priority)
{

    /* create the work */
    struct dect_mac_phy_handler_queue_work *dect_mac_phy_handler_queue_work = k_malloc(sizeof(struct dect_mac_phy_handler_queue_work)); // TODO: use memory pool instead of malloc
    if (dect_mac_phy_handler_queue_work == NULL)
    {
        LOG_ERR("Failed to allocate memory for the work");
        return -1; //TODO: return a proper error code
    }
    k_work_init(&dect_mac_phy_handler_queue_work->work, dect_mac_phy_queue_work_handler);

    

    /* create the work context */
    dect_mac_phy_handler_queue_work->function = function;
    if(params != NULL)
    {
        dect_mac_phy_handler_queue_work->params = *params;
    } 
    else
    {
        dect_mac_phy_handler_queue_work->params = (union dect_mac_phy_handler_params){0};
    }
    dect_mac_phy_handler_queue_work->priority = priority;

    /* submit the work */
    k_work_submit(&dect_mac_phy_handler_queue_work->work);

    

    return 0;
}

void dect_mac_phy_queue_work_handler(struct k_work *work)
{
    /* get the context */
    struct dect_mac_phy_handler_queue_work *work_context = CONTAINER_OF(work, struct dect_mac_phy_handler_queue_work, work);
    enum dect_mac_phy_function function = work_context->function;
    union dect_mac_phy_handler_params params = work_context->params;
    uint32_t priority = work_context->priority;

    /* free the work */
    k_free(work_context); // TODO: use memory pool instead of free

    LOG_DBG("Adding item to the queue - function: %d, priority: %u", function, priority);

    /* locks the list while working on it */
    k_mutex_lock(&queue_mutex, K_FOREVER);
    {
        /* create the item */
        struct dect_mac_phy_handler_queue_item *item;
        LOG_DBG("Allocating memory for the queue item");
        item = k_malloc(sizeof(struct dect_mac_phy_handler_queue_item)); // TODO: use memory pool instead of malloc

        /* check if the item was allocated */
        if (item == NULL)
        {
            LOG_ERR("Failed to allocate memory for the queue item");
            return;
        }

        /* init the item */
        LOG_DBG("Initializing the queue item");
        item->function = function;
        item->params = params;
        item->priority = priority;

        /* check if the maximum number of item in the queue is already reached */
        if (sys_slist_len(&dect_mac_phy_handler_queue) >= DECT_MAC_PHY_HANDLER_QUEUE_MAX_ITEMS) // warning! this operation is O(n) which means that if the queue max item is big it should probably be handled differently
        { 
            LOG_ERR("The queue is full, cannot add more items");
            k_mutex_unlock(&queue_mutex);
            return;
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
            params->rx_params.handle |= (1<<27); // set in the handle that the operation comes from the queue
            dect_mac_phy_handler_rx(params->rx_params);
            break;
        case TX:
            LOG_DBG("tx");
            params->tx_params.handle |= (1<<27); // set in the handle that the operation comes from the queue
            dect_mac_phy_handler_tx(params->tx_params);
            break;
        case TX_HARQ:
            LOG_DBG("tx harq");
            params->tx_harq_params.handle |= (1<<27); // set in the handle that the operation comes from the queue
            dect_mac_phy_handler_tx_harq(params->tx_harq_params);
            break;
        case TX_RX:
            LOG_DBG("tx rx");
            params->tx_rx_params.handle |= (1<<27); // set in the handle that the operation comes from the queue
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
            break;
        default:
            LOG_ERR("Unknown function: %d", function);
            return -1; // TODO: return a proper error code
    }
    return 0;
}

void dect_mac_phy_queue_thread()
{

    LOG_DBG("Starting the queue thread");

    k_work_queue_init(&dect_mac_phy_handler_queue_work_queue);
    k_work_queue_start(&dect_mac_phy_handler_queue_work_queue, dect_mac_phy_handler_queue_work_queue_stack, K_THREAD_STACK_SIZEOF(dect_mac_phy_handler_queue_work_queue_stack), 10, NULL);

    while (1)
    {
        /* wait for the phy layer to be free */
        k_sem_take(&phy_layer_sem, K_FOREVER);

        current_state = IDLING;
        LOG_DBG("waiting for a new item in the queue");

        /* wait for a new item in the queue */
        k_sem_take(&queue_item_sem, K_FOREVER);

        LOG_DBG("Got a new item in the queue");

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

        /* reset fail counter and execute the function */
        LOG_DBG("Executing the function");
        dect_mac_phy_handler_queue_operation_failed_counter = 0;
        dect_mac_phy_queue_function_execute(local_item.function, &local_item.params);
    }
}

void dect_mac_phy_handler_queue_operation_failed_retry(struct k_timer *timer)
{
    LOG_DBG("Retrying operation");

    /* increment the counter */
    dect_mac_phy_handler_queue_operation_failed_counter++;
    
    /* check if over the operation max retry limit */
    if(dect_mac_phy_handler_queue_operation_failed_counter > DECT_MAC_PHY_HANDLER_QUEUE_MAX_RETRY)
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
    dect_mac_phy_queue_function_execute(current_item.function, &current_item.params);
}