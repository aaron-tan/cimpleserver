#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include "request.h"

int invalid_check(uint8_t head) {
  // Get the first 4 bits by masking with 11110000 (0xf0)
  uint8_t type = head & 0xf0;

  // Shift right 4 bits to get the first 4 bits.
  type = type >> 4;

  // Do an invalid check.
  if (type == 0x0 || type == 0x2 || type == 0x4 || type == 0x6) {
    // The type digit is not any request types.
    return 0;
  } else {
    return 1;
  }
}

int shutdown_request(uint8_t head) {
  // Get the first 4 bits.
  uint8_t type = head & 0xf0;
  type = type >> 4;

  // Check if it is a shutdown request.
  if (type == 0x8) {
    return 1;
  } else {
    return 0;
  }
}

int echo_request(uint8_t head) {
  // Get the first 4 bits by masking.
  uint8_t type = head & 0xf0;
  type = type >> 4;

  // Check if this is an echo request.
  if (type == 0x0) {
    return 1;
  } else {
    return 0;
  }
}

int dir_request(uint8_t head) {
  // Get the first 4 bits.
  uint8_t type = head & 0xf0;
  type = type >> 4;

  if (type == 0x2) {
    return 1;
  } else {
    return 0;
  }
}

int size_request(uint8_t head) {
  // Get the first 4 bits.
  uint8_t type = head & 0xf0;
  type = type >> 4;

  if (type == 0x4) {
    return 1;
  } else {
    return 0;
  }
}

int retrieve_request(struct message* msg, struct six_type* payl) {
  // Check the type digit.
  uint8_t type = msg->header & 0xf0;
  type = type >> 4;

  printf("Header: %hhx\n", msg->header);

  uint8_t paylen[8];
  memcpy(paylen, &msg->payload_len, 8);

  for (int i = 0; i < 8; i++) {
    printf("Payload length bytes: %hhx\n", paylen[i]);
  }

  for (int i = 0; i < msg->payload_len; i++) {
    printf("Payload: %hhx\n", msg->payload[i]);
  }

  if (type == 0x6) {
    payl->data = malloc(payl->var_len);

    memcpy(&payl->session_id, msg->payload, 4);
    memcpy(&payl->offset, (msg->payload + 4), 8);
    memcpy(&payl->data_len, (msg->payload + 12), 8);
    memcpy(payl->data, (msg->payload + 20), payl->var_len);

    return 1;
  } else {
    return 0;
  }
}

int is_compressed(uint8_t head) {
  if (head & 0x08) {
    return 1;
  } else {
    return 0;
  }
}

int requires_compression(uint8_t head) {
  if (head & 0x04) {
    return 1;
  } else {
    return 0;
  }
}
