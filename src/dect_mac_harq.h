#ifndef DECT_MAC_HARQ_H
#define DECT_MAC_HARQ_H

#include <zephyr/kernel.h>

#include "dect_mac_phy_handler_types.h"

/* struct that represent a harq process */
struct dect_mac_harq_process {
    uint32_t process_number;
    void* data;
    size_t data_len;
    uint32_t receiver_id;
    struct k_timer timer;
    uint8_t transmission_count;
    uint8_t redundancy_version;
    uint8_t new_data_indication;
};


/* struct that contains all the needed parameters to transmit a harq message */
struct dect_mac_harq_transmit_params {
    void *data;
    size_t data_len;
    uint64_t start_time;
    uint32_t receiver_id;
};


/* function to handle the harq request (sending an acknowledgement)*/
int dect_mac_harq_request(struct phy_ctrl_field_common_type2 *header, uint64_t start_time);

/* function to handle the harq response (receiving the acknowledgment)*/
void dect_mac_harq_response(struct phy_ctrl_field_common_type2 *header);

/* function to transmit a harq message */
int dect_mac_harq_transmit(struct dect_mac_harq_transmit_params params);

/* function to handle the harq retransmission */
int dect_mac_harq_retransmit(struct dect_mac_harq_process *harq_process);

/* function to increment the redundancy version of a harq process */
void dect_mac_harq_increment_redundancy_version(struct dect_mac_harq_process *harq_process);






/* function to get an avaiable harq process */
struct dect_mac_harq_process * dect_mac_harq_take_process();

/* function to give back a harq process */
void dect_mac_harq_give_process(struct dect_mac_harq_process *harq_process);

/* function to init a harq process */
void dect_mac_harq_init_process(struct dect_mac_harq_process *harq_process, uint32_t process_number);




#endif // DECT_MAC_HARQ_H