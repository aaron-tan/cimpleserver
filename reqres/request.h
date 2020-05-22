#ifndef REQ_H
#define REQ_H

// The request files contains functions used to handle all client requests.

// invalid_check checks the type digit in the header if it is invalid.
int invalid_check(uint8_t head);

// Check if message is an echo request.
int echo_request(uint8_t head);

#endif
