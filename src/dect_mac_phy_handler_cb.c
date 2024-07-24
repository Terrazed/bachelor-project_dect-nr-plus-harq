#include "dect_mac_phy_handler_cb.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(handler_cb);

/* initialize globals variables */
struct dect_capabilities capabilities = {0};


void dect_mac_phy_init_cb(const uint64_t *time, int16_t temp, enum nrf_modem_dect_phy_err err, const struct nrf_modem_dect_phy_modem_cfg *cfg){
    LOG_DBG("init callback - time: %llu, temp: %d, err: %d", *time, temp, err);

    if(err){
        LOG_ERR("init callback - error: %d", err);
        return;
    }

    /* release the semaphore */
    k_sem_give(&phy_access_sem);
}


void dect_mac_phy_op_complete_cb(const uint64_t *time, int16_t temperature, enum nrf_modem_dect_phy_err err, uint32_t handle){
    LOG_DBG("op complete callback - time: %llu, temp: %d, err: %d, handle: %d", *time, temperature, err, handle);

    if(err){
        LOG_ERR("op complete callback - error: %d", err);
        return;
    }

    /* dont release the semaphore when switching from tx to rx in a combined operation */
    if((current_state == TRANSMITTING) && ((handle | 0xF0000000) == TX_RX)){
        // switch from tx to rx
        current_state = RECEIVING;
    }
    else{
        /* release the semaphore */
        k_sem_give(&phy_access_sem);
    }    
}


void dect_mac_phy_rssi_cb(const uint64_t *time, const struct nrf_modem_dect_phy_rssi_meas *meas){
    LOG_DBG("rssi callback - time: %llu", *time);

    /* release the semaphore */
    k_sem_give(&phy_access_sem);
}


void dect_mac_phy_rx_stop_cb(const uint64_t *time, enum nrf_modem_dect_phy_err err, uint32_t handle){
    LOG_DBG("rx stop callback - time: %llu, err: %d, handle: %d", *time, err, handle);

    if(err){
        LOG_ERR("rx stop callback - error: %d", err);
        return;
    }
}


void dect_mac_phy_pcc_cb(const uint64_t *time, const struct nrf_modem_dect_phy_rx_pcc_status *status, const union nrf_modem_dect_phy_hdr *hdr){
    LOG_DBG("pcc callback - time: %llu", *time);
}


void dect_mac_phy_pcc_crc_err_cb(const uint64_t *time, const struct nrf_modem_dect_phy_rx_pcc_crc_failure *crc_failure){
    LOG_ERR("pcc crc error callback - time: %llu", *time);
}


void dect_mac_phy_pdc_cb(const uint64_t *time, const struct nrf_modem_dect_phy_rx_pdc_status *status, const void *data, uint32_t len){
    LOG_DBG("pdc callback - time: %llu", *time);

    LOG_INF("Received data: %s", data);
}


void dect_mac_phy_pdc_crc_err_cb(const uint64_t *time, const struct nrf_modem_dect_phy_rx_pdc_crc_failure *crc_failure){
    LOG_ERR("pdc crc error callback - time: %llu", *time);
}


void dect_mac_phy_link_config_cb(const uint64_t *time, enum nrf_modem_dect_phy_err err){
    LOG_DBG("link config callback - time: %llu, err: %d", *time, err);

    if(err){
        LOG_ERR("link config callback - error: %d", err);
        return;
    }

    /* release the semaphore */
    k_sem_give(&phy_access_sem);
}


void dect_mac_phy_time_get_cb(const uint64_t *time, enum nrf_modem_dect_phy_err err){
    LOG_DBG("time get callback - time: %llu, err: %d", *time, err);

    if(err){
        LOG_ERR("time get callback - error: %d", err);
        return;
    }

    /* release the semaphore */
    k_sem_give(&phy_access_sem);
}


void dect_mac_phy_capability_get_cb(const uint64_t *time, enum nrf_modem_dect_phy_err err, const struct nrf_modem_dect_phy_capability *capability){
    LOG_DBG("capability get callback - time: %llu, err: %d", *time, err);

    if(err){
        LOG_ERR("capability get callback - error: %d", err);
        k_sem_give(&phy_access_sem);
        return;
    }

    /* save the capabilities of the modem */
    capabilities.dect_version = capability->dect_version;
    capabilities.power_class = capability->variant[0].power_class;
    capabilities.rx_spatial_streams = capability->variant[0].rx_spatial_streams;
    capabilities.rx_tx_diversity = capability->variant[0].rx_tx_diversity;
    capabilities.rx_gain = capability->variant[0].rx_gain;
    capabilities.mcs_max = capability->variant[0].mcs_max;
    capabilities.harq_soft_buf_size = capability->variant[0].harq_soft_buf_size;
    capabilities.harq_process_count_max = capability->variant[0].harq_process_count_max;
    capabilities.harq_feedback_delay = capability->variant[0].harq_feedback_delay;
    capabilities.mu = capability->variant[0].mu;
    capabilities.beta = capability->variant[0].beta;

    /* release the semaphore */
    k_sem_give(&phy_access_sem);
}


void dect_mac_phy_deinit_cb(const uint64_t *time, enum nrf_modem_dect_phy_err err){
    LOG_DBG("deinit callback - time: %llu, err: %d", *time, err);

    if(err){
        LOG_ERR("deinit callback - error: %d", err);
        return;
    }

    /* release the semaphore */
    k_sem_give(&phy_access_sem);
    
}


struct nrf_modem_dect_phy_callbacks* dect_mac_phy_handler_get_callbacks(void){
    static struct nrf_modem_dect_phy_callbacks dect_phy_callbacks;

	dect_phy_callbacks.init = dect_mac_phy_init_cb;
	dect_phy_callbacks.deinit = dect_mac_phy_deinit_cb;
	dect_phy_callbacks.op_complete = dect_mac_phy_op_complete_cb;
	dect_phy_callbacks.rx_stop = dect_mac_phy_rx_stop_cb;
	dect_phy_callbacks.pcc = dect_mac_phy_pcc_cb;
	dect_phy_callbacks.pcc_crc_err = dect_mac_phy_pcc_crc_err_cb;
	dect_phy_callbacks.pdc = dect_mac_phy_pdc_cb;
	dect_phy_callbacks.pdc_crc_err = dect_mac_phy_pdc_crc_err_cb;
	dect_phy_callbacks.rssi = dect_mac_phy_rssi_cb;
	dect_phy_callbacks.link_config = dect_mac_phy_link_config_cb;
	dect_phy_callbacks.time_get = dect_mac_phy_time_get_cb;
	dect_phy_callbacks.capability_get = dect_mac_phy_capability_get_cb;
    
	return &dect_phy_callbacks;
}