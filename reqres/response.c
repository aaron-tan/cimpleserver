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
