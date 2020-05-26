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
