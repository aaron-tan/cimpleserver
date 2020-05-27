#ifndef REQ_H
#define REQ_H
#include <string.h>
#include "../server.h"

// The request files contains functions used to handle all client requests.

// invalid_check checks the type digit in the header if it is invalid.
int invalid_check(uint8_t head);

// Check if we get a shutdown request. Shutdown the server if we do.
int shutdown_request(uint8_t head);

// Check if message is an echo request.
int echo_request(uint8_t head);

// Check if type digit is directory listing.
int dir_request(uint8_t head);

// Check if type digit is file size query.
int size_request(uint8_t head);

// Check if it is a retrieve request. If it is, copy session id, offset, data length
// and filename into the arguments given, then return 1 or 0.
int retrieve_request(struct message* msg, struct six_type* payl);

#endif
