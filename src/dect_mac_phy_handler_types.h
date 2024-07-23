#ifndef DEC_MAC_PHY_HANDLER_TYPES_H
#define DEC_MAC_PHY_HANDLER_TYPES_H

#include <zephyr/kernel.h>
#include <nrf_modem_dect_phy.h>


/* enumerate all the function that the phy api can do and give them a number */
enum dect_mac_phy_function {
    CAPABILITY_GET = 1,
    INIT = 2,
    DEINIT = 3,
    RX = 4,
    TX = 5,
    TX_HARQ = 6,
    TX_RX = 7,
    RSSI = 8,
    RX_STOP = 9,
    LINK_CONFIG = 10,
    TIME_GET = 11,
};

/* enumerate all the action that the phy api can be doing */
enum dect_mac_phy_state {
    IDLING = 0,
    GETTING_CAPABILITY = 1,
    INITIALIZING = 1,
    DEINITIALIZING = 2,
    TRANSMITTING = 2,
    RECEIVING = 3,
    MEASURING_RSSI = 4,
    STOPPING_RECEPTION = 5,
    CONFIGURING_LINK = 6,
    GETTING_TIME = 7,
};

/* feedback info as in table 6.2.2-2 of DECT-SPEC "DECT-2020 NR Part 4", due to endianness the order is different than in the specification. */
union feedback_info{
	struct mac_feedback_info_format_1{
		uint32_t transmission_feedback : 1;
		uint32_t harq_process_number : 3;
		uint32_t pad : 4;
		uint32_t buffer_status : 4;
		uint32_t channel_quality_indicator : 4;
	} format_1;
	struct mac_feedback_info_format_2 { 
		uint32_t channel_quality_indicator : 4;
		uint32_t pad : 4;
		uint32_t codebook_index : 3;
		uint32_t mimo_feedback : 1;
		uint32_t buffer_status : 4;
	} format_2;
	struct mac_feedback_info_format_3 {
		uint32_t transmission_feedback_transmitter : 1;
		uint32_t harq_process_number_transmitter : 3;
		uint32_t pad : 4;
		uint32_t channel_quality_indicator : 4;
		uint32_t transmission_feedback_receiver : 1;
		uint32_t harq_process_number_receiver : 3;
		
	} format_3;
	struct mac_feedback_byte {
		uint32_t hi : 4;
		uint32_t pad : 4;
		uint32_t lo : 8;
	} byte;
	uint16_t raw;
};

/* structure for the harq parameters in used in the tx function */
struct harq_tx_params {
    uint32_t redundancy_version : 2;
    uint32_t new_data_indication : 1;
    uint32_t harq_process_nr : 3;
    uint32_t buffer_size; // buffer size in bytes
};



/* structure for the parameters inside phy handler functions */
struct dect_mac_phy_handler_rx_params {
    uint32_t handle : 28;
    enum nrf_modem_dect_phy_rx_mode rx_mode;
    uint32_t rx_period_ms;
    uint16_t receiver_identity;
    uint64_t start_time;
};

struct dect_mac_phy_handler_tx_params {
    uint32_t handle : 28; 
    enum dect_phy_handler_tx_usage {
        NO_HARQ,
        HARQ,
        BEACON,
    } tx_usage;
    bool lbt_enable;
    uint8_t *data;
    size_t data_size;
    uint32_t receiver_id;
    struct feedback{
        uint32_t format;
        union feedback_info info;
    } feedback;
    struct harq_tx_params harq;
    uint64_t start_time;
};

/* Header type 1, due to endianness the order is different than in the specification. */
struct phy_ctrl_field_common_type1 {
	uint32_t packet_length : 4;
	uint32_t packet_length_type : 1;
	uint32_t header_format : 3;
	uint32_t short_network_id : 8;
	uint32_t transmitter_id_hi : 8;
	uint32_t transmitter_id_lo : 8;
	uint32_t df_mcs : 3;
	uint32_t reserved : 1;
	uint32_t transmit_power : 4;
};

/* Header type 2, due to endianness the order is different than in the specification. */
struct phy_ctrl_field_common_type2 {
	uint32_t packet_length : 4;
	uint32_t packet_length_type : 1;
	uint32_t header_format : 3;
	uint32_t short_network_id : 8;
	uint32_t transmitter_id_hi : 8;
	uint32_t transmitter_id_lo : 8;
	uint32_t df_mcs : 4;
	uint32_t transmit_power : 4;
	uint32_t receiver_id_hi : 8;
	uint32_t receiver_id_lo : 8;
	uint32_t df_harq_process_nr: 3; // set to 0 if header format is 001
	uint32_t df_ind : 1; 			// set to 0 if header format is 001
	uint32_t df_red_version : 2; 	// set to 0 if header format is 001
	uint32_t spatial_streams : 2;
	uint32_t feedback_info_hi : 4;
	uint32_t feedback_format : 4;
	uint32_t feedback_info_lo : 8;
};


enum nrf_mac_header_type{
	HEADER_TYPE_1 = 0,
	HEADER_TYPE_2 = 1,
};

enum nrf_mac_header_format{
	HEADER_FORMAT_000 = 0,
	HEADER_FORMAT_001 = 1,
};

enum nrf_mac_feedback_format{
	NO_FEEDBACK = 0,
	FEEDBACK_FORMAT_1 = 1,
	FEEDBACK_FORMAT_2 = 2,
	FEEDBACK_FORMAT_3 = 3,
	FEEDBACK_FORMAT_4 = 4,
	FEEDBACK_FORMAT_5 = 5,
};

#endif // DEC_MAC_PHY_HANDLER_TYPES_H