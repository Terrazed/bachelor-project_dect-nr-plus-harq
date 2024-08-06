#ifndef DECT_MAC_PHY_HANDLER_CB_H
#define DECT_MAC_PHY_HANDLER_CB_H

#include <zephyr/kernel.h>
#include <nrf_modem_dect_phy.h>

#include "dect_mac_phy_handler_types.h"
#include "dect_mac_phy_handler_queue.h"
#include "dect_mac_harq.h"
#include "dect_mac_utils.h"

/* variable that holds the capability of the modem (declared in dect_mac_phy_handler_cb.c) */
extern struct dect_capabilities capabilities;

/* semaphore to protect the access to the phy layer api (declared in dect_mac_phy_handler.c)*/
extern struct k_sem phy_access_sem;

/* variable that represent the current state of the modem (declared in dect_mac_phy_handler.c) */
extern enum dect_mac_phy_state current_state;

/* Callback after init operation. */
void dect_mac_phy_init_cb(const struct nrf_modem_dect_phy_init_event *evt);

/* Operation complete notification. */
void dect_mac_phy_op_complete_cb(const struct nrf_modem_dect_phy_op_complete_event *evt);

/* RSSI measurement result notification. */
void dect_mac_phy_rssi_cb(const struct nrf_modem_dect_phy_rssi_event *evt);

/* Callback after receive stop operation. */
void dect_mac_phy_cancel_cb(const struct nrf_modem_dect_phy_cancel_event *evt);

/* Physical Control Channel reception notification. */
void dect_mac_phy_pcc_cb(const struct nrf_modem_dect_phy_pcc_event *evt);

/* Physical Control Channel CRC error notification. */
void dect_mac_phy_pcc_crc_err_cb(const struct nrf_modem_dect_phy_pcc_crc_failure_event *evt);

/* Physical Data Channel reception notification. */
void dect_mac_phy_pdc_cb(const struct nrf_modem_dect_phy_pdc_event *evt);

/* Physical Data Channel CRC error notification. */
void dect_mac_phy_pdc_crc_err_cb(const struct nrf_modem_dect_phy_pdc_crc_failure_event *evt);

/* Callback after radio configuration operation */
void dect_mac_phy_radio_config_cb(const struct nrf_modem_dect_phy_radio_config_event *evt);

/* Callback after link configuration operation. */
void dect_mac_phy_link_config_cb(const struct nrf_modem_dect_phy_link_config_event *evt);

/* Callback after time query operation. */
void dect_mac_phy_time_get_cb(const struct nrf_modem_dect_phy_time_get_event *evt);

/* Callback after capability get operation. */
void dect_mac_phy_capability_get_cb(const struct nrf_modem_dect_phy_capability_get_event *evt);

/* Callback after deinit operation. */
void dect_mac_phy_deinit_cb(const struct nrf_modem_dect_phy_deinit_event *evt);

/* getter for dect_phy_callbacks */
struct nrf_modem_dect_phy_callbacks *dect_mac_phy_handler_get_callbacks(void);

#endif // DECT_MAC_PHY_HANDLER_CB_H