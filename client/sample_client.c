#include <stdlib.h>
#include <sys/socket.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

void send_file(int socket_fd, char* filename) {
  // Open the file we are to send.
  FILE* fp = fopen(filename, "r+");

  // Declare stat to access file size.
  struct stat buf;
  if (stat(filename, &buf) < 0) {
    perror("stat");
    exit(EXIT_FAILURE);
  }

  // Get the length of the string (including the null byte).
  uint64_t filename_len = strlen(filename) + 1;

  // Construct the message.
  uint8_t* msg = malloc(17 + filename_len + buf.st_size);
  uint64_t msg_len = 17 + filename_len + buf.st_size;
  msg[0] = 0x90;  // Type digit
  memcpy((msg + 1), &msg_len, 8); // Payload length
  memcpy((msg + 9), &filename_len, 8);  // Length of filename
  memcpy((msg + 17), filename, filename_len); // Filename

  fread((msg + (17 + filename_len)), buf.st_size, 1, fp); // File contents.

  // Send the message.
  write(socket_fd, msg, msg_len);

  fclose(fp);
  free(msg);
  return;
}

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
  uint8_t type_digit;
  uint64_t payload_len;
  char* payload;

  int compressed;
  int req_compress;

  printf("What is the request you would like to send?\n");
  scanf("%hhu", &type_digit);

  // Type digit is the leftmost 4 bits. Shift left bitwise by 4.
  type_digit = type_digit << 4;

  printf("Specify payload length\n");
  scanf("%lu", &payload_len);
  getchar();  // Remove the trailing newline from scanf.

  payload = malloc(payload_len);

  printf("Input the payload\n");
  fgets(payload, payload_len + 1, stdin);

  printf("Is payload compressed?\n");
  scanf("%d", &compressed);

  // Set 5th bit (compression bit) to 1 if payload is compressed.
  if (compressed) {
    type_digit = type_digit | 0x08;
  }

  printf("Require compression in response?\n");
  scanf("%d", &req_compress);

  // Set 6th bit (requires compression) to 1 if response must be compressed.
  if (req_compress) {
    type_digit = type_digit | 0x04;
  }

  int msg_len = 9 + payload_len;
  uint8_t* msg = malloc(9 + payload_len);

  // Copy the type digit, payload length and payload into msg.
  memcpy(msg, &type_digit, 1);
  memcpy((msg + 9), payload, payload_len);

  // Convert length to network byte order (big endian).
  payload_len = htobe64(payload_len);
  memcpy((msg + 1), &payload_len, 8);

  // Print msg.
  printf("Client message: ");
  for (int i = 0; i < msg_len; i++) {
    printf("0x%hhx ", msg[i]);
  }
  puts("");

  // Send the message to the server.
  write(connection, msg, msg_len);

  // Read the response from the server. Copy response into msg.
  uint8_t resp_type;
  uint64_t resp_len;

  if (read(connection, &resp_type, 1) < 1) {
    // There is no server response. Free all and return.
    free(payload);
    free(msg);
    return 0;
  }
  read(connection, &resp_len, 8);

  resp_len = htobe64(resp_len);

  // Reallocate msg memory.
  msg_len = 9 + resp_len;
  msg = realloc(msg, (9 + resp_len));

  // Convert resp_len back to host byte order.
  resp_len = be64toh(resp_len);

  // Memcpy response into msg.
  memcpy(msg, &resp_type, 1);
  memcpy((msg + 1), &resp_len, 8);
  // Read into msg directly.
  read(connection, (msg + 9), resp_len);

  // Print response message.
  printf("Server response: ");
  for (int i = 0; i < msg_len; i++) {
    printf("0x%hhx ", msg[i]);
  }
  puts("");

  free(payload);
  free(msg);
  return 0;
}
