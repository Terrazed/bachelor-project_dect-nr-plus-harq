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

        err = dect_mac_phy_handler_stop_modem();
        if(err){
            LOG_ERR("Failed to stop modem");
            return err;
        }
        LOG_DBG("Modem stopped");

        

        return 0;
}
