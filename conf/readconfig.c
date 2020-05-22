#include <stdio.h>
#include <stdlib.h>
#include "readconfig.h"

void read_config(char* filename, struct config* conf) {
  FILE* fp = fopen(filename, "rb");

  if (fp == NULL) {
    perror("Error opening file. Try again.");
    return;
  }

  fread(&conf->address, 4, 1, fp);
  fread(&conf->port, 2, 1, fp);

  // Calculate the remaining number of bytes.
  // Get the current position.
  long pos = ftell(fp);

  // Seek to the end of the file.
  fseek(fp, 0, SEEK_END);

  // Calculate the bytes by subtracting previous position pos from current position.
  long bytes = ftell(fp) - pos;

  // Go back to the original position.
  fseek(fp, -bytes, SEEK_END);

  // Malloc the char* dir array in conf.
  conf->dir = malloc(bytes + 1);

  // Read the remaining bytes.
  fread(conf->dir, (size_t) bytes, 1, fp);

  // Add null terminator to the string.
  conf->dir[bytes] = '\0';

  return;
}
