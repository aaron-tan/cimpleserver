#include <inttypes.h>
#include <string.h>
#include "response.h"

void err_response(uint8_t* err) {
  err[0] = 0xf0;

  for (int i = 1; i < 9; i++) {
    err[i] = 0x00;
  }

  return;
}

void echo_response(struct message* msg, uint8_t* resp) {
  // Header type.
  resp[0] = 0x10;

  memcpy((resp + 1), &msg->payload_len, 8);

  memcpy((resp + 9), msg->payload, msg->payload_len);

  return;
}
