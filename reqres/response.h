#ifndef RES_H
#define RES_H

// The response files contains functions used in handling server responses.

// Construct an error response with type 0xf0 and payload length 0 (no payload)
void err_response(uint8_t* err);

#endif
