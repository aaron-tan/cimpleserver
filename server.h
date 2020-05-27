#ifndef SERVER_H
#define SERVER_H

// struct message is used to store client requests and server responses.
struct message {
  uint8_t header;
  uint64_t payload_len;
  uint8_t* payload;
};

// This struct is used to store the payload structure of retrieve file (0x6 type).
struct six_type {
  uint32_t session_id;
  uint64_t offset;
  uint64_t data_len;
  char* data;
  int var_len;  // Length of the variable bytes, different to data_len, see specs.
};

#endif
