#include <zephyr/kernel.h>

#include <nrf_modem_dect_phy.h>
#include <modem/nrf_modem_lib.h>

#include <dk_buttons_and_leds.h>

#include "dect_mac_phy_handler.h"
#include "dect_mac_phy_handler_types.h"
#include "dect_mac_harq.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, 4);


void button1_pressed(){
    dk_set_led(DK_LED1, 1);
    k_sleep(K_MSEC(200));
    dk_set_led(DK_LED1, 0);

    static uint8_t data[700];
    static uint16_t data_len;
    data_len = sprintf(data, "Hello Worldi 123456789016780167890123456789012345678901234567890123456789012345678901234567890678901234567890123456789012345678901234567890123456789012345678901234567890123dsafasdfsadfsadfasdfasdfasdfasdfasdfsadf45678906789012345678901234567890123456345678901234567890123456789012345678901234567890123456789012345678901test");

    struct dect_mac_harq_transmit_params params = {
        .data = data,
        .data_len = data_len,
        .start_time = 0,
        .receiver_id = 14231,
    };

    dect_mac_harq_transmit(params);

    struct dect_mac_phy_handler_rx_stop_params rx_stop_params = {
        .handle = 0x48000014,
    };

    dect_mac_phy_handler_rx_stop(rx_stop_params);
}

void button2_pressed(){
    dk_set_led(DK_LED2, 1);
    k_sleep(K_MSEC(200));
    dk_set_led(DK_LED2, 0);

    static uint8_t data[700];
    static uint16_t data_len;
    data_len = sprintf(data, "Hello Worldo");

    struct dect_mac_harq_transmit_params params = {
        .data = data,
        .data_len = data_len,
        .start_time = 0,
        .receiver_id = 29054,
    };

    dect_mac_harq_transmit(params);

    struct dect_mac_phy_handler_rx_stop_params rx_stop_params = {
        .handle = 0x48000014,
    };

    dect_mac_phy_handler_rx_stop(rx_stop_params);
}

void button3_pressed(){
    dk_set_led(DK_LED3, 1);
    k_sleep(K_MSEC(200));
    dk_set_led(DK_LED3, 0);
}

void button4_pressed(){
    dk_set_led(DK_LED4, 1);
    k_sleep(K_MSEC(200));
    dk_set_led(DK_LED4, 0);
}

void button_handler(uint32_t button_state, uint32_t has_changed)
{
    switch (has_changed)
    {
    case DK_BTN1_MSK:
        if (button_state & DK_BTN1_MSK)
        {
            LOG_INF("Button 1 pressed");
            button1_pressed();
        }
        break;
    case DK_BTN2_MSK:
        if (button_state & DK_BTN2_MSK)
        {
            LOG_INF("Button 2 pressed");
            button2_pressed();
        }
        break;
    case DK_BTN3_MSK:
        if (button_state & DK_BTN3_MSK)
        {
            LOG_INF("Button 3 pressed");
            button3_pressed();
        }
        break;
    case DK_BTN4_MSK:
        if (button_state & DK_BTN4_MSK)
        {
            LOG_INF("Button 4 pressed");
            button4_pressed();
        }
        break;
    }

}


int main(void)
{
    LOG_DBG("Starting main");

    int err;

    /* initialize the buttons */
    dk_buttons_init(button_handler);
    if(err)
    {
        LOG_ERR("Failed to initialize buttons");
        return err;
    }

    /* initialize the leds */
    dk_leds_init();
    if(err)
    {
        LOG_ERR("Failed to initialize leds");
        return err;
    }

    /* initialize the modem */
    
    err = dect_mac_phy_handler_start_modem();
    if (err)
    {
        LOG_ERR("Failed to start modem");
        return err;
    }

    LOG_DBG("Modem started");

    /* permanent receiving */
    struct dect_mac_phy_handler_rx_params rx_params = {
        .handle = 0x14,
        .rx_mode = NRF_MODEM_DECT_PHY_RX_MODE_SINGLE_SHOT,
        .rx_period_ms = 10000,
        .receiver_identity = 0,
        .start_time = 0,
    };
    dect_phy_queue_put(RX, (union dect_mac_phy_handler_params*)&rx_params, PRIORITY_PERMANENT);    

    return 0;
}


