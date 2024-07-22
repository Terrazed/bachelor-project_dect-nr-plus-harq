#ifndef DECT_MAC_PHY_HANDLER_H
#define DECT_MAC_PHY_HANDLER_H

#include <zephyr/kernel.h>
#include <nrf_modem_dect_phy.h>

int dect_mac_phy_handler_start_modem();
int dect_mac_phy_handler_stop_modem();

void dect_mac_phy_handler_capability_get();
void dect_mac_phy_handler_init();
void dect_mac_phy_handler_deinit();
void dect_mac_phy_handler_rx();
void dect_mac_phy_handler_tx();
void dect_mac_phy_handler_tx_harq();
void dect_mac_phy_handler_tx_rx();
void dect_mac_phy_handler_rssi();
void dect_mac_phy_handler_rx_stop();
void dect_mac_phy_handler_link_config();
void dect_mac_phy_handler_time_get();





#endif // DECT_MAC_PHY_HANDLER_H