#ifndef RES_H
#define RES_H
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <endian.h>
#include "../server.h"

// The response files contains functions used in handling server responses.

// Construct an error response with type 0xf0 and payload length 0 (no payload)
void err_response(int socket_fd, uint8_t* err);

// Construct an echo response with type 0x10 with the payload.
void echo_response(int socket_fd, struct message* msg);

#endif
