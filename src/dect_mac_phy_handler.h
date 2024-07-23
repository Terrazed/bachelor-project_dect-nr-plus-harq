#ifndef DECT_MAC_PHY_HANDLER_H
#define DECT_MAC_PHY_HANDLER_H

#include <zephyr/kernel.h>
#include <nrf_modem_dect_phy.h>

#include "dect_mac_phy_handler_cb.h"

enum dect_mac_phy_function {
    CAPABILITY_GET = 1,
    INIT = 2,
    DEINIT = 3,
    RX = 4,
    TX = 5,
    TX_HARQ = 6,
    TX_RX = 7,
    RSSI = 8,
    RX_STOP = 9,
    LINK_CONFIG = 10,
    TIME_GET = 11,
};

/* semaphore to protect the access to the phy layer api (declared in dect_mac_phy_handler.c)*/
extern struct k_sem phy_access_sem;

struct dect_mac_phy_handler_rx_params {
    uint32_t handle : 28;
    enum nrf_modem_dect_phy_rx_mode rx_mode;
    uint32_t rx_period_ms;
    uint16_t receiver_identity;
    uint64_t start_time;
};

/* functions to start and stop the modem, the modem should be started when using the other commands */
int dect_mac_phy_handler_start_modem();
int dect_mac_phy_handler_stop_modem();

/* functions that simplify the access to the phy layer api */
void dect_mac_phy_handler_capability_get();
void dect_mac_phy_handler_init();
void dect_mac_phy_handler_deinit();
void dect_mac_phy_handler_rx(struct dect_mac_phy_handler_rx_params params);
void dect_mac_phy_handler_tx();
void dect_mac_phy_handler_tx_harq();
void dect_mac_phy_handler_tx_rx();
void dect_mac_phy_handler_rssi();
void dect_mac_phy_handler_rx_stop();
void dect_mac_phy_handler_link_config();
void dect_mac_phy_handler_time_get();


/* variable that holds the capability of the modem (declared in dect_mac_phy_handler_cb.c) */
extern struct dect_capabilities capabilities;

/* semaphore to protect the access to the phy layer api (declared in dect_mac_phy_handler.c)*/
extern struct k_sem phy_access_sem;



#endif // DECT_MAC_PHY_HANDLER_H