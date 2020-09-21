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
  if (type == 0x0 || type == 0x2 || type == 0x4 || type == 0x6 || type == 0x9) {
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

int retrieve_request(struct message* msg, struct six_type* payl, struct huffman_tree* root) {
  // Check the type digit.
  uint8_t type = msg->header & 0xf0;
  type = type >> 4;

  if (is_compressed(msg->header)) {
    decompress_payload(msg, root);
  }

  payl->var_len = (msg->payload_len - 20);

  if (type == 0x6) {
    // Break up the payload into its respective sections and store it in payl struct.
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

int receive_request(uint8_t head) {
  // Get the first 4 bits.
  uint8_t type = head & 0xf0;
  type = type >> 4;

  if (type == 0x9) {
    return 1;
  } else {
    return 0;
  }
}

void receive_file(char* target_dir, struct message* msg) {
  // Get the length of the filename which is contained in the 1st 8 bytes.
  uint64_t filename_len;
  memcpy(&filename_len, msg->payload, 8);

  // Get the filename. Filename length includes null byte.
  char* filepath = malloc(strlen(target_dir)+ 1 + filename_len);
  memcpy(filepath, target_dir, strlen(target_dir));
  filepath[strlen(target_dir)] = '/';
  memcpy((filepath + strlen(target_dir) + 1), (msg->payload + 8), filename_len);

  // Open the file or create it if it does not exist.
  FILE* fp = fopen(filepath, "w+");

  // Seek the payload to where the contents begin.
  uint8_t* data = msg->payload + 8 + filename_len;

  // Get the length of the file contents.
  uint64_t data_len = msg->payload_len - 8 - filename_len;

  // Write the contents to the file.
  if (fwrite(data, 1, data_len, fp) < 1) {
    fprintf(stderr, "0 bytes of data was written\n");
  }

  fclose(fp);
  free(filepath);
  return;
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

int send_whole_file(uint8_t head) {
  if (head & 0x02) {
    return 1;
  } else {
    return 0;
  }
}
