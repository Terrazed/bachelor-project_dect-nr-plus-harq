#ifndef DECT_MAC_PHY_HANDLER_CB_H
#define DECT_MAC_PHY_HANDLER_CB_H

#include <zephyr/kernel.h>
#include <nrf_modem_dect_phy.h>


/* struct that represents the capabilities of the modem. */
struct dect_capabilities {
    uint8_t dect_version;
    uint8_t power_class;
    uint8_t rx_spatial_streams;
    uint8_t rx_tx_diversity;
    int8_t rx_gain;
    uint8_t mcs_max;
    uint32_t harq_soft_buf_size;
    uint8_t harq_process_count_max;
    uint8_t harq_feedback_delay;
    uint8_t mu;
    uint8_t beta;
};

/* variable that holds the capability of the modem (declared in dect_mac_phy_handler_cb.c) */
extern struct dect_capabilities capabilities;

/* semaphore to protect the access to the phy layer api (declared in dect_mac_phy_handler.c)*/
extern struct k_sem phy_access_sem;


/* Callback after init operation. */
void dect_mac_phy_init_cb(const uint64_t *time, int16_t temp, enum nrf_modem_dect_phy_err err, const struct nrf_modem_dect_phy_modem_cfg *cfg);

/* Operation complete notification. */
void dect_mac_phy_op_complete_cb(const uint64_t *time, int16_t temperature, enum nrf_modem_dect_phy_err err, uint32_t handle);

/* RSSI measurement result notification. */
void dect_mac_phy_rssi_cb(const uint64_t *time, const struct nrf_modem_dect_phy_rssi_meas *meas);

/* Callback after receive stop operation. */
void dect_mac_phy_rx_stop_cb(const uint64_t *time, enum nrf_modem_dect_phy_err err, uint32_t handle);

/* Physical Control Channel reception notification. */
void dect_mac_phy_pcc_cb(const uint64_t *time, const struct nrf_modem_dect_phy_rx_pcc_status *status, const union nrf_modem_dect_phy_hdr *hdr);

/* Physical Control Channel CRC error notification. */
void dect_mac_phy_pcc_crc_err_cb(const uint64_t *time, const struct nrf_modem_dect_phy_rx_pcc_crc_failure *crc_failure);

/* Physical Data Channel reception notification. */
void dect_mac_phy_pdc_cb(const uint64_t *time, const struct nrf_modem_dect_phy_rx_pdc_status *status, const void *data, uint32_t len);

/* Physical Data Channel CRC error notification. */
void dect_mac_phy_pdc_crc_err_cb(const uint64_t *time, const struct nrf_modem_dect_phy_rx_pdc_crc_failure *crc_failure);

/* Callback after link configuration operation. */
void dect_mac_phy_link_config_cb(const uint64_t *time, enum nrf_modem_dect_phy_err err);

/* Callback after time query operation. */
void dect_mac_phy_time_get_cb(const uint64_t *time, enum nrf_modem_dect_phy_err err);

/* Callback after capability get operation. */
void dect_mac_phy_capability_get_cb(const uint64_t *time, enum nrf_modem_dect_phy_err err, const struct nrf_modem_dect_phy_capability *capability);

/* Callback after deinit operation. */
void dect_mac_phy_deinit_cb(const uint64_t *time, enum nrf_modem_dect_phy_err err);



/* getter for dect_phy_callbacks */
struct nrf_modem_dect_phy_callbacks* dect_mac_phy_handler_get_callbacks(void);


#endif // DECT_MAC_PHY_HANDLER_CB_H