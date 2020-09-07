#ifndef REQ_H
#define REQ_H
#include <string.h>
#include "../server.h"
#include "compression.h"

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
int retrieve_request(struct message* msg, struct six_type* payl, struct huffman_tree* root);

// Check the header to see if payload is compressed.
int is_compressed(uint8_t head);

// Check the header to see if the response is required to be compressed.
int requires_compression(uint8_t head);

// Check if the 7th bit of a retrieve request is set to 1.
// If true, we read the whole file contents and send it to the client.
int send_whole_file(uint8_t head);

#endif
