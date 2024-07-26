#include "dect_mac_harq.h"

bool harq_process_occupied[HARQ_PROCESS_MAX] = {0};
struct dect_mac_harq_process harq_processes[HARQ_PROCESS_MAX] = {0};

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

void dect_mac_harq_retransmission_work_handler(struct k_work_delayable *work)
{
    //TODO: Implement function
}

int dect_mac_harq_retransmit(struct dect_mac_harq_process *harq_process)
{
    //TODO: Implement function
}

void dect_mac_harq_increment_redundancy_version(struct dect_mac_harq_process *harq_process)
{
    //TODO: Implement function
}


struct dect_mac_harq_process * dect_mac_harq_take_process()
{
    //TODO: Implement function
}

void dect_mac_harq_give_process(struct dect_mac_harq_process *harq_process)
{
    //TODO: Implement function
}

void dect_mac_harq_init_process(struct dect_mac_harq_process *harq_process, uint32_t process_number)
{   
    //TODO: Implement function
}