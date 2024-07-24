#ifndef DECT_MAC_PHY_HANDLER_H
#define DECT_MAC_PHY_HANDLER_H

#include <zephyr/kernel.h>
#include <nrf_modem_dect_phy.h>
#include <zephyr/drivers/hwinfo.h>

#include "dect_mac_phy_handler_types.h"
#include "dect_mac_phy_handler_cb.h"

/* functions to start and stop the modem, the modem should be started when using the other commands */
int dect_mac_phy_handler_start_modem();
int dect_mac_phy_handler_stop_modem();

/* functions that simplify the access to the phy layer api */
void dect_mac_phy_handler_capability_get();
void dect_mac_phy_handler_init();
void dect_mac_phy_handler_deinit();
void dect_mac_phy_handler_rx(struct dect_mac_phy_handler_rx_params params);
void dect_mac_phy_handler_tx(struct dect_mac_phy_handler_tx_params params);
void dect_mac_phy_handler_tx_harq(struct dect_mac_phy_handler_tx_harq_params params);
void dect_mac_phy_handler_tx_rx(struct dect_mac_phy_handler_tx_rx_params params);
void dect_mac_phy_handler_rssi(struct dect_mac_phy_handler_rssi_params params);
void dect_mac_phy_handler_rx_stop(struct dect_mac_phy_handler_rx_stop_params params);
void dect_mac_phy_handler_link_config();
void dect_mac_phy_handler_time_get();

#define DECT_MAC_PHY_HANDLER_TRUE_TX_PARAM_CREATE(name, phy_handler_param) \
    struct nrf_modem_dect_phy_tx_params name;                              \
    union nrf_modem_dect_phy_hdr header;                                   \
    name.phy_header = &header;                                             \
    dect_mac_phy_handler_tx_config(&phy_handler_param, &name);

void dect_mac_phy_handler_tx_config(struct dect_mac_phy_handler_tx_params *input_params, struct nrf_modem_dect_phy_tx_params *output_params);

#define DECT_MAC_PHY_HANDLER_TRUE_RX_PARAM_CREATE(name, phy_handler_param) \
    struct nrf_modem_dect_phy_rx_params name;                              \
    dect_mac_phy_handler_rx_config(&phy_handler_param, &name);

void dect_mac_phy_handler_rx_config(struct dect_mac_phy_handler_rx_params *input_params, struct nrf_modem_dect_phy_rx_params *output_params);

/* variable that holds the capability of the modem (declared in dect_mac_phy_handler_cb.c) */
extern struct dect_capabilities capabilities;

/* variable that holds the device id of the device (declared in dect_mac_phy_handler.c) */
extern uint16_t device_id;

/* semaphore to protect the access to the phy layer api (declared in dect_mac_phy_handler.c) */
extern struct k_sem phy_access_sem;

/* variable that represent the current state of the modem (declared in dect_mac_phy_handler.c) */
extern enum dect_mac_phy_state current_state;

#endif // DECT_MAC_PHY_HANDLER_H