#ifndef SERVER_H
#define SERVER_H

// struct message is used to store client requests and server responses.
struct message {
  uint8_t header;
  uint64_t payload_len;
  uint8_t* payload;
};

#endif
