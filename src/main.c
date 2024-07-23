#include <zephyr/kernel.h>

#include <nrf_modem_dect_phy.h>
#include <modem/nrf_modem_lib.h>

#include "dect_mac_phy_handler.h"


#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main,4);

int main(void)
{
        LOG_DBG("Starting main");

        int err;
        err = dect_mac_phy_handler_start_modem();
        if(err){
            LOG_ERR("Failed to start modem");
            return err;
        }

        LOG_DBG("Modem started");

        uint8_t data[10];
        size_t data_size;
        data_size = sprintf(data, "Hello");

        struct dect_mac_phy_handler_tx_params tx_params = {
                .handle = 10,
                .tx_usage = NO_HARQ,
                .lbt_enable = false,
                .data = &data,
                .data_size = data_size,
                .receiver_id = 0,
                .feedback = {
                        .format = NO_FEEDBACK,
                        .info = {
                                .raw = 0,
                        },
                },
                .harq = {
                        .redundancy_version = 0,
                        .new_data_indication = 0,
                        .harq_process_nr = 0,
                        .buffer_size = 0,
                },
                .start_time = 0,                
        };
        LOG_DBG("transmitting");
        dect_mac_phy_handler_tx(tx_params);
        

        struct dect_mac_phy_handler_rx_params rx_params = {
                .handle = 20,
                .rx_mode = NRF_MODEM_DECT_PHY_RX_MODE_CONTINUOUS,
                .rx_period_ms = 10000,
                .receiver_identity = 0,
                .start_time = 0,
        };
        LOG_DBG("receiving");
        dect_mac_phy_handler_rx(rx_params);

        //k_sleep(K_MSEC(11000));

        err = dect_mac_phy_handler_stop_modem();
        if(err){
            LOG_ERR("Failed to stop modem");
            return err;
        }
        LOG_DBG("Modem stopped");

        

        return 0;
}
