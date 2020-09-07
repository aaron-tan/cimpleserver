#ifndef RES_H
#define RES_H
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <endian.h>
#include <dirent.h>
#include "../server.h"
#include "compression.h"

// The response files contains functions used in handling server responses.

// Construct an error response with type 0xf0 and payload length 0 (no payload)
void err_response(uint8_t* err);

// Construct an echo response with type 0x10 with the payload.
void echo_response(int socket_fd, struct message* msg, int compress, struct bit_code* dict);

// Construct a response to the directory request and get the filenames.
void dir_response(int socket_fd, char* target, struct message* msg, int compress, struct bit_code* dict);

// Construct a response for the file size query.
void size_response(int socket_fd, char* target_dir, struct message* msg, int compress, struct bit_code* dict);

// Handle multiplexing of multiple retrieve file connections.
int multiplex_handling(FILE* sessionsp, struct six_type* payl, int socket_fd);

// Send file data to the client. Passed in payload structure as arguments.
void retrieve_response(int socket_fd, struct message* msg, char* target_dir,
  struct six_type* payl, int compress, struct bit_code* dict, FILE* sessionsp, int sendall_file);
#endif
