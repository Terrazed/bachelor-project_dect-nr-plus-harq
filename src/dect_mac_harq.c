#include "dect_mac_harq.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(harq, 3);

K_THREAD_STACK_DEFINE(dect_mac_harq_work_queue_stack, DECT_MAC_HARQ_WORK_QUEUE_STACK_SIZE);

bool harq_process_occupied[CONFIG_HARQ_PROCESS_COUNT] = {0};
struct dect_mac_harq_process harq_processes[CONFIG_HARQ_PROCESS_COUNT] = {0};
bool dect_mac_harq_initialized = false;
struct k_work_q dect_mac_harq_work_queue;

int dect_mac_harq_request(struct phy_ctrl_field_common_type2 *header, uint64_t start_time)
{
    /* get packet length */
    uint8_t packet_length = header->packet_length;
    uint16_t packet_length_bytes = dect_mac_utils_get_bytes_from_packet_length(packet_length, header->df_mcs);

    /* remove buffer size */
    dect_mac_harq_remove_buffer_space(header->df_harq_process_nr, packet_length_bytes);
    
    /* set the feedback */
    struct feedback feedback;

    if(packet_length_bytes > capabilities.harq_soft_buf_size/CONFIG_HARQ_PROCESS_COUNT){
        LOG_ERR("Packet length bigger than buffer size");
        feedback.format = FEEDBACK_FORMAT_6;
        feedback.info.format_6.harq_process_number = header->df_harq_process_nr;
        feedback.info.format_6.channel_quality_indicator = dect_mac_node_get_cqi(header->transmitter_id_hi << 8 | header->transmitter_id_lo);
        feedback.info.format_6.buffer_status = dect_mac_harq_get_buffer_status(header->df_harq_process_nr);
        feedback.info.format_6.reserved = 0;
    }
    else
    {
        feedback.format = FEEDBACK_FORMAT_1;
        feedback.info.format_1.channel_quality_indicator = dect_mac_node_get_cqi(header->transmitter_id_hi << 8 | header->transmitter_id_lo);
        feedback.info.format_1.transmission_feedback = NACK; // this will be changed to ACK by the phy layer if the PDC crc is correct
        feedback.info.format_1.harq_process_number = header->df_harq_process_nr;
        feedback.info.format_1.buffer_status = dect_mac_harq_get_buffer_status(header->df_harq_process_nr);
    }

    /* config the operation */
    struct dect_mac_phy_handler_tx_harq_params params = {
        .handle = (HANDLE_HARQ + header->df_harq_process_nr) | (1<<27),
        .lbt_enable = false,
        .data = NULL,
        .data_size = 0,
        .receiver_id = header->transmitter_id_hi << 8 | header->transmitter_id_lo,
        .feedback = feedback,
        .start_time = start_time + (10 * 10000/24 * NRF_MODEM_DECT_MODEM_TIME_TICK_RATE_KHZ / 1000), // TODO: check this
    };

    /* send the acknoledgement */
    dect_mac_phy_handler_tx_harq(params);
    int ret = dect_phy_queue_put(PLACEHOLDER, NO_PARAMS, PRIORITY_CRITICAL);
    return ret;
}

void dect_mac_harq_response(struct phy_ctrl_field_common_type2 *header)
{
    /* get the feedback infos */
    union feedback_info feedback;
    enum nrf_mac_feedback_format format = header->feedback_format;
    feedback.byte.hi = header->feedback_info_hi;
    feedback.byte.lo = header->feedback_info_lo;

    /* get the harq process */
    struct dect_mac_harq_process *harq_process = &harq_processes[feedback.format_1.harq_process_number];

    if(format == FEEDBACK_FORMAT_1)
    {
        /* check if the transmission was successful */
        if(feedback.format_1.transmission_feedback == ACK){
            LOG_INF("ACK received for harq process %d", feedback.format_1.harq_process_number);
            k_work_cancel_delayable(&harq_process->retransmission_work); // stop the scheduled work 
            dect_mac_harq_give_process(harq_process);
        } else {
            LOG_WRN("NACK received for harq process %d", feedback.format_1.harq_process_number);
            dect_mac_harq_increment_redundancy_version(harq_process);
            int ret = k_work_reschedule_for_queue(&dect_mac_harq_work_queue, &harq_process->retransmission_work, K_NO_WAIT); // schedule the retransmission work
        }
    }
    else if(format == FEEDBACK_FORMAT_6)
    {
        LOG_WRN("NACK received for harq process %d and buffer not big enough", feedback.format_1.harq_process_number);
        harq_process->redundancy_version = 0; // the other device can't store the data, so we reset the redundancy version to send the full data
        int ret = k_work_reschedule_for_queue(&dect_mac_harq_work_queue, &harq_process->retransmission_work, K_NO_WAIT); // schedule the retransmission work
    }

    
}

