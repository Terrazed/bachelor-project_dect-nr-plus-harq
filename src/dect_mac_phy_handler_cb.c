#include "dect_mac_phy_handler_cb.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(handler_cb, 3);

/* initialize globals variables */
struct dect_capabilities capabilities = {0};

void dect_mac_phy_init_cb(const uint64_t *time, int16_t temp, enum nrf_modem_dect_phy_err err, const struct nrf_modem_dect_phy_modem_cfg *cfg)
{
    LOG_DBG("init callback - time: %llu, temp: %d, err: %d", *time, temp, err);

    /* saving the time */
    dect_mac_utils_modem_time_save(time);

    if (err)
    {
        LOG_ERR("init callback - error: %d", err);
        return;
    }

    /* release the semaphore */
    k_sem_give(&phy_layer_sem);
}

void dect_mac_phy_op_complete_cb(const uint64_t *time, int16_t temperature, enum nrf_modem_dect_phy_err err, uint32_t handle)
{
    LOG_DBG("op complete callback - time: %llu, temp: %d, err: %d, handle: %x", *time, temperature, err, handle);

    /* saving the time */
    dect_mac_utils_modem_time_save(time);

    if (err)
    {
        if (err == NRF_MODEM_DECT_PHY_ERR_INVALID_START_TIME)
        {
            LOG_INF("operation with handle: %x couldn't be started at the requested time, retrying...", handle);
            dect_mac_phy_handler_queue_operation_failed_retry();
        }
        else if (err != NRF_MODEM_DECT_PHY_ERR_COMBINED_OP_FAILED)
        {
            LOG_ERR("op complete callback - time: %llu, temp: %d, err: %d, handle: %x", *time, temperature, err, handle);
        }

        return;
    }

    /* dont release the semaphore when switching from tx to rx in a combined operation */
    if ((current_state == TRANSMITTING) && ((handle >> 28) == TX_RX))
    {
        // switch from tx to rx
        current_state = RECEIVING;

        if ((handle & 0x07FFFFF0) == HANDLE_HARQ)
        {
            // get harq process
            uint8_t harq_porcess_number = handle & 0x0000000F;
            struct k_work_delayable *work = &harq_processes[harq_porcess_number].retransmission_work;

            // schedule retransmission work
            k_work_schedule_for_queue(&dect_mac_harq_work_queue, work, K_MSEC(CONFIG_HARQ_RX_WAITING_TIME_MS));
        }
    }
    else
    {
        if (((handle & (1 << 27)) >> 27) == 1) // if the operation comes from the queue
        {
            /* release the semaphore */
            k_sem_give(&phy_layer_sem);
        }
    }
}

void dect_mac_phy_rssi_cb(const uint64_t *time, const struct nrf_modem_dect_phy_rssi_meas *meas)
{
    LOG_DBG("rssi callback - time: %llu", *time);

    /* saving the time */
    dect_mac_utils_modem_time_save(time);

    /* release the semaphore */
    k_sem_give(&phy_layer_sem);
}

void dect_mac_phy_rx_stop_cb(const uint64_t *time, enum nrf_modem_dect_phy_err err, uint32_t handle)
{
    LOG_DBG("rx stop callback - time: %llu, err: %d, handle: %x", *time, err, handle);

    /* saving the time */
    dect_mac_utils_modem_time_save(time);

    if (err)
    {
        LOG_ERR("rx stop callback - error: %d", err);
        return;
    }
}

