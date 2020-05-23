#include "response.h"

void err_response(uint8_t* err) {
  err[0] = 0xf0;

  for (int i = 1; i < 9; i++) {
    err[i] = 0x00;
  }

  return;
}

void echo_response(int socket_fd, struct message* msg) {
  uint8_t* resp = malloc(9 + msg->payload_len);
  resp[0] = 0x01;
  memcpy((resp + 1), &msg->payload_len, 8);
  memcpy((resp + 9), msg->payload, msg->payload_len);

  for (int i = 0; i < (9 + msg->payload_len); i++) {
    printf("Resp message contains byte %hhx\n", resp[i]);
  }

  // write(socket_fd, &msg->header, 1);
  // write(socket_fd, &msg->payload_len, 8);
  // write(socket_fd, msg->payload, msg->payload_len);
  write(socket_fd, resp, 9 + msg->payload_len);

  free(resp);
  return;
}
