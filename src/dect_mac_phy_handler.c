#include "dect_mac_phy_handler.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(handler);

K_SEM_DEFINE(phy_access_sem, 1, 1);
uint16_t device_id = 0;

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

    /* Get the device ID. */
    hwinfo_get_device_id((void *)&device_id, sizeof(device_id));

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

    /* take the semaphore */
    k_sem_take(&phy_access_sem, K_FOREVER);

    /* get the capability of the dect phy modem */
    int ret = nrf_modem_dect_phy_capability_get();
    if(ret){
        LOG_ERR("nrf_modem_dect_phy_capability_get() returned %d", ret);
    }
    
}


void dect_mac_phy_handler_init(){

    /* take the semaphore */
    k_sem_take(&phy_access_sem, K_FOREVER);
    
    /* create init parameters */
    const struct nrf_modem_dect_phy_init_params params = {
        .harq_rx_expiry_time_us = CONFIG_HARQ_RX_EXPIRY_TIME_US,
        .harq_rx_process_count = capabilities.harq_process_count_max,
    };

    /* initialize the dect phy modem */
    int ret = nrf_modem_dect_phy_init(&params);
    if(ret){
        LOG_ERR("nrf_modem_dect_phy_init() returned %d", ret);
    }
}


void dect_mac_phy_handler_deinit(){

    /* take the semaphore */
    k_sem_take(&phy_access_sem, K_FOREVER);

    /* deinitialize the dect phy modem */
    int ret = nrf_modem_dect_phy_deinit();
    if(ret){
        LOG_ERR("nrf_modem_dect_phy_deinit() returned %d", ret);
    }
}


void dect_mac_phy_handler_rx(struct dect_mac_phy_handler_rx_params params){

    /* create true params */
    const struct nrf_modem_dect_phy_rx_params true_params = {
        .start_time = params.start_time,
        .handle = ((RX << 28) | (params.handle & 0x0fffffff)),
        .network_id = CONFIG_NETWORK_ID,
        .mode = params.rx_mode,
        .rssi_interval = NRF_MODEM_DECT_PHY_RSSI_INTERVAL_OFF,
        .link_id = NRF_MODEM_DECT_PHY_LINK_UNSPECIFIED,
        .rssi_level = CONFIG_RSSI_TARGET,
        .carrier = CONFIG_CARRIER,
        .duration = params.rx_period_ms,
        .filter = {
            .short_network_id = CONFIG_NETWORK_ID,
            .is_short_network_id_used = true,
            .receiver_identity = params.receiver_identity,
        }
    };

    /* take the semaphore */
    k_sem_take(&phy_access_sem, K_FOREVER);

    /* start the reception */
    int ret = nrf_modem_dect_phy_rx(&true_params);
    if(ret){
        LOG_ERR("nrf_modem_dect_phy_rx() returned %d", ret);
    }
}


void dect_mac_phy_handler_tx(struct dect_phy_handler_tx_params params){

    /* create true params */
    DECT_MAC_PHY_HANDLER_TRUE_PARAM_CREATE(true_params, params);
    
    /* start the transmission */
    int ret = nrf_modem_dect_phy_tx(&true_params);
    if(ret){
        LOG_ERR("nrf_modem_dect_phy_tx() returned %d", ret);
    }

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

void dect_mac_phy_handler_tx_config(struct dect_phy_handler_tx_params *input_params, struct nrf_modem_dect_phy_tx_params *output_params){

    /* set the time */
    output_params->start_time = input_params->start_time;

    /* set the handle */
    output_params->handle = ((TX << 28) | (input_params->handle & 0x0fffffff));

    /* set the network id */
    output_params->network_id = CONFIG_NETWORK_ID;

    /* setup lbt */
    output_params->lbt_rssi_threshold_max = CONFIG_RSSI_TARGET;
    output_params->lbt_period = input_params->lbt_enable ? NRF_MODEM_DECT_LBT_PERIOD_MAX : 0;

    /* set the carrier */
    output_params->carrier = CONFIG_CARRIER;

    uint32_t tx_power = 11; //TODO: create a function to calculate these
    uint32_t df_mcs = 2;

    uint32_t packet_length_type = 0; //TODO: create a function to calculate these
    uint32_t packet_length = 0;

    if(input_params->tx_usage == BEACON){
        output_params->phy_type = HEADER_TYPE_1;
        struct phy_ctrl_field_common_type1 *header = (struct phy_ctrl_field_common_type1 *)output_params->phy_header;

        header->header_format = HEADER_FORMAT_000;
        header->packet_length_type = packet_length_type;
        header->packet_length = packet_length;
        header->short_network_id = (CONFIG_NETWORK_ID & 0xff);
        header->transmitter_id_hi = (device_id >> 8);
        header->transmitter_id_lo = (device_id & 0xff);
        header->transmit_power = tx_power; //TODO: set to the worst connection in the registered nodes
        header->reserved = 0;
        header->df_mcs = df_mcs; //TODO: set to the worst connection in the registered nodes
    }
    else{
        output_params->phy_type = HEADER_TYPE_2;
        struct phy_ctrl_field_common_type2 *header = (struct phy_ctrl_field_common_type2 *)output_params->phy_header;

        header->header_format = HEADER_FORMAT_001;
        header->packet_length_type = packet_length_type;
        header->packet_length = packet_length;
        header->short_network_id = (CONFIG_NETWORK_ID & 0xff);
        header->transmitter_id_hi = (device_id >> 8);
        header->transmitter_id_lo = (device_id & 0xff);
        header->transmit_power = tx_power;
        header->df_mcs = df_mcs;
        header->receiver_id_hi = (input_params->receiver_id >> 8);
        header->receiver_id_lo = (input_params->receiver_id & 0xff);
        header->spatial_streams = 0; // device does not support spatial streams
        header->feedback_format = input_params->feedback.format;
        header->feedback_info_hi = input_params->feedback.info.byte.hi;
        header->feedback_info_lo = input_params->feedback.info.byte.lo;

        if(input_params->tx_usage == HARQ){
            header->df_red_version = input_params->harq.redundancy_version;
            header->df_ind = input_params->harq.new_data_indication;
            header->df_harq_process_nr = input_params->harq.harq_process_nr;
        }
        else{
            header->df_red_version = 0;
            header->df_ind = 0;
            header->df_harq_process_nr = 0;
        }
    }

    output_params->bs_cqi = NRF_MODEM_DECT_PHY_BS_CQI_NOT_USED; //TODO: implement cqi and buffer
    output_params->data = input_params->data;
    output_params->data_size = input_params->data_size;

}