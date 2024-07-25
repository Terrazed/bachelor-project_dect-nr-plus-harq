#include "dect_mac_phy_handler.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(handler);

uint16_t device_id = 0;
enum dect_mac_phy_state current_state = IDLING;

int dect_mac_phy_handler_start_modem()
{

    int ret; // Return value

    /* initialize the modem lib */
    ret = nrf_modem_lib_init();
    if (ret)
    {
        LOG_ERR("nrf_modem_lib_init() returned %d", ret);
        return ret;
    }

    /* set the callbacks for the dect phy modem */
    ret = nrf_modem_dect_phy_callback_set(dect_mac_phy_handler_get_callbacks());
    if (ret)
    {
        LOG_ERR("nrf_modem_dect_phy_callback_set() returned %d", ret);
        return ret;
    }

    /* Get the device ID. */
    hwinfo_get_device_id((void *)&device_id, sizeof(device_id));

    /* get the capability of the api */
    dect_mac_phy_handler_capability_get();

    /* initialize the dect phy modem */
    dect_mac_phy_handler_init();

    return 0; // Success
}

int dect_mac_phy_handler_stop_modem()
{

    int ret;

    /* deinitialize the dect phy modem */
    dect_mac_phy_handler_deinit();

    /* deinitialize the modem lib */
    ret = nrf_modem_lib_shutdown();
    if (ret)
    {
        LOG_ERR("nrf_modem_lib_shutdown() returned %d", ret);
        return ret;
    }

    return 0; // Success
}

void dect_mac_phy_handler_capability_get()
{
    /* indicating current state */
    current_state = GETTING_CAPABILITY;

    /* get the capability of the dect phy modem */
    int ret = nrf_modem_dect_phy_capability_get();
    if (ret)
    {
        LOG_ERR("nrf_modem_dect_phy_capability_get() returned %d", ret);
    }
}

void dect_mac_phy_handler_init()
{
    /* indicating current state */
    current_state = INITIALIZING;

    /* create init parameters */
    const struct nrf_modem_dect_phy_init_params params = {
        .harq_rx_expiry_time_us = CONFIG_HARQ_RX_EXPIRY_TIME_US,
        .harq_rx_process_count = capabilities.harq_process_count_max,
    };

    /* initialize the dect phy modem */
    int ret = nrf_modem_dect_phy_init(&params);
    if (ret)
    {
        LOG_ERR("nrf_modem_dect_phy_init() returned %d", ret);
    }
}

void dect_mac_phy_handler_deinit()
{
    /* indicating current state */
    current_state = DEINITIALIZING;

    /* deinitialize the dect phy modem */
    int ret = nrf_modem_dect_phy_deinit();
    if (ret)
    {
        LOG_ERR("nrf_modem_dect_phy_deinit() returned %d", ret);
    }
}

void dect_mac_phy_handler_rx(struct dect_mac_phy_handler_rx_params params)
{

    /* create true params */
    DECT_MAC_PHY_HANDLER_TRUE_RX_PARAM_CREATE(true_params, params);

    /* indicating current state */
    current_state = RECEIVING;

    /* start the reception */
    int ret = nrf_modem_dect_phy_rx(&true_params);
    if (ret)
    {
        LOG_ERR("nrf_modem_dect_phy_rx() returned %d", ret);
    }
}

void dect_mac_phy_handler_tx(struct dect_mac_phy_handler_tx_params params)
{

    /* create true params */
    DECT_MAC_PHY_HANDLER_TRUE_TX_PARAM_CREATE(true_params, params);

    /* indicating current state */
    current_state = TRANSMITTING;

    /* start the transmission */
    int ret = nrf_modem_dect_phy_tx(&true_params);
    if (ret)
    {
        LOG_ERR("nrf_modem_dect_phy_tx() returned %d", ret);
    }
}

