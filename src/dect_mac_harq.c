#include "dect_mac_harq.h"

bool harq_process_occupied[HARQ_PROCESS_MAX] = {0};
struct dect_mac_harq_process harq_processes[HARQ_PROCESS_MAX] = {0};
bool dect_mac_harq_initialized = false;

int dect_mac_harq_request(struct phy_ctrl_field_common_type2 *header, uint64_t start_time)
{
    //TODO: Implement function
}

void dect_mac_harq_response(struct phy_ctrl_field_common_type2 *header)
{
    //TODO: Implement function
}

int dect_mac_harq_transmit(struct dect_mac_harq_transmit_params params)
{
    //TODO: Implement function
}

void dect_mac_harq_retransmission_work_handler(struct k_work *work)
{
    //TODO: Implement function
}

int dect_mac_harq_retransmit(struct dect_mac_harq_process *harq_process)
{
    //TODO: Implement function
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

struct dect_mac_harq_process * dect_mac_harq_take_process()
{
    /* initialize if not already initialized */
    if(dect_mac_harq_initialized == false)
    {
        dect_mac_harq_initialize();
    }

    /* loop through all processes */
    for(int i = 0; i < HARQ_PROCESS_MAX; i++){
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
}

void dect_mac_harq_initialize()
{
    /* initialize only once */
    if(dect_mac_harq_initialized == false)
    {
        /* loop through all processes */
        for (int i = 0; i < HARQ_PROCESS_MAX; i++)
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
