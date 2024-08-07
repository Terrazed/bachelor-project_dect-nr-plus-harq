# DECT NR+ HARQ Implementation

_insert logos of "DECT NR+", "Nordic semi", "HEVS"_

## About The Project
This project is undertaken as part of Simon Donnet-Monay's bachelor's thesis in collaboration with **HES-SO Valais/Wallis** and **Nordic Semiconductor** 

## Description
This repository contains an implementation of a part of the MAC layer. The implemented part is the HARQ protocol. The HARQ protocol is the protocol that handle the acknowledgement and retransmission of messages sent. 

This can be used as an example of how to implement the HARQ protocol and also shows a way to use the [DECT NR+ PHY API](https://docs.nordicsemi.com/bundle/ncs-latest/page/nrfxlib/nrf_modem/doc/api.html#dect_nr_phy_api).


## Files Organisation
- dect_mac_phy_handler: This file contains all the functions needed to send operation to the PHY API (user → PHY)
- dect_mac_phy_handler_cb: This file contains the callback from the PHY API (PHY → user)
- dect_mac_phy_handler_queue: this file contains a queue that plans operations through the handler. This is done to handle the fact that the PHY API is not thread-safe. The queue handle operation's priority and permanent operation (operation that reschedule themself indefinitely).
- dect_mac_phy_handler_type: This file contains all the custom types (structs, enum, defines, ...) used in the dect_mac_phy_handler_xxx files.
- dect_mac_node: this file can be used to memorize other devices (nodes) and optimize the power and modulation for all the cached nodes.
- dect_mac_harq: this file handle all the HARQ-related process. It handles the transmissions, retransmissions, acknowledgement, ...
- dect_mac_error_code: this file contains all the error codes used in the mac layer.
- dect_mac_utils: this file contains some function that are used in the mac layer but were not relevent to put in other files.
## Requirement


### Knowledge


### Hardware


### Software


## Author and acknowledgement


