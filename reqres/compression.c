#include "compression.h"

uint32_t* read_compress(const char* dict_name, int* file_size) {
  FILE* fp = fopen(dict_name, "rb");

  if (fp == NULL) {
    perror("Can't open compression dictionary");
    return NULL;
  }

  int bit_arr_cap = 1;
  int bit_indx = 0;
  uint32_t* bit_arr = malloc(sizeof(uint32_t));

  while (!feof(fp)) {
    // Read the bits into the bit array.
    fread((bit_arr + bit_indx), 4, 1, fp);

    // Convert to big endian.
    uint32_t big_end = htobe32(bit_arr[bit_indx]);
    memcpy((bit_arr + bit_indx), &big_end, 4);

    // Increase the index and realloc the array.
    bit_indx += 1;
    bit_arr_cap += 1;
    bit_arr = realloc(bit_arr, bit_arr_cap * sizeof(uint32_t));
  }

  *file_size = ftell(fp);

  return bit_arr;
}

int get_bit(uint32_t* bit_arr, int bit_pos, int bit_num) {
  // Index of the array it's in.
  int arr_indx = bit_pos / bit_num;

  // Get the position of the bit in the array.
  int bit_shift = bit_pos % bit_num;

  unsigned int bit = (1 << (bit_num - 1));

  int mask = bit >> bit_shift;
  // printf("Mask: %x\n", mask);
  // Get the bit at the position.
  // mask = bit_arr[arr_indx] & mask;

  if (bit_arr[arr_indx] & mask) {
    return 1;
  } else {
    return 0;
  }

  // Shift to right to the original.
  // mask = mask >> (31 - bit_shift);

  // return mask;
}

void set_bit(uint8_t* bit_code, int bit_pos) {
  int arr_indx = bit_pos / 8;

  int bit_shift = bit_pos % 8;

  unsigned int bit = (0x80 >> bit_shift);

  bit_code[arr_indx] = bit_code[arr_indx] | bit;
}

void create_huffman_tree(uint32_t* bit_arr) {
  uint32_t length_le = bit_arr[0] & 0xff000000;
  uint8_t length = (uint8_t) htobe32(length_le);
  struct huffman_tree* root = malloc(sizeof(struct huffman_tree));
  int offset = 8;

  while (offset != 44) {
    for (int i = 0; i < length; i++) {
      if (get_bit(bit_arr, offset, 32)) {
        root->input_byte = NULL;
        root->right = malloc(sizeof(struct huffman_tree));
        root = root->right;
      } else {
        root->input_byte = NULL;
        root->left = malloc(sizeof(struct huffman_tree));
        root = root->left;
      }
      offset += 1;
    }

    root->input_byte = malloc(sizeof(uint8_t));
    *(root->input_byte) = 0x00;

    // Reset length.
    length = 0;

    for (int i = 0; i < 8; i++) {
      length = length | (get_bit(bit_arr, offset, 32) << 8) >> (i + 1);
      offset += 1;
    }

    printf("Offset: %d\n", offset);
    printf("Length: %hhx\n", length);
  }

  return;
}

struct bit_code* create_dict(uint32_t* bit_arr, int* file_size) {
  // uint32_t dict[256];
  // memset(dict, 0, 1024);

  struct bit_code* code_arr = malloc(256 * sizeof(struct bit_code));
  memset(code_arr, 0, 256 * sizeof(struct bit_code));
  // uint32_t length = bit_arr[0] & 0xff000000;
  int offset = 0;

  // Convert this to big endian.
  // uint32_t len_be = htobe32(length);

  // Convert to a byte.
  // uint8_t lenbyte_be = (uint8_t) len_be;
  uint8_t lenbyte_be = 0;
  // printf("Length %hhx\n", lenbyte_be);

  for (uint8_t i = 0x00; i < 0x100; i++) {
    // Reset the length.
    lenbyte_be = 0;

    // Get the length of the next bit code.
    for (int i = 0; i < 8; i++) {
      if (get_bit(bit_arr, offset, 32)) {
        lenbyte_be = lenbyte_be | 0x80 >> i;
      }
      offset += 1;
    }

    // printf("Length in loop: %hhx\n", lenbyte_be);
    // printf("%x\n", dict[i]);

    code_arr[i].length = lenbyte_be;

    if (offset >= (*file_size * 8)) {
      break;
    }
    // Read the bit code.
    for (int j = 0; j < lenbyte_be; j++) {
      if (get_bit(bit_arr, offset, 32)) {
        // dict[i] = dict[i] | 0x80000000 >> j;
        set_bit(code_arr[i].bit_code, j);
      }
      offset += 1;
    }

    // printf("Dict %hhx: %x\n", i, dict[i]);
    // printf("Length code %hhx\n", code_arr[i].length);

    // printf("Code arr %hhx: ", i);
    // for (int k = 0; k < ceil(code_arr[i].length / 8.0); k++) {
    //   printf("%hhx", code_arr[i].bit_code[k]);
    // }
    // puts("");
    // printf("\nOffset in loop: %d\n", offset);

  }

  return code_arr;
}

void decompress_payload(struct message* msg, struct bit_code* dict) {
  // TODO
  return;
}

void compress_payload(struct message* msg, struct bit_code* dict) {
  uint64_t compress_len = 0;
  int code_offset = 0;
  int byte_ctr = 0;

  int compress_cap = 1;
  int temp_compcap = 0;
  uint8_t* compress_payl = malloc(sizeof(uint8_t));
  memset(compress_payl, 0, sizeof(uint8_t));
  uint8_t* temp_comppayl = NULL;

  // Get each byte in the payload and get its compression bit code.
  for (int i = 0; i < msg->payload_len; i++) {
    // Get the payload byte.
    uint8_t payl_byte = msg->payload[i];

    // Get the length of its compression bit code.
    uint8_t len = dict[payl_byte].length;

    uint32_t bcode_be = htobe32(*((uint32_t*) dict[payl_byte].bit_code));

    // For each of the bytes of the bit code add to compress_payl.
    for (int k = 0; k < len; k++) {
      // memcpy((compress_payl + compress_len), &dict[payl_byte].bit_code[k], 1);
      if (byte_ctr == 8) {
        compress_len += 1;
        temp_compcap = compress_cap;
        compress_cap += 1;

        temp_comppayl = compress_payl;
        compress_payl = malloc(compress_cap * sizeof(uint8_t));
        memset(compress_payl, 0, compress_cap * sizeof(uint8_t));
        memcpy(compress_payl, temp_comppayl, temp_compcap);
        free(temp_comppayl);

        byte_ctr = 0;
      }

      if (get_bit(&bcode_be, k, 32)) {
        set_bit(compress_payl, code_offset);
      }

      // Resize the compressed payload.
      // compress_cap += 1;
      code_offset += 1;
      byte_ctr += 1;
      // compress_len += 1;
      // compress_payl = realloc(compress_payl, compress_cap * sizeof(uint8_t));
    }

  }

  // Compress length is the capacity of the compressed payload.
  compress_len = compress_cap;

  // Get the bits padding.
  uint8_t padding = (compress_len * 8) - code_offset;

  // Increase length to accommodate the padding.
  compress_len += 1;

  // Copy the new length and compressed bytes into the msg.
  memcpy(&msg->payload_len, &compress_len, 8);
  msg->payload = realloc(msg->payload, compress_len * sizeof(uint8_t));
  memcpy(msg->payload, compress_payl, compress_cap);

  // Copy bit padding length to the end.
  memcpy((msg->payload + (compress_len -1)), &padding, 1);

  free(compress_payl);
  return;
}
