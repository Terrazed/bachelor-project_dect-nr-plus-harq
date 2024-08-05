#include "dect_mac_utils.h"

#include <zephyr/logging/log.h>
#include <zephyr/drivers/hwinfo.h>
LOG_MODULE_REGISTER(utils, 3);

static uint64_t m_last_modem_time_bb;
static uint64_t m_last_app_time_bb;

/* Current app processor time in modem bb ticks. */
static uint64_t dect_app_ztime_now_in_mdm_ticks(void)
{
	uint64_t time_now_ns = k_ticks_to_ns_floor64(sys_clock_tick_get());

	return time_now_ns / 1000 * NRF_MODEM_DECT_MODEM_TIME_TICK_RATE_KHZ / 1000;
}

/* Current time in modem ticks */
uint64_t dect_mac_utils_modem_time_now(void)
{
	uint64_t time_now;

	time_now = m_last_modem_time_bb + (dect_app_ztime_now_in_mdm_ticks() - m_last_app_time_bb);

	return time_now;
}

/* Save the modem time */
void dect_mac_utils_modem_time_save(uint64_t const *time)
{
	if (*time > m_last_modem_time_bb) {
		m_last_modem_time_bb = *time;
		m_last_app_time_bb = dect_app_ztime_now_in_mdm_ticks();
	}
}

static const int16_t byte_per_mcs_and_length[5][16] = {
    {0,17,33,50,67,83,99,115,133,149,165,181,197,213,233,249},
    {4,37,69,103,137,169,201,233,263,295,327,359,391,423,463,495},
    {7,57,107,157,205,253,295,343,399,447,495,540,596,644,692,-1},
    {11,77,141,209,271,335,399,463,532,596,660,-1,-1,-1,-1,-1,},
    {18,117,217,311,407,503,604,700,-1,-1,-1,-1,-1,-1,-1,-1}
};

int dect_mac_utils_get_packet_length(size_t *data_size, uint32_t *mcs, uint32_t *packet_length_type, uint32_t *packet_length){

    *packet_length_type = 0;

    bool found = false;
    for(*packet_length = 0; *packet_length < 16; (*packet_length)++){
        if(byte_per_mcs_and_length[*mcs][*packet_length] >= (int16_t)*data_size)
        {
            found = true;
            break;
        }
    }
    if(!found){
        LOG_DBG("Data size is too big for the given MCS");
    }
    return found?OK:PACKET_TOO_BIG; // return 0 if found, -1 if not found


}


uint16_t dect_mac_utils_get_bytes_from_packet_length(uint32_t packet_length, uint32_t mcs)
{
    if(mcs > 4 || packet_length > 15){
        LOG_ERR("Invalid MCS or packet length");
        return 0;
    }
    return byte_per_mcs_and_length[mcs][packet_length];
}