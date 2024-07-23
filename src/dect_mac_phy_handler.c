#include "dect_mac_phy_handler.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(handler);

K_SEM_DEFINE(phy_access_sem, 1, 1);

int dect_mac_phy_handler_start_modem(){

    int ret; // Return value

    /* initialize the modem lib */
    ret = nrf_modem_lib_init();
    if(ret){
        LOG_ERR("nrf_modem_lib_init() returned %d", ret);
        return ret;
    }

    /* set the callbacks for the dect phy modem */
    ret = nrf_modem_dect_phy_callback_set(dect_mac_phy_handler_get_callbacks());
    if(ret){
        LOG_ERR("nrf_modem_dect_phy_callback_set() returned %d", ret);
        return ret;
    }

    /* get the capability of the api */
    dect_mac_phy_handler_capability_get();

    /* initialize the dect phy modem */
    dect_mac_phy_handler_init();

    return 0; // Success
}


int dect_mac_phy_handler_stop_modem(){

    int ret;

    /* deinitialize the dect phy modem */
    dect_mac_phy_handler_deinit();

    /* deinitialize the modem lib */
    ret = nrf_modem_lib_shutdown();
    if(ret){
        LOG_ERR("nrf_modem_lib_shutdown() returned %d", ret);
        return ret;
    }

    return 0; // Success
}


void dect_mac_phy_handler_capability_get(){

    k_sem_take(&phy_access_sem, K_FOREVER);

    int ret = nrf_modem_dect_phy_capability_get();
    if(ret){
        LOG_ERR("nrf_modem_dect_phy_capability_get() returned %d", ret);
    }
    
}


void dect_mac_phy_handler_init(){

    k_sem_take(&phy_access_sem, K_FOREVER);
    
    const struct nrf_modem_dect_phy_init_params params = {
        .harq_rx_expiry_time_us = CONFIG_HARQ_RX_EXPIRY_TIME_US,
        .harq_rx_process_count = capabilities.harq_process_count_max,
    };

    int ret = nrf_modem_dect_phy_init(&params);
    if(ret){
        LOG_ERR("nrf_modem_dect_phy_init() returned %d", ret);
    }
}


void dect_mac_phy_handler_deinit(){
    
    k_sem_take(&phy_access_sem, K_FOREVER);

    int ret = nrf_modem_dect_phy_deinit();
    if(ret){
        LOG_ERR("nrf_modem_dect_phy_deinit() returned %d", ret);
    }
}


void dect_mac_phy_handler_rx(){

    
}


void dect_mac_phy_handler_tx(){

    
}


void dect_mac_phy_handler_tx_harq(){

    
}


void dect_mac_phy_handler_tx_rx(){

    
}


void dect_mac_phy_handler_rssi(){

    
}


void dect_mac_phy_handler_rx_stop(){

    
}


void dect_mac_phy_handler_link_config(){

    
}


void dect_mac_phy_handler_time_get(){

    
}

