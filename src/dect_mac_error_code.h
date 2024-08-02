#ifndef DECT_MAC_ERROR_CODE_H
#define DECT_MAC_ERROR_CODE_H

enum dect_mac_error_code
{
    OK = 0, // No error
    UNKNOWN_ERR = 1, // An unknown error occurred
    NO_FREE_HARQ = 2, // the program does not have any free harq process at the moment
    NO_MEM_HEAP = 3, // the program does not have enough memory in the heap to allocate the needed memory
    NO_FREE_NODE = 4, // the program does not have any free node at the moment
    NODE_NOT_FOUND = 5, // the node was not found in the hashmap
    POWER_NOT_IN_RANGE = 6, // the power is not in the range of 0 to 15
    CHANNEL_TOO_NOISY = 7, // the channel is too noisy to transmit with <10% PER
    MIN_MCS_REACHED = 8, // the minimum MCS has already been reached and it cannot be decreased
    MAX_MCS_REACHED = 9, // the maximum MCS has already been reached and it cannot be increased
    NO_MEM_SLAB = 10, // the program does not have enough memory in the slab to allocate the needed memory
    FUNC_NOT_EXIST = 11, // the function does not exist
};




#endif // DECT_MAC_ERROR_CODE_H