void dect_mac_phy_handler_tx_harq(struct dect_mac_phy_handler_tx_harq_params params)
{

    /* convert to normal tx params */
    struct dect_mac_phy_handler_tx_params tx_params = {
        .handle = params.handle,
        .tx_usage = NO_HARQ,
        .lbt_enable = params.lbt_enable,
        .data = params.data,
        .data_size = params.data_size,
        .receiver_id = params.receiver_id,
        .feedback = {
            .format = FEEDBACK_FORMAT_1,
            .info = {
                .format_1 = {
                    .channel_quality_indicator = 2, // TODO PUT TRUE VALUE
                    .transmission_feedback = 0,
                    .harq_process_number = 0, // TODO PUT TRUE VALUE
                    .buffer_status = 0,       // TODO PUT TRUE VALUE
                },
            },
        },
        .harq = {
            .redundancy_version = params.harq.redundancy_version,
            .new_data_indication = params.harq.new_data_indication,
            .harq_process_nr = params.harq.harq_process_nr,
            .buffer_size = params.harq.buffer_size,
        },
        .start_time = params.start_time,
    };

    /* create true params */
    DECT_MAC_PHY_HANDLER_TRUE_TX_PARAM_CREATE(true_params, tx_params);


    /* setting the handle back to the good handle */
    true_params.handle = ((TX_HARQ << 28) | (params.handle & 0x0fffffff));

    /* indicating current state */
    // current_state = GETTING_CAPABILITY
    // TODO: see what to do here ^

    /* start the transmission */
    int ret = nrf_modem_dect_phy_tx_harq(&true_params);
    if (ret)
    {
        LOG_ERR("nrf_modem_dect_phy_tx_harq() returned %d", ret);
    }
}

void dect_mac_phy_handler_tx_rx(struct dect_mac_phy_handler_tx_rx_params params)
{

    /* convert to normal tx params */
    struct dect_mac_phy_handler_tx_params tx_params = {
        .handle = params.handle,
        .tx_usage = params.tx_usage,
        .lbt_enable = params.lbt_enable,
        .data = params.data,
        .data_size = params.data_size,
        .receiver_id = params.receiver_id,
        .feedback = params.feedback,
        .harq = params.harq,
        .start_time = params.start_time,
    };

    /* convert to normal rx params */
    struct dect_mac_phy_handler_rx_params rx_params = {
        .handle = params.handle,
        .rx_mode = params.rx_mode,
        .rx_period_ms = params.rx_period_ms,
        .receiver_identity = params.receiver_id,
        .start_time = 0,
    };

    /* create true params */
    DECT_MAC_PHY_HANDLER_TRUE_TX_PARAM_CREATE(true_tx_params, tx_params);
    DECT_MAC_PHY_HANDLER_TRUE_RX_PARAM_CREATE(true_rx_params, rx_params);

    /* setting the handle back to the good handle */
    true_tx_params.handle = ((TX_RX << 28) | (params.handle & 0x0fffffff));
    true_rx_params.handle = ((TX_RX << 28) | (params.handle & 0x0fffffff));

    struct nrf_modem_dect_phy_tx_rx_params true_params = {
        .tx = true_tx_params,
        .rx = true_rx_params,
    };

    /* indicating current state */
    current_state = TRANSMITTING;

    /* start the transmission */
    int ret = nrf_modem_dect_phy_tx_rx(&true_params);
    if (ret)
    {
        LOG_ERR("nrf_modem_dect_phy_tx_rx() returned %d", ret);
    }
}

void dect_mac_phy_handler_rssi(struct dect_mac_phy_handler_rssi_params params)
{

    /* create true params */
    struct nrf_modem_dect_phy_rssi_params true_params = {
        .start_time = params.start_time,
        .handle = params.handle,
        .carrier = CONFIG_CARRIER,
        .duration = params.duration,
        .reporting_interval = params.reporting_interval,
    };

    /* indicating current state */
    current_state = MEASURING_RSSI;

    /* start the rssi */
    int ret = nrf_modem_dect_phy_rssi(&true_params);
    if (ret)
    {
        LOG_ERR("nrf_modem_dect_phy_rssi() returned %d", ret);
    }
}

void dect_mac_phy_handler_rx_stop(struct dect_mac_phy_handler_rx_stop_params params)
{

    /* indicating current state */
    current_state = STOPPING_RECEPTION;

    /* start the rx stop */
    int ret = nrf_modem_dect_phy_rx_stop(params.handle);
    if (ret)
    {
        LOG_ERR("nrf_modem_dect_phy_rx_stop() returned %d", ret);
    }
}

void dect_mac_phy_handler_link_config()
{
    // TODO: implement this function
    LOG_ERR("dect_mac_phy_handler_link_config() not implemented");
}

void dect_mac_phy_handler_time_get()
{
    /* indicating current state */
    current_state = GETTING_TIME;

    /* start the time get */
    int ret = nrf_modem_dect_phy_time_get();
    if (ret)
    {
        LOG_ERR("nrf_modem_dect_phy_time_get() returned %d", ret);
    }
}

