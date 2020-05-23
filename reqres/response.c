#include "response.h"

void err_response(uint8_t* err) {
  err[0] = 0xf0;

  for (int i = 1; i < 9; i++) {
    err[i] = 0x00;
  }

  return;
}

void echo_response(int socket_fd, struct message* msg) {
  write(socket_fd, &msg->header, 1);
  write(socket_fd, &msg->payload_len, 8);
  write(socket_fd, msg->payload, msg->payload_len);

  return;
}
