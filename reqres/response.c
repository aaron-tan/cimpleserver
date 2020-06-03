#include <sys/types.h>
#include <sys/stat.h>
#include "response.h"

void err_response(uint8_t* err) {
  err[0] = 0xf0;

  for (int i = 1; i < 9; i++) {
    err[i] = 0x00;
  }

  return;
}

void echo_response(int socket_fd, struct message* msg, int compress, struct bit_code* dict) {
  // If payload requires compression compress the payload.
  if (compress) {
    compress_payload(msg, dict);
  }
  // printf("Header: %hhx\n", msg->header);
  // printf("Payload len: %ld\n", msg->payload_len);

  uint8_t* resp = malloc(9 + msg->payload_len);
  resp[0] = msg->header;
  uint64_t paylen_be = htobe64(msg->payload_len);
  // printf("Payload len be: %ld\n", paylen_be);
  memcpy((resp + 1), &paylen_be, 8);
  memcpy((resp + 9), msg->payload, msg->payload_len);
  // for (int i = 0; i < (9 + msg->payload_len); i++) {
  //   printf("%hhx\n", resp[i]);
  // }

  write(socket_fd, resp, 9 + msg->payload_len);

  free(resp);
  return;
}

void dir_response(int socket_fd, char* target, struct message* msg, int compress, struct bit_code* dict) {
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
  // uint8_t* resp = malloc(9 + string_len);
  // int iter = 9;
  int iter = 0;
  int payload_cap = 0;

  // Type digit.
  // resp[0] = msg->header;
  // Payload length in big endian.
  // uint64_t be_strlen = htobe64(string_len);
  // printf("%ld\n", be_strlen);
  // memcpy((resp + 1), &be_strlen, 8);
  memcpy(&msg->payload_len, &string_len, 8);

  if (dir_count == 0) {
    // resp = realloc(resp, 10);
    uint8_t nullb = 0x00;
    // memcpy((resp + 9), &nullb, 1);
    memcpy(msg->payload, &nullb, 1);

    if (compress) {
      echo_response(socket_fd, msg, 1, dict);
    } else {
      echo_response(socket_fd, msg, 0, dict);
    }

    // write(socket_fd, resp, 10);
  } else {
    // Loop through each filename and assign the byte to the payload.
    for (int i = 0; i < dir_count; i++) {
      // memcpy((resp + iter), filenames[i], strlen(filenames[i]) + 1);
      // printf("%s\n", filenames[i]);
      payload_cap += strlen(filenames[i]) + 1;
      // printf("%d\n", iter);
      msg->payload = realloc(msg->payload, payload_cap * sizeof(uint8_t));
      memcpy((msg->payload + iter), filenames[i], strlen(filenames[i]) + 1);
      iter += strlen(filenames[i]) + 1;
    }

    if (compress) {
      echo_response(socket_fd, msg, 1, dict);
    } else {
      echo_response(socket_fd, msg, 0, dict);
    }

    // write(socket_fd, resp, 9 + string_len);
  }


  // Free filenames, and resp.
  for (int i = 0; i < dir_count; i++) {
    free(filenames[i]);
  }

  free(filenames);
  // free(resp);
  closedir(dirp);

  return;
}


void size_response(int socket_fd, char* target_dir, struct message* msg, int compress, struct bit_code* dict) {
  struct stat buf;
  int ret;
  uint64_t len = msg->payload_len;
  char* filename = malloc(strlen(target_dir) + 1 + len);

  memcpy(filename, target_dir, strlen(target_dir));
  filename[strlen(target_dir)] = '/';

  memcpy((filename + strlen(target_dir) + 1), msg->payload, len);

  if ((ret = stat(filename, &buf)) != 0) {
    uint8_t error[9];
    err_response(error);
    write(socket_fd, error, 9);

    free(filename);
    return;
  }

  // Get the file size.
  uint64_t fsize = buf.st_size;

  // Construct the message.
  // uint8_t* resp = malloc(17);
  // resp[0] = msg->header;

  uint64_t paylen = 8;
  // uint64_t paylen_be = htobe64(paylen);
  // memcpy((resp + 1), &paylen_be, 8);
  memcpy(&msg->payload_len, &paylen, 8);

  uint64_t fsize_be = htobe64(fsize);
  // memcpy((resp + 9), &fsize_be, 8);
  msg->payload = realloc(msg->payload, 8 * sizeof(uint8_t));
  memcpy(msg->payload, &fsize_be, 8);

  // write(socket_fd, resp, 17);
  if (compress) {
    echo_response(socket_fd, msg, 1, dict);
  } else {
    echo_response(socket_fd, msg, 0, dict);
  }

  // free(resp);
  free(filename);
  return;
}

void retrieve_response(int socket_fd, struct message* msg, char* target_dir,
  struct six_type* payl, int compress, struct bit_code* dict) {

  struct stat buf;
  char* filepath = malloc(strlen(target_dir) + 1 + strlen(payl->data) + 1);

  memcpy(filepath, target_dir, strlen(target_dir));
  filepath[strlen(target_dir)] = '/';

  memcpy((filepath + strlen(target_dir) + 1), payl->data, (strlen(payl->data) + 1));

  FILE* fp = fopen(filepath, "rb");
  stat(filepath, &buf);

  uint64_t data_lenbe = htobe64(payl->data_len);
  uint64_t offset_be = htobe64(payl->offset);
  // printf("Data length in big endian: %lx\n", data_lenbe);
  // printf("Offset in big endian: %lx\n", offset_be);

  if (fp == NULL || offset_be > buf.st_size || (data_lenbe + offset_be) > buf.st_size) {
    uint8_t error[9];
    err_response(error);
    write(socket_fd, error, 9);

    free(filepath);
    return;
  }

  // Create a response.
  // uint8_t* resp = malloc(29 + data_lenbe);
  msg->payload = realloc(msg->payload, 20 + data_lenbe);

  // Seek the file to the offset, given by uint8_t* offset.
  fseek(fp, offset_be, SEEK_SET);

  // resp[0] = 0x70;
  uint64_t paylen_be = 20 + data_lenbe;
  // Convert to host byte order so that client receives in network order.
  // uint64_t paylen = be64toh(paylen_be);
  // printf("Pay length in big endian: %lx\n", paylen);
  // memcpy((resp + 1), &paylen, 8);
  memcpy(&msg->payload_len, &paylen_be, 8);

  // memcpy((resp + 9), &payl->session_id, 4);
  // memcpy((resp + 13), &payl->offset, 8);
  // memcpy((resp + 21), &payl->data_len, 8);
  memcpy(msg->payload, &payl->session_id, 4);
  memcpy((msg->payload + 4), &payl->offset, 8);
  memcpy((msg->payload + 12), &payl->data_len, 8);

  // Read the contents of the file into the response.
  // fread((resp + 29), data_lenbe, 1, fp);
  fread((msg->payload + 20), data_lenbe, 1, fp);

  // write(socket_fd, resp, 29 + data_lenbe);
  if (compress) {
    echo_response(socket_fd, msg, 1, dict);
  } else {
    echo_response(socket_fd, msg, 0, dict);
  }

  free(payl->data);
  free(filepath);
  // free(resp);
  return;
}
