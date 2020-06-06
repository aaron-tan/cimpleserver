#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdint.h>
#include "server.h"
#include "conf/readconfig.h"
#include "reqres/request.h"
#include "reqres/response.h"
#include "reqres/compression.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    puts("Need a configuration file.");
    return 0;
  }

  fd_set master;
  fd_set read_fds;
  int maxfd;
  int ret;

  // Clear the master and read_fds set.
  FD_ZERO(&master);
  FD_ZERO(&read_fds);

	int serversocket_fd = -1;
	int clientsocket_fd = -1;

  // Create an address and a message struct to store when read.
  struct message msg;
  struct sockaddr_in address;
  int option = 1;

  // Read the configuration file.
	struct config conf;
  read_config(argv[1], &conf);

	serversocket_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (serversocket_fd < 0) {
		close(serversocket_fd);
		exit(1);
	}

	address.sin_family = AF_INET;
	address.sin_addr = conf.address;
	address.sin_port = conf.port;

	setsockopt(serversocket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(int));

	if (bind(serversocket_fd, (struct sockaddr*) &address, sizeof(struct sockaddr_in))) {
		close(serversocket_fd);
		exit(1);
	}

	listen(serversocket_fd, 4);

  // Add serversocket_fd to the set.
  FD_SET(serversocket_fd, &master);
  // Keep track of the biggest file descriptor.
  maxfd = serversocket_fd;

  // Read the compression dictionary.
  int file_size;
  uint32_t* bit_array = read_compress("compression.dict", &file_size);

  struct bit_code* code_dict = create_dict(bit_array, &file_size);

  struct huffman_tree* root = create_huffman_tree(code_dict);

  FILE* sessionsp = fopen("./sessions", "wb+");

  if (sessionsp == NULL) {
    perror("Error creating/opening sessions file");
  }

	while (1) {
    // We do this because select changes the set so we use two sets to keep track of this change.
    read_fds = master;

    ret = select(maxfd + 1, &read_fds, NULL, NULL, NULL);

    if (ret < 0) {
      perror("Select error");
      exit(1);
    }

    // If there is an incoming conection to clientsocket_fd it's new.
    if (FD_ISSET(serversocket_fd, &read_fds)) {
      // Accept the new connection and add this new fd into the set.
      uint32_t addrlen = sizeof(struct sockaddr_in);
      clientsocket_fd = accept(serversocket_fd, (struct sockaddr*) &address, &addrlen);

      FD_SET(clientsocket_fd, &master);
      if (clientsocket_fd > maxfd) {
        maxfd = clientsocket_fd;
      }

    } else {
      // Otherwise we have activity on existing connections. Handle them.
      for (int i = 0; i <= maxfd; i++) {
        if (FD_ISSET(i, &read_fds)) {
          printf("Connection: %d\n", i);
          // Get client request and read as a message.
          if (recv(i, &msg.header, 1, 0) == 0) {
            close(i);
            FD_CLR(i, &master);

            continue;
          }

          if (shutdown_request(msg.header)) {
            // Close all connections.
            for (int i = 0; i <= maxfd; i++) {
              shutdown(i, SHUT_RDWR);
            }

            remove("./sessions");
            fclose(sessionsp);
            destroy_huffman_tree(root);
            free(code_dict);
            free(bit_array);
            free(conf.dir);
            return 0;
          }

          if (recv(i, &msg.payload_len, 8, 0) == 0) {
            close(i);
            FD_CLR(i, &master);

            continue;
          }

          msg.payload_len = htobe64(msg.payload_len);

          msg.payload = malloc(msg.payload_len);

          if (msg.payload_len != 0) {
            recv(i, msg.payload, msg.payload_len, 0);
          }

          if (invalid_check(msg.header)) {
            // Create an error response.
            uint8_t resp[9];
            err_response(resp);

            // Send error response to client and close the connection.
            write(i, resp, 9);

            free(msg.payload);
            close(i);
            FD_CLR(i, &master);

            continue;
          }

          if (echo_request(msg.header)) {
            // Send a echo response.
            if (requires_compression(msg.header) && !is_compressed(msg.header)) {
              // If it requries compression and it's not compressed, compress msg.
              msg.header = 0x18;
              echo_response(i, &msg, 1, code_dict);
            } else if (requires_compression(msg.header) && is_compressed(msg.header)) {
              // If it is already compressed we don't compress twice.
              msg.header = 0x18;
              echo_response(i, &msg, 0, code_dict);
            } else {
              // Check if payload is compressed, if it is decompress it.
              if (is_compressed(msg.header)) {
                decompress_payload(&msg, root);
                msg.header = msg.header & 0x08;
              }

              msg.header = 0x10;
              echo_response(i, &msg, 0, code_dict);
            }
          }

          if (dir_request(msg.header)) {
            // Send a response with filenames in the payload.
            if (requires_compression(msg.header)) {
              // Compress the msg.
              msg.header = 0x38;
              dir_response(i, conf.dir, &msg, 1, code_dict);
            } else {
              msg.header = 0x30;
              dir_response(i, conf.dir, &msg, 0, code_dict);
            }
          }

          if (size_request(msg.header)) {
            // Send a response with the file size.
            if (requires_compression(msg.header)) {
              msg.header = 0x58;
              size_response(i, conf.dir, &msg, 1, code_dict);
            } else {
              msg.header = 0x50;
              size_response(i, conf.dir, &msg, 0, code_dict);
            }
          }

          /**Check if it is a retrieve file request, if it is split payload
          * into the struct payl given, representing its corresponding name.
          */
          struct six_type payl;

          // if (is_compressed(msg.header)) {
          //   decompress_payload(&msg, root);
          // }
          // payl.var_len = (msg.payload_len - 20);

          if (retrieve_request(&msg, &payl, root)) {
            // Send the file as a response.
            if (requires_compression(msg.header) && !is_compressed(msg.header)) {
              // If requires compression and it is not compressed, compress payload.
              msg.header = 0x78;
              retrieve_response(i, &msg, conf.dir, &payl, 1, code_dict, sessionsp);
            } else if (requires_compression(msg.header) && is_compressed(msg.header)) {
              // Decompress the payload to read the payload.
              decompress_payload(&msg, root);

              // Create the response and compress this new response.
              msg.header = 0x78;
              retrieve_response(i, &msg, conf.dir, &payl, 1, code_dict, sessionsp);
            } else {
              // Decompress the payload first.
              // if (is_compressed(msg.header)) {
              //   decompress_payload(&msg, root);
              // }

              msg.header = 0x70;
              retrieve_response(i, &msg, conf.dir, &payl, 0, code_dict, sessionsp);
            }

          }

          free(msg.payload);
        }
      }
    }

	}
	close(clientsocket_fd);
	close(serversocket_fd);
	return 0;

}
