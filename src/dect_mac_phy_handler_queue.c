#include "dect_mac_phy_handler_queue.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(phy_queue);


K_THREAD_DEFINE(dect_phy_queue_thread_id, DECT_MAC_PHY_HANDLER_QUEUE_THREAD_STACK_SIZE, dect_phy_queue_thread, NULL, NULL, NULL, 10, 0, 0);

sys_slist_t dect_mac_phy_handler_queue;
struct k_sem phy_layer_sem;
struct k_mutex queue_mutex;


int dect_phy_queue_put(enum dect_mac_phy_function function, union dect_mac_phy_handler_params *params, uint32_t priority)
{
    int ret;

    /* check if the limit is already reached */
    if(sys_slist_len(&dect_mac_phy_handler_queue) >= DECT_MAC_PHY_HANDLER_QUEUE_MAX_ITEMS){ // warning! this operation is O(n) which means that if the queue max item is big it should probably be handled differently
        LOG_ERR("The queue is full, cannot add more items");
        return -1; // TODO: return a proper error code
    }

    LOG_DBG("Adding item to the queue - function: %d, priority: %d", function, priority);

    /* create the item */
    struct dect_phy_handler_queue_item *item;
    item = k_malloc(sizeof(struct dect_phy_handler_queue_item));

    /* check if the item was allocated */
    if(item == NULL){
        LOG_ERR("Failed to allocate memory for the queue item");
        return -1; // TODO: return a proper error code
    }
    /* init the item */
    item->function = function;
    item->params = *params;
    item->priority = priority;

    /* locks the list while working on it */
    k_mutex_lock(&queue_mutex, K_FOREVER);
    {
        /* if the list is empty, add the item to the head */
        if(sys_slist_is_empty(&dect_mac_phy_handler_queue)){
            sys_slist_prepend(&dect_mac_phy_handler_queue, &item->node);
        }
        /* if the list is not empty, add the item to the correct position */
        else{
            bool item_inserted = false; // flag to check if the item was inserted
            sys_snode_t *previous_node = NULL; // previous node to insert the item
            for(sys_snode_t *node = sys_slist_peek_head(&dect_mac_phy_handler_queue); // create a node to iterate through the list
                node != NULL; // iterate until the end of the list (but will break if the item is inserted before the end of the list)
                previous_node = node, node = sys_slist_peek_next(node)){ // set the previous node to the current node and the current node to the next node
                /* get the current item */
                struct dect_phy_handler_queue_item *current_item = CONTAINER_OF(node, struct dect_phy_handler_queue_item, node);
                /* if the current item has a lower priority than the item to insert, insert the item before the current item */
                if(current_item->priority < item->priority){
                    sys_slist_insert(&dect_mac_phy_handler_queue, previous_node, &item->node);
                    item_inserted = true;
                    break;
                }
            }
            /* if the item was not inserted, add it to the end of the list */
            if(!item_inserted){
                sys_slist_append(&dect_mac_phy_handler_queue, &item->node);
            }
        }
    }
    k_mutex_unlock(&queue_mutex);
    
    return ret;
}

void dect_phy_queue_thread()
{


}