int dect_mac_harq_transmit(struct dect_mac_harq_transmit_params params)
{
    /* get a harq process */
    struct dect_mac_harq_process *harq_process = dect_mac_harq_take_process();
    if(harq_process == NULL){
        return NO_FREE_HARQ;
    }

    /* copy the data to the harq process */
    uint8_t *data = k_malloc(params.data_len);
    if(data == NULL){
        dect_mac_harq_give_process(harq_process);
        LOG_ERR("No memory for harq data");
        return NO_MEM_HEAP;
    }
    memcpy(data, params.data, params.data_len);

    /* initialize the harq process */
    harq_process->data = data;
    harq_process->data_len = params.data_len;
    harq_process->receiver_id = params.receiver_id;
    harq_process->transmission_count = 1; // first transmission

    struct dect_mac_phy_handler_tx_rx_params tx_rx_params = {
        .handle = HANDLE_HARQ + harq_process->process_number,
        .tx_usage = HARQ,
        .lbt_enable = false,
        .data = harq_process->data,
        .data_size = harq_process->data_len,
        .receiver_id = harq_process->receiver_id,
        .feedback = DECT_MAC_PHY_HANDLER_NO_FEEDBACK,
        .harq = {
            .redundancy_version = harq_process->redundancy_version,
            .new_data_indication = harq_process->new_data_indication,
            .harq_process_nr = harq_process->process_number,
        },
        .rx_mode = NRF_MODEM_DECT_PHY_RX_MODE_SINGLE_SHOT,
        .rx_period_ms = CONFIG_HARQ_RX_WAITING_TIME_MS,
        .start_time = params.start_time,
    };
    
    LOG_INF("Transmitting... (harq process: %d, count: %d, rv: %d)", harq_process->process_number, harq_process->transmission_count, harq_process->redundancy_version);
    return dect_phy_queue_put(TX_RX, (union dect_mac_phy_handler_params*)&tx_rx_params, PRIORITY_HIGH);
    
}

void dect_mac_harq_retransmission_work_handler(struct k_work *work)
{
    /* find the calling harq process */
    struct dect_mac_harq_process *harq_process = CONTAINER_OF((struct k_work_delayable*)work, struct dect_mac_harq_process, retransmission_work);

    /* check if the transmission count is less than the maximum */
    if(harq_process->transmission_count < CONFIG_HARQ_MAX_TRANSMISSIONS)
    {
        dect_mac_harq_retransmit(harq_process);
    }
    else
    {
        LOG_WRN("Max retransmissions reached for harq process %d", harq_process->process_number);
        dect_mac_harq_give_process(harq_process);
    }
        
}

int dect_mac_harq_retransmit(struct dect_mac_harq_process *harq_process)
{
    harq_process->transmission_count++; // incremenrt transmission
    
    dect_mac_node_add_power(harq_process->receiver_id, harq_process->transmission_count); // add power to the receiver
    dect_mac_node_reduce_mcs(harq_process->receiver_id, 1); // reduce the MCS of the receiver

    LOG_INF("Transmitting... (harq process: %d, count: %d, rv: %d)", harq_process->process_number, harq_process->transmission_count, harq_process->redundancy_version);

    /* config tx-rx operation */
    struct dect_mac_phy_handler_tx_rx_params tx_rx_params = {
        .handle = HANDLE_HARQ + harq_process->process_number,
        .tx_usage = HARQ,
        .lbt_enable = false,
        .data = harq_process->data,
        .data_size = harq_process->data_len,
        .receiver_id = harq_process->receiver_id,
        .feedback = DECT_MAC_PHY_HANDLER_NO_FEEDBACK,
        .harq = {
            .redundancy_version = harq_process->redundancy_version,
            .new_data_indication = harq_process->new_data_indication,
            .harq_process_nr = harq_process->process_number,
        },
        .rx_mode = NRF_MODEM_DECT_PHY_RX_MODE_SINGLE_SHOT,
        .rx_period_ms = CONFIG_HARQ_RX_WAITING_TIME_MS,
        .start_time = 0,
    };

    return dect_phy_queue_put(TX_RX, (union dect_mac_phy_handler_params*)&tx_rx_params, PRIORITY_MEDIUM);
}

void dect_mac_harq_increment_redundancy_version(struct dect_mac_harq_process *harq_process)
{
    /* increment the redundancy version */
    /* increment pattern: 0 -> 2 -> 3 -> 1 -> 0 -> ... */

    switch (harq_process->redundancy_version){
        case 0:
            harq_process->redundancy_version = 2;
            break;
        case 1:
            harq_process->redundancy_version = 0;
            break;
        case 2:
            harq_process->redundancy_version = 3;
            break;
        case 3:
            harq_process->redundancy_version = 1;
            break;
        default:
            LOG_ERR("Invalid redundancy version");
            break;
    }
}

