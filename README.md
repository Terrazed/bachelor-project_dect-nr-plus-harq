# DECT NR+ HARQ Implementation

_insert logos of "DECT NR+", "Nordic semi", "HEVS"_

## About The Project
This project is undertaken as part of Simon Donnet-Monay's bachelor's thesis in collaboration with **HES-SO Valais/Wallis** and **Nordic Semiconductor** 

## Description
This repository contains an implementation of a part of the MAC layer. The implemented part is the HARQ protocol. The HARQ protocol is the protocol that handle the acknowledgement and retransmission of messages sent. 

This can be used as an example of how to implement the HARQ protocol and also shows a way to use the [DECT NR+ PHY API](https://docs.nordicsemi.com/bundle/ncs-latest/page/nrfxlib/nrf_modem/doc/api.html#dect_nr_phy_api).


## Files Organisation
- **dect_mac_phy_handler**: This file contains all the functions needed to send operation to the PHY API (user → PHY)
- **dect_mac_phy_handler_cb**: This file contains the callback from the PHY API (PHY → user)
- **dect_mac_phy_handler_queue**: this file contains a queue that plans operations through the handler. This is done to handle the fact that the PHY API is not thread-safe. The queue handle operation's priority and permanent operation (operation that reschedule themself indefinitely).
- **dect_mac_phy_handler_type**: This file contains all the custom types (structs, enum, defines, ...) used in the dect_mac_phy_handler_xxx files.
- **dect_mac_node**: this file can be used to memorize other devices (nodes) and optimize the power and modulation for all the cached nodes.
- **dect_mac_harq**: this file handle all the HARQ-related process. It handles the transmissions, retransmissions, acknowledgement, ...
- **dect_mac_error_code**: this file contains all the error codes used in the mac layer.
- **dect_mac_utils**: this file contains some function that are used in the mac layer but were not relevent to put in other files.

## Requirement
To use this project it is required to have a basic knowledge of the DECT NR+ protocol, to know how to use Zephyr RTOS and to have a compatible hardware.

### Knowledge
To understand the DECT NR+ protocol it is ***highly recommended*** to read at least once the following technical specification: 
- **[Part 1: Overview [ETSI_TS_103_636-1]](./doc/dect-nr-plus-specifications/etsi_ts_103_636-1_v1.5.1.pdf)**
- **[Part 3: Physical layer [ETSI_TS_103_636-3]](./doc/dect-nr-plus-specifications/etsi_ts_103_636-3_v1.5.1.pdf)**
- **[Part 4: MAC layer [ETSI_TS_103_636-4]](./doc/dect-nr-plus-specifications/etsi_ts_103_636-4_v1.5.1.pdf)**

The most important parts for this project are in the MAC layer (**5.5: "HARQ Operation"** and **6.2: "Physical Header Field"**)

To understand the PHY layer API it is also recommended to read **[this article](https://docs.nordicsemi.com/bundle/ncs-latest/page/nrfxlib/nrf_modem/doc/dectphy.html)**

***Before running this project be aware of the radio regulation on the 1.9 GHz band in you country to change the CONFIG_TX_POWER in the [prj.conf](./prj.conf) file***

### Software

Before starting with this project, it is better to have a good understanding of the Zephyr RTOS and also to be familliar with RTOS concepts.

This project is using the nRF connect SDK and needs at least SDK v2.7.0.

To run this project, **the DECT NR+ modem firmware** is needed to run on the modem core. _(To obtain the DECT NR+ PHY firmware, you must contact the Nordic Semiconductor sales department)_

_There is a branch for the DECT NR+ PHY API v1.1.0, but to run it, it needs the modem firmware version 1.1.0 which is not released yet. The main difference is that is implements some power managment features and some function calls are a bit differents. There is a preview [here](https://github.com/nrfconnect/sdk-nrf/pull/16142)_

### Hardware

This project has been made for nRF91x1 developpement kits but should be able t run on any board containing a [DECT NR+ compatible nRF chip](https://www.nordicsemi.com/Products/Wireless/DECT-NR/Products).




## Author and acknowledgement