void dect_mac_phy_pcc_cb(const uint64_t *time, const struct nrf_modem_dect_phy_rx_pcc_status *status, const union nrf_modem_dect_phy_hdr *hdr)
{
    LOG_DBG("pcc callback - time: %llu, stf_start_time: %llu", *time, status->stf_start_time);

    /* saving the time */
    dect_mac_utils_modem_time_save(time);

    /* optimize node */
    uint32_t transmitter_id = ((struct phy_ctrl_field_common_type1 *)hdr)->transmitter_id_hi << 8 | ((struct phy_ctrl_field_common_type1 *)hdr)->transmitter_id_lo;
    uint32_t rssi = status->rssi_2 / 2; // rssi is in 0.5 dBm
    uint32_t snr = status->snr / 4;     // snr is in 0.25 dB
    uint32_t transmit_power = ((struct phy_ctrl_field_common_type1 *)hdr)->transmit_power;
    uint32_t transmit_mcs = ((struct phy_ctrl_field_common_type1 *)hdr)->df_mcs;
    dect_mac_node_optimize(transmitter_id, rssi, snr, transmit_power, transmit_mcs);

    if (status->phy_type == HEADER_TYPE_2)
    {
        LOG_DBG("Received PCC with header type 2");

        struct phy_ctrl_field_common_type2 *header = (struct phy_ctrl_field_common_type2 *)hdr;

        LOG_DBG("header format : %d", header->header_format);

        if (header->header_format == HEADER_FORMAT_000) // requesting HARQ response
        {
            if (header->short_network_id == (CONFIG_NETWORK_ID & 0xff) // correct network ID
                && ((header->receiver_id_hi == (device_id >> 8) && header->receiver_id_lo == (device_id & 0xff)) // correct receiver ID (this device)
                    || (header->receiver_id_hi == 0xff && header->receiver_id_lo == 0xff))) // correct receiver ID (broadcast) TODO: does this makes sense ?
            {
                LOG_DBG("reveiving HARQ request, rv: %d", header->df_red_version);

                int err = dect_mac_harq_request(header, status->stf_start_time);
                if (err)
                {
                    LOG_ERR("Transmit HARQ failed");
                }
            }
            else
            {
                LOG_WRN("Received HARQ request with wrong receiver ID");
            }
        }
        else if (header->header_format == HEADER_FORMAT_001)
        {
            if (header->short_network_id == (CONFIG_NETWORK_ID & 0xff) // correct network ID
                && ((header->receiver_id_hi == (device_id >> 8) && header->receiver_id_lo == (device_id & 0xff)) // correct transmitter ID (this device)
                    || (header->receiver_id_hi == 0xff && header->receiver_id_hi == 0xff))) // correct transmitter ID (broadcast)
            {
                LOG_DBG("feedback format : %d", header->feedback_format);
                if ((header->feedback_format == FEEDBACK_FORMAT_1) || (header->feedback_format == FEEDBACK_FORMAT_6)) // receiving HARQ response
                {
                    LOG_DBG("reveiving HARQ response");
                    dect_mac_harq_response(header);
                }
            }
            else
            {
                LOG_WRN("Received message with wrong receiver ID, received: %d, expected: %d", header->receiver_id_hi << 8 | header->receiver_id_lo, device_id);
            }
        }
        else
        {
            LOG_ERR("Received PCC with unknown header format");
        }
    }
    else if (status->phy_type == HEADER_TYPE_1)
    {
        LOG_DBG("Received PCC with header type 1");
    }
    else
    {
        LOG_ERR("Received PCC with unknown header type");
    }
}

void dect_mac_phy_pcc_crc_err_cb(const uint64_t *time, const struct nrf_modem_dect_phy_rx_pcc_crc_failure *crc_failure)
{
    LOG_ERR("pcc crc error callback - time: %llu", *time);

    /* saving the time */
    dect_mac_utils_modem_time_save(time);
}

void dect_mac_phy_pdc_cb(const struct nrf_modem_dect_phy_pdc_event *evt)
{

    /* saving the data locally to ensure data validity */
    uint8_t data_local[evt->len];
    memcpy(data_local, evt->data, evt->len);

    LOG_DBG("pdc callback - time: %llu", evt->time);

    /* saving the time */
    dect_mac_utils_modem_time_save(&evt->time);

    if (evt->len > 0)
    {
        LOG_INF("Received data: %.*s, length: %d", evt->len, data_local, evt->len);
    }
    else
    {
        LOG_INF("Received data: NULL");
    }
}

void dect_mac_phy_pdc_crc_err_cb(const struct nrf_modem_dect_phy_pdc_crc_failure_event *evt)
{
    LOG_ERR("pdc crc error callback - time: %llu", evt->time);

    /* saving the time */
    dect_mac_utils_modem_time_save(&evt->time);
}

void dect_mac_phy_radio_config_cb(const struct nrf_modem_dect_phy_radio_config_event *evt)
{
    LOG_DBG("radio config callback - time: %llu, err: %d", evt->time, evt->err);

    /* saving the time */
    dect_mac_utils_modem_time_save(&evt->time);

    if (evt->err)
    {
        LOG_ERR("radio config callback - error: %d", evt->err);
        return;
    }

    /* release the semaphore */
    k_sem_give(&phy_layer_sem);
}