uint8_t dect_mac_harq_get_buffer_status(uint32_t process_number)
{
    if(process_number >= CONFIG_HARQ_PROCESS_COUNT)
    {
        LOG_ERR("Invalid process number");
        return 0;
    }
    else
    {
        uint8_t buffer_status;
        uint32_t process_buffer_size = harq_processes[process_number].buffer_size;

        if(process_buffer_size == 0)
        {
            buffer_status = 0;
        }
        else if(process_buffer_size <= 16)
        {
            buffer_status = 0x1;
        }
        else if(process_buffer_size <= 32)
        {
            buffer_status = 0x2;
        }
        else if(process_buffer_size <= 64)
        {
            buffer_status = 0x3;
        }
        else if(process_buffer_size <= 128)
        {
            buffer_status = 0x4;
        }
        else if(process_buffer_size <= 256)
        {
            buffer_status = 0x5;
        }
        else if(process_buffer_size <= 512)
        {
            buffer_status = 0x6;
        }
        else if(process_buffer_size <= 1024)
        {
            buffer_status = 0x7;
        }
        else if(process_buffer_size <= 2048)
        {
            buffer_status = 0x8;
        }
        else if(process_buffer_size <= 4096)
        {
            buffer_status = 0x9;
        }
        else if(process_buffer_size <= 8192)
        {
            buffer_status = 0xa;
        }
        else if(process_buffer_size <= 16384)
        {
            buffer_status = 0xb;
        }
        else if(process_buffer_size <= 32768)
        {
            buffer_status = 0xc;
        }
        else if(process_buffer_size <= 65536)
        {
            buffer_status = 0xd;
        }
        else if(process_buffer_size <= 131072)
        {
            buffer_status = 0xe;
        }
        else
        {
            buffer_status = 0xf;
        }

        return buffer_status;
    }
}

int dect_mac_harq_remove_buffer_space(uint32_t process_number, uint8_t byte_count){
    if(process_number >= CONFIG_HARQ_PROCESS_COUNT)
    {
        LOG_ERR("Invalid process number");
        return INVALID_PROCESS_NUMBER;
    }
    if(harq_processes[process_number].buffer_size < byte_count)
    {
        LOG_ERR("Not enough space in the buffer");
        return BUFFER_TOO_SMALL;
    }
    else
    {
        harq_processes[process_number].buffer_size -= byte_count;
        return OK;
    }
}

struct dect_mac_harq_process * dect_mac_harq_take_process()
{
    /* initialize if not already initialized */
    if(dect_mac_harq_initialized == false)
    {
        dect_mac_harq_initialize();
    }

    /* loop through all processes */
    for(int i = 0; i < CONFIG_HARQ_PROCESS_COUNT; i++){
        if(!harq_process_occupied[i]){ // find a free process
            harq_process_occupied[i] = true; // set the process as occupied
            harq_processes[i].new_data_indication = !harq_processes[i].new_data_indication; // toggle the new data indication
            return &harq_processes[i]; // return the process
        }
    }

    LOG_ERR("No free harq process found");
    return NULL;

}

void dect_mac_harq_give_process(struct dect_mac_harq_process *harq_process)
{
    /* initialize if not already initialized */
    if(dect_mac_harq_initialized == false)
    {
        dect_mac_harq_initialize();
    }

    LOG_DBG("Giving back harq process %d", harq_process->process_number);

    dect_mac_harq_init_process(harq_process, harq_process->process_number); // initialize the process
    harq_process_occupied[harq_process->process_number] = false; // set the process as free
    k_work_cancel_delayable(&harq_process->retransmission_work); // cancel the retransmission work
}

void dect_mac_harq_init_process(struct dect_mac_harq_process *harq_process, uint32_t process_number)
{   
    LOG_DBG("Initializing harq process %d", process_number);

    harq_process->process_number = process_number;
    k_free(harq_process->data); // free the data (if any)
    harq_process->data = NULL;
    harq_process->data_len = 0;
    harq_process->receiver_id = 0;
    harq_process->transmission_count = 0;
    harq_process->redundancy_version = 0;
    harq_process->buffer_size = capabilities.harq_soft_buf_size/CONFIG_HARQ_PROCESS_COUNT;
}

void dect_mac_harq_initialize()
{
    /* initialize only once */
    if(dect_mac_harq_initialized == false)
    {

        k_work_queue_init(&dect_mac_harq_work_queue);
        k_work_queue_start(&dect_mac_harq_work_queue, dect_mac_harq_work_queue_stack, K_THREAD_STACK_SIZEOF(dect_mac_harq_work_queue_stack), 8, NULL);

        /* loop through all processes */
        for (int i = 0; i < CONFIG_HARQ_PROCESS_COUNT; i++)
        {
            dect_mac_harq_init_process(&harq_processes[i], i); // initialize the process
            harq_process_occupied[i] = false; // set the process as free
            harq_processes[i].new_data_indication = true; // set the new data indication to true
            k_work_init_delayable(&harq_processes[i].retransmission_work, dect_mac_harq_retransmission_work_handler); // initialize the retransmission work
        }
        dect_mac_harq_initialized = true; // set the flag to true
        LOG_DBG("Harq initialized");
    }
}
