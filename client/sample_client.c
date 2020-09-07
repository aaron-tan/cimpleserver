#include <stdlib.h>
#include <sys/socket.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
  /** Sample client program to connect to the server.
  * For now, this program only connects to the server. Features to be added.
  * Usage: ./sample_client <dotted quad ipv4 address xxx.xxx.xxx.xxx> <port number>
  * For example: ./sample_client 127.0.0.1 9000
  */
  assert(argc == 3);

  // Get the port.
  long rawport = strtol(argv[2], NULL, 10);

  // Port is 16-bit unsigned integer, so 0-65535. Port 0 is special and cannot be used.
  assert(rawport > 0);
  assert(rawport < 65536);

  // Convert port number to network byte order
  uint16_t port = htons((uint16_t)rawport);

  // struct in_addr is how the address is stored
  struct in_addr inaddr;

  // Convert address from dotted quad notation (e.g. 127.0.0.1) to network byte order integer, and also save it to the struct
  // Access the integer representation itself with the s_addr field of struct in_addr
  inet_pton(AF_INET, argv[1], &inaddr);

  // Create socket, and check for error
  // AF_INET = this is an IPv4 socket
  // SOCK_STREAM = this is a TCP socket
  // connection is already a file descriptor. It just isn't connected to anything yet.
  int connection = socket(AF_INET, SOCK_STREAM, 0);
  if (connection < 0) {
      perror("socket");
      return 1;
  }

  // struct sockaddr_in is how you specify a network address
  // Because this is the client, this represents our connection destination
  // However, you will use a very similar construct on the server, to represent the address where the server listens
  // sin_family = AF_INET = this is an IPv4 address
  // sin_port = the port number goes here
  // sin_addr = the address (as a struct in_addr) goes here
  struct sockaddr_in destination;
  destination.sin_family = AF_INET;
  destination.sin_port = port;
  destination.sin_addr = inaddr;

  // connect(2) actually makes the connection. Check for error.
  if (connect(connection, (struct sockaddr *)&destination, sizeof(destination)) != 0) {
      perror("connect");
      return 1;
  }

  // Once connect succeeds, we can start read and write to the connection file descriptor.

  // We send a shutdown command for now.
  uint8_t shutdown[9] = {0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  write(connection, shutdown, 9);

  return 0;
}