void dect_mac_phy_link_config_cb(const struct nrf_modem_dect_phy_link_config_event *evt)
{
    LOG_DBG("link config callback - time: %llu, err: %d", evt->time, evt->err);

    /* saving the time */
    dect_mac_utils_modem_time_save(&evt->time);

    if (evt->err)
    {
        LOG_ERR("link config callback - error: %d", evt->err);
        return;
    }

    /* release the semaphore */
    k_sem_give(&phy_layer_sem);
}

void dect_mac_phy_time_get_cb(const struct nrf_modem_dect_phy_time_get_event *evt)
{
    LOG_DBG("time get callback - time: %llu, err: %d", evt->time, evt->err);

    /* saving the time */
    dect_mac_utils_modem_time_save(&evt->time);

    if (evt->err)
    {
        LOG_ERR("time get callback - error: %d", evt->err);
        return;
    }

    /* release the semaphore */
    k_sem_give(&phy_layer_sem);
}

void dect_mac_phy_capability_get_cb(const struct nrf_modem_dect_phy_capability_get_event *evt)
{
    LOG_DBG("capability get callback - time: %llu, err: %d", evt->time, evt->err);

    /* saving the time */
    dect_mac_utils_modem_time_save(&evt->time);

    if (evt->err)
    {
        LOG_ERR("capability get callback - error: %d", evt->err);
        return;
    }

    /* save the capabilities of the modem */
    capabilities.dect_version = evt->capability->dect_version;
    capabilities.power_class = evt->capability->variant[0].power_class;
    capabilities.rx_spatial_streams = evt->capability->variant[0].rx_spatial_streams;
    capabilities.rx_tx_diversity = evt->capability->variant[0].rx_tx_diversity;
    capabilities.rx_gain = evt->capability->variant[0].rx_gain;
    capabilities.mcs_max = evt->capability->variant[0].mcs_max;
    capabilities.harq_soft_buf_size = evt->capability->variant[0].harq_soft_buf_size;
    capabilities.harq_process_count_max = evt->capability->variant[0].harq_process_count_max;
    capabilities.harq_feedback_delay = evt->capability->variant[0].harq_feedback_delay;
    capabilities.mu = evt->capability->variant[0].mu;
    capabilities.beta = evt->capability->variant[0].beta;

    /* release the semaphore */
    k_sem_give(&phy_layer_sem);
}

void dect_mac_phy_deinit_cb(const struct nrf_modem_dect_phy_deinit_event *evt)
{
    LOG_DBG("deinit callback - time: %llu, err: %d", evt->time, evt->err);

    /* saving the time */
    dect_mac_utils_modem_time_save(&evt->time);

    if (evt->err)
    {
        LOG_ERR("deinit callback - error: %d", err);
        return;
    }

    /* release the semaphore */
    k_sem_give(&phy_layer_sem);
}

struct nrf_modem_dect_phy_callbacks *dect_mac_phy_handler_get_callbacks(void)
{
    static struct nrf_modem_dect_phy_callbacks dect_phy_callbacks;

    dect_phy_callbacks.init = dect_mac_phy_init_cb;
    dect_phy_callbacks.deinit = dect_mac_phy_deinit_cb;
    dect_phy_callbacks.op_complete = dect_mac_phy_op_complete_cb;
    dect_phy_callbacks.cancel = dect_mac_phy_cancel_cb;
    dect_phy_callbacks.pcc = dect_mac_phy_pcc_cb;
    dect_phy_callbacks.pcc_crc_err = dect_mac_phy_pcc_crc_err_cb;
    dect_phy_callbacks.pdc = dect_mac_phy_pdc_cb;
    dect_phy_callbacks.pdc_crc_err = dect_mac_phy_pdc_crc_err_cb;
    dect_phy_callbacks.rssi = dect_mac_phy_rssi_cb;
    dect_phy_callbacks.radio_config = dect_mac_phy_radio_config_cb;
    dect_phy_callbacks.link_config = dect_mac_phy_link_config_cb;
    dect_phy_callbacks.time_get = dect_mac_phy_time_get_cb;
    dect_phy_callbacks.capability_get = dect_mac_phy_capability_get_cb;

    return &dect_phy_callbacks;
}