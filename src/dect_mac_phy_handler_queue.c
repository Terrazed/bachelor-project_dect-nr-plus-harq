#include "dect_mac_phy_handler_queue.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(phy_queue);


sys_slist_t dect_mac_phy_handler_queue;
struct k_sem phy_layer_sem;
struct k_mutex queue_mutex;




int dect_phy_queue_put(enum dect_mac_phy_function function, union dect_mac_phy_handler_params *params, uint32_t priority)
{
    int ret;


    
    return ret;
}

void dect_phy_queue_thread()
{}