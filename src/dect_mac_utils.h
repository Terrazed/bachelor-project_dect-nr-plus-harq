#ifndef DECT_MAC_UTILS_H
#define DECT_MAC_UTILS_H

#include <zephyr/kernel.h>
#include <nrf_modem_dect_phy.h>

/* function to get the modem time faster than with the modem call (taken from fmac)*/
uint64_t dect_mac_utils_modem_time_now(void);

/* function that is used to update the modem time (taken from fmac)*/
void dect_mac_utils_modem_time_save(uint64_t const *time);

/* function that compute the appopriate the packet length using the data size and the mcs*/
int dect_mac_utils_get_packet_length(size_t *data_size, uint32_t *mcs, uint32_t *packet_length_type, uint32_t *packet_length);



#endif // DECT_MAC_UTILS_H