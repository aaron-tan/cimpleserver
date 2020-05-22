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
  // if (type == 0x0 || type == 0x2 || type == 0x4 || type == 0x6) {
  //   // The type digit is a request type.
  //   return 0;
  // } else 
  if (type == 0x1 || type == 0x3 || type == 0x5 || type == 0x7) {
    // Type digit is a response type return invalid.
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