void dect_mac_phy_handler_tx_config(struct dect_mac_phy_handler_tx_params *input_params, struct nrf_modem_dect_phy_tx_params *output_params)
{

    /* set the time */
    output_params->start_time = input_params->start_time;

    /* set the handle */
    output_params->handle = ((TX << 28) | (input_params->handle & 0x0fffffff));

    /* set the network id */
    output_params->network_id = CONFIG_NETWORK_ID;

    /* setup lbt */
    output_params->lbt_rssi_threshold_max = CONFIG_RSSI_TARGET;
    output_params->lbt_period = input_params->lbt_enable ? NRF_MODEM_DECT_LBT_PERIOD_MAX : 0;

    /* set the carrier */
    output_params->carrier = CONFIG_CARRIER;

    uint32_t tx_power = 11; // TODO: create a function to calculate these
    uint32_t df_mcs = 2;

    uint32_t packet_length_type = 0; // TODO: create a function to calculate these
    uint32_t packet_length = 2;

    /* handle the case of sending a message with no data */
    if(input_params->data_size == 0)
    {
        packet_length_type = 0;
        packet_length = 0;
        df_mcs = 0;
    }

    if (input_params->tx_usage == BEACON)
    {
        output_params->phy_type = HEADER_TYPE_1;
        struct phy_ctrl_field_common_type1 *header = (struct phy_ctrl_field_common_type1 *)output_params->phy_header;

        header->header_format = HEADER_FORMAT_000;
        header->packet_length_type = packet_length_type;
        header->packet_length = packet_length;
        header->short_network_id = (CONFIG_NETWORK_ID & 0xff);
        header->transmitter_id_hi = (device_id >> 8);
        header->transmitter_id_lo = (device_id & 0xff);
        header->transmit_power = tx_power; // TODO: set to the worst connection in the registered nodes
        header->reserved = 0;
        header->df_mcs = df_mcs; // TODO: set to the worst connection in the registered nodes
    }
    else
    {
        output_params->phy_type = HEADER_TYPE_2;
        struct phy_ctrl_field_common_type2 *header = (struct phy_ctrl_field_common_type2 *)output_params->phy_header;

        header->packet_length_type = packet_length_type;
        header->packet_length = packet_length;
        header->short_network_id = (CONFIG_NETWORK_ID & 0xff);
        header->transmitter_id_hi = (device_id >> 8);
        header->transmitter_id_lo = (device_id & 0xff);
        header->transmit_power = tx_power;
        header->df_mcs = df_mcs;
        header->receiver_id_hi = (input_params->receiver_id >> 8);
        header->receiver_id_lo = (input_params->receiver_id & 0xff);
        header->spatial_streams = 0; // device does not support spatial streams
        header->feedback_format = input_params->feedback.format;
        header->feedback_info_hi = input_params->feedback.info.byte.hi;
        header->feedback_info_lo = input_params->feedback.info.byte.lo;

        if (input_params->tx_usage == HARQ)
        {
            header->header_format = HEADER_FORMAT_000;
            header->df_red_version = input_params->harq.redundancy_version;
            header->df_ind = input_params->harq.new_data_indication;
            header->df_harq_process_nr = input_params->harq.harq_process_nr;
        }
        else
        {
            header->header_format = HEADER_FORMAT_001;
            header->df_red_version = 0;
            header->df_ind = 0;
            header->df_harq_process_nr = 0;
        }
    }

    output_params->bs_cqi = NRF_MODEM_DECT_PHY_BS_CQI_NOT_USED; // TODO: implement cqi and buffer
    output_params->data = input_params->data;
    output_params->data_size = input_params->data_size;
}

void dect_mac_phy_handler_rx_config(struct dect_mac_phy_handler_rx_params *input_params, struct nrf_modem_dect_phy_rx_params *output_params)
{

    /* create true params */
    output_params->start_time = input_params->start_time;
    output_params->handle = ((RX << 28) | (input_params->handle & 0x0fffffff));
    output_params->network_id = CONFIG_NETWORK_ID;
    output_params->mode = input_params->rx_mode;
    output_params->rssi_interval = NRF_MODEM_DECT_PHY_RSSI_INTERVAL_OFF;
    output_params->link_id = NRF_MODEM_DECT_PHY_LINK_UNSPECIFIED;
    output_params->rssi_level = CONFIG_RSSI_TARGET;
    output_params->carrier = CONFIG_CARRIER;
    output_params->duration = input_params->rx_period_ms * 69120;
    output_params->filter.short_network_id = CONFIG_NETWORK_ID;
    output_params->filter.is_short_network_id_used = true;
    output_params->filter.receiver_identity = input_params->receiver_identity;
}