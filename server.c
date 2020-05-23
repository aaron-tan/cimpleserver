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

int main(int argc, char** argv) {
  if (argc < 2) {
    puts("Need a configuration file.");
    return 0;
  }

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

	if(serversocket_fd < 0) {
		puts("This failed!");
		exit(1);
	}

	address.sin_family = AF_INET;
	address.sin_addr = conf.address;
	address.sin_port = conf.port;

	setsockopt(serversocket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(int));

	if(bind(serversocket_fd, (struct sockaddr*) &address, sizeof(struct sockaddr_in))) {
		puts("This broke! :(");
		exit(1);
	}

	listen(serversocket_fd, 4);
	while(1) {
		uint32_t addrlen = sizeof(struct sockaddr_in);
		clientsocket_fd = accept(serversocket_fd, (struct sockaddr*) &address, &addrlen);
		// pid_t p = fork();
    //
		// if(p == 0) {
		// }
    // Get client request and read as a message.
    read(clientsocket_fd, &msg.header, 1);
    read(clientsocket_fd, &msg.payload_len, 8);
    // msg.payload_len = msg.payload_len >> 56;
    msg.payload_len = htobe64(msg.payload_len);
    // printf("%ld\n", msg.payload_len);
    msg.payload = malloc(msg.payload_len);
    read(clientsocket_fd, msg.payload, msg.payload_len);

    uint8_t* cpy_buf = malloc(9 + msg.payload_len);
    cpy_buf[0] = msg.header;
    memcpy((cpy_buf + 1), &msg.payload_len, 8);
    memcpy((cpy_buf + 9), msg.payload, msg.payload_len);

    // for (int i = 0; i < (9 + msg.payload_len); i++) {
    //   printf("Read byte %hhx from client\n", cpy_buf[i]);
    // }

    if (invalid_check(msg.header)) {
      // Create an error response.
      uint8_t resp[9];
      err_response(resp);

      // Send error response to client and close the connection.
      write(clientsocket_fd, resp, 9);

      free(msg.payload);
      shutdown(clientsocket_fd, SHUT_RD);
      exit(1);
    }

    if (echo_request(msg.header)) {
      // Create a response.
      // int msg_len = 9 + msg.payload_len;
      // uint8_t resp[msg_len];
      // echo_response(resp);

      // Send the response to client.
      // uint8_t head = 0x10;
      // write(clientsocket_fd, &head, 1);
      // write(clientsocket_fd, &msg.payload_len, 8);
      // write(clientsocket_fd, msg.payload, msg.payload_len);
      msg.header = 0x10;
      echo_response(clientsocket_fd, &msg);
    }

    free(msg.payload);
    shutdown(clientsocket_fd, SHUT_RDWR);
    exit(1);
	}
	close(clientsocket_fd);
	close(serversocket_fd);
	return 0;

}
