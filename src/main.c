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

    struct dect_mac_phy_handler_cancel_params rx_stop_params = {
        .handle = 0x48000014,
    };

    dect_mac_phy_handler_cancel(rx_stop_params);
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

    struct dect_mac_phy_handler_cancel_params rx_stop_params = {
        .handle = 0x48000014,
    };

    dect_mac_phy_handler_cancel(rx_stop_params);
}

static uint8_t radio_mode = 0;

void button3_pressed(){
    dk_set_led(DK_LED3, 1);
    k_sleep(K_MSEC(200));
    dk_set_led(DK_LED3, 0);

    radio_mode = (radio_mode + 1)%3;

    struct dect_mac_phy_handler_radio_config_params radio_params = {
        .handle = 0x187,
        .radio_mode = radio_mode,
        .start_time = 0,
    };

    dect_phy_queue_put(RADIO_CONFIG, (union dect_mac_phy_handler_params*)&radio_params, PRIORITY_HIGH);

    struct dect_mac_phy_handler_cancel_params rx_stop_params = {
        .handle = 0x48000014,
    };

    dect_mac_phy_handler_cancel(rx_stop_params);
}

void button4_pressed(){
    dk_set_led(DK_LED4, 1);
    k_sleep(K_MSEC(200));
    dk_set_led(DK_LED4, 0);

    struct dect_mac_phy_handler_rx_params rx_params = {
        .handle = 0x14,
        .rx_mode = NRF_MODEM_DECT_PHY_RX_MODE_SINGLE_SHOT,
        .rx_period_ms = 10000,
        .receiver_identity = 0,
        .start_time = 0,
    };
    dect_phy_queue_put(RX, (union dect_mac_phy_handler_params*)&rx_params, PRIORITY_HIGH);    
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

    k_sleep(K_SECONDS(1));

    struct dect_mac_phy_handler_radio_config_params radio_params = {
        .handle = 0x187,
        .radio_mode = NRF_MODEM_DECT_PHY_RADIO_MODE_LOW_LATENCY_WITH_STANDBY,
        .start_time = 0,
    };

    dect_phy_queue_put(RADIO_CONFIG, (union dect_mac_phy_handler_params*)&radio_params, PRIORITY_HIGH);

    k_sleep(K_SECONDS(5));

    LOG_DBG("Starting test");

    static const int16_t byte_per_mcs_and_length[5][16] = {
    { 0,  17,  33,  50,  67,  83,  99, 115, 133, 149, 165, 181, 197, 213, 233, 249},
    { 4,  37,  69, 103, 137, 169, 201, 233, 263, 295, 327, 359, 391, 423, 463, 495},
    { 7,  57, 107, 157, 205, 253, 295, 343, 399, 447, 495, 540, 596, 644, 692,  -1},
    {11,  77, 141, 209, 271, 335, 399, 463, 532, 596, 660,  -1,  -1,  -1,  -1,  -1},
    {18, 117, 217, 311, 407, 503, 604, 700,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}};

    LOG_INF("MCS,Power,Packet Length,Bytes");

bool tx = true;
if(tx)
{
    for(int mcs = 0; mcs <= 4; mcs++)
    {
        for(int power = 0; power <= CONFIG_TX_POWER; power++)
        {
            for(int packet_length = 0; packet_length < 16; packet_length++)
            {
                if(byte_per_mcs_and_length[mcs][packet_length] == -1)
                {
                    continue;
                }
                for(int repeat = 0; repeat < 8; repeat++)
                {
                    LOG_INF("%d,%d,%d,%d", mcs, power, packet_length, byte_per_mcs_and_length[mcs][packet_length]);
                    test_mcs = mcs;
                    test_power = power;
                    test_packet_length = packet_length;

                    static uint8_t data[700];
                    static uint16_t data_len;
                    data_len = sprintf(data, "Hello World");

                    struct dect_mac_phy_handler_tx_params tx_params = {
                        .handle = 0x1234,
                        .tx_usage = NO_HARQ,
                        .lbt_enable = false,
                        .data = data,
                        .data_size = data_len,
                        .receiver_id = 14231,
                        .feedback = NO_FEEDBACK,
                        .harq = NO_HARQ,
                        .start_time = 0,
                    };

                    dect_phy_queue_put(TX, (union dect_mac_phy_handler_params*)&tx_params, PRIORITY_HIGH);

                    k_sleep(K_MSEC(10));
                }
                
                
            }
        }
    }
}
else
{
    /* permanent receiving */
    struct dect_mac_phy_handler_rx_params rx_params = {
        .handle = 0x14,
        .rx_mode = NRF_MODEM_DECT_PHY_RX_MODE_SINGLE_SHOT,
        .rx_period_ms = 10000,
        .receiver_identity = 0,
        .start_time = 0,
    };
    dect_phy_queue_put(RX, (union dect_mac_phy_handler_params*)&rx_params, PRIORITY_PERMANENT);    
}
    return 0;
}


