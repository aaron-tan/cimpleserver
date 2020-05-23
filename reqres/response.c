#include "response.h"

void err_response(int socket_fd, uint8_t* err) {
  err[0] = 0xf0;

  for (int i = 1; i < 9; i++) {
    err[i] = 0x00;
  }

  // Send error response to client and close the connection.
  write(socket_fd, err, 9);

  return;
}

void echo_response(int socket_fd, struct message* msg) {
  uint8_t* resp = malloc(9 + msg->payload_len);
  resp[0] = msg->header;
  uint64_t paylen_be = htobe64(msg->payload_len);
  memcpy((resp + 1), &paylen_be, 8);
  memcpy((resp + 9), msg->payload, msg->payload_len);

  write(socket_fd, resp, 9 + msg->payload_len);

  free(resp);
  return;
}
