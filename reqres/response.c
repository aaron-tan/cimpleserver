#include "response.h"

void err_response(uint8_t* err) {
  err[0] = 0xf0;

  for (int i = 1; i < 9; i++) {
    err[i] = 0x00;
  }

  return;
}

void echo_response(int socket_fd, struct message* msg) {
  uint8_t* resp = malloc(9 + msg->payload_len);
  resp[0] = msg->header;
  uint64_t paylen_be = htobe64(msg->payload_len);
  memcpy((resp + 1), &paylen_be, 8);
  memcpy((resp + 9), msg->payload, msg->payload_len);

  write(socket_fd, resp, 9 + msg->payload_len);

  free(resp);
  return;
}

void dir_response(int socket_fd, char* target, struct message* msg) {
  DIR* dirp;
  struct dirent* dir;
  // Get the total string length of all filenames. Used for payload length.
  uint64_t string_len = 0;
  // Used to count the number of files in the dir.
  int dir_count = 0;
  // We store all the file names in filenames, a malloc'd array of strings.
  char** filenames = malloc(sizeof(char*));

  dirp = opendir(target);

  // Error opening the directory.
  if (dirp == NULL) {
    perror("Error opening the directory");
    return;
  }

  // Read each entry in the directory.
  while ((dir = readdir(dirp)) != NULL) {
    if (dir->d_type == DT_REG) {
      // Get the total string length of all file names.
      string_len += strlen(dir->d_name) + 1;
      filenames[dir_count] = malloc(strlen(dir->d_name) + 1);
      strcpy(filenames[dir_count], dir->d_name);
      dir_count += 1;

      // Set dir_count + 1 so we can realloc a larger array by one.
      filenames = realloc(filenames, (dir_count + 1) * sizeof(char*));
    }
  }

  // Set up the payload, consisting of a byte for each char in filename.
  uint8_t* resp = malloc(9 + string_len);
  int iter = 9;

  // Type digit.
  resp[0] = msg->header;
  // Payload length in big endian.
  uint64_t be_strlen = htobe64(string_len);
  memcpy((resp + 1), &be_strlen, 8);

  if (dir_count == 0) {
    resp = realloc(resp, 10);
    uint8_t nullb = 0x00;
    memcpy((resp + 9), &nullb, 1);

    write(socket_fd, resp, 10);
  } else {
    // Loop through each filename and assign the byte to the payload.
    for (int i = 0; i < dir_count; i++) {
      memcpy((resp + iter), filenames[i], strlen(filenames[i]) + 1);
      iter += strlen(filenames[i]) + 1;
    }

    write(socket_fd, resp, 9 + string_len);
  }


  // Free filenames, and resp.
  for (int i = 0; i < dir_count; i++) {
    free(filenames[i]);
  }

  free(filenames);
  free(resp);
  closedir(dirp);

  return;
}


void size_response(int socket_fd, struct message* msg) {
  uint64_t len = msg->payload_len;
  char* filename = malloc(len);
  memcpy(filename, msg->payload, len);

  if (access(filename, F_OK) != 0) {
    uint8_t err[9];
    err_response(err);
    write(socket_fd, err, 9);

    free(filename);
    return;
  }

  FILE* fp = fopen(filename, "rb");

  // Seek to the end of the file and get the position at the end.
  fseek(fp, 0, SEEK_END);

  // Get the end position, that is the size of the file.
  uint64_t fsize = (uint64_t) ftell(fp);

  // Construct the message.
  uint8_t* resp = malloc(17);
  resp[0] = msg->header;

  uint64_t paylen = 8;
  uint64_t paylen_be = htobe64(paylen);
  memcpy((resp + 1), &paylen_be, 8);

  uint64_t fsize_be = htobe64(fsize);
  memcpy((resp + 9), &fsize_be, 8);

  write(socket_fd, resp, 17);

  fclose(fp);
  free(filename);
  return;
}
