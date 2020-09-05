#include <sys/socket.h>
#include <assert.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>
#include <unistd.h>

int main(int argc, char ** argv) {
    // Compile with: gcc -o genconf -O0 -Wall -Werror -Werror=vla -std=gnu11 -g genconf.c
    // Usage: ./generate-config -i <dotted quad ipv4 address xxx.xxx.xxx.xxx>
    // -p <port number> -t <target directory> CONFIG_FILENAME
    // For example: ./generate-config -i 127.0.0.1 -p 8888 -t /dir config_file
    // assert(argc == 4);
    int opt;
    long rawport;

    // struct in_addr is how the address is stored
    struct in_addr inaddr;

    // All of the options must be provided, so we check if there are 8 args.
    if (argc != 8) {
      fprintf(stderr, "Usage: %s [-i IPv4 addr] [-p port] [-t target dir] CONFIG_FILENAME\n",
        argv[0]);
      exit(EXIT_FAILURE);
    }

    // Place the config file in the parent directory.
    char* parent_dir = malloc(4 + strlen(argv[7]));
    memcpy(parent_dir, "../", 4);
    memcpy((parent_dir + 3), argv[7], strlen(argv[7]) + 1);

    // Open the file given by CONFIG_FILENAME
    FILE* fp = fopen(parent_dir, "w+");

    while ((opt = getopt(argc, argv, "i:p:t:")) != -1) {
      switch (opt) {
        case 'i':
          // Convert address from dotted quad notation (e.g. 127.0.0.1) to network byte order integer, and also save it to the struct
          // Access the integer representation itself with the s_addr field of struct in_addr
          inet_pton(AF_INET, argv[2], &inaddr);

          // First 4 bytes: address in network byte order
          fwrite(&(inaddr.s_addr), sizeof(inaddr.s_addr), 1, fp);
          break;
        case 'p':
          // Convert port to a long
          rawport = strtol(argv[4], NULL, 10);

          // Port is 16-bit unsigned integer, so 0-65535. Port 0 is special and cannot be used.
          assert(rawport > 0);
          assert(rawport < 65536);

          // Convert port number to network byte order
          uint16_t port = htons((uint16_t)rawport);

          // Next 2 bytes: port in network byte order
          fwrite(&port, sizeof(port), 1, fp);
          break;
        case 't':
          // Remainder: directory as non-NULL terminated ASCII
          fwrite(argv[6], strlen(argv[6]), 1, fp);
          break;
        default:
          fprintf(stderr, "Usage: %s [-i IPv4 addr] [-p port] [-t target dir] CONFIG_FILENAME\n",
            argv[0]);
          free(parent_dir);
          exit(EXIT_FAILURE);
      }
    }

    free(parent_dir);
    return 0;
}
