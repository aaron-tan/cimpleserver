#ifndef CONFIG_H
#define CONFIG_H
#include <inttypes.h>
#include <arpa/inet.h>

// struct config is used to store address, port and directory from config file.
struct config {
  struct in_addr address;
  uint16_t port;
  char* dir;
};

// read_config reads the config file with filename and stores values into struct config.
void read_config(char* filename, struct config* conf);

#endif
