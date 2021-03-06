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

  // Use the mask to get the bit at bit_pos.
  int mask = bit >> bit_shift;

  if (bit_arr[arr_indx] & mask) {
    return 1;
  } else {
    return 0;
  }

}

void set_bit(uint8_t* bit_code, int bit_pos) {
  // We set the bit in the bit code at bit_pos to 1.
  int arr_indx = bit_pos / 8;

  int bit_shift = bit_pos % 8;

  unsigned int bit = (0x80 >> bit_shift);

  bit_code[arr_indx] = bit_code[arr_indx] | bit;
}

struct huffman_tree* create_huffman_tree(struct bit_code* dict) {
  /** Create a binary tree where for every bit in the bit code, if it's 1 we
  *   set up a right node, and if it's 0 we set up a left node. Then we simply
  *   traverse this tree to decompress the payload. */
  struct huffman_tree* root = malloc(sizeof(struct huffman_tree));
  root->node_id = 0;
  root->internal = 2;
  root->left = NULL;
  root->right = NULL;
  struct huffman_tree* cur = root;

  for (uint16_t i = 0x00; i < 0x100; i++) {
    struct bit_code code = dict[i];
    uint32_t code_32 = htobe32(*((uint32_t*) code.bit_code));

    for (int k = 0; k < code.length; k++) {
      // If the bit is 1, then we put this into the right node.
      if (get_bit(&code_32, k, 32)) {
        if (cur->right == NULL) {
          cur->right = malloc(sizeof(struct huffman_tree));
          cur->right->left = NULL;
          cur->right->right = NULL;
        }

        cur = cur->right;
        cur->internal = 1;
        cur->node_id = 'r';

      } else {
        // Otherwise, we put this in the left node.
        if (cur->left == NULL) {
          cur->left = malloc(sizeof(struct huffman_tree));
          cur->left->left = NULL;
          cur->left->right = NULL;
        }

        cur = cur->left;
        cur->internal = 1;
        cur->node_id = 'l';
      }
    }

    // When we finish reading the bit code, assign the byte to the node.
    cur->input_byte = i;
    cur->internal = 0;

    cur = root;
  }

  return root;
}

void destroy_huffman_tree(struct huffman_tree* root) {
  // Inorder traversal of the tree using one stack data structure.
  struct huffman_tree* cur = root;
  struct stack* st = stack_init();
  struct huffman_tree* temp;

  while (!is_empty(st) || cur != NULL) {
    // Keep pushing left nodes until we get to a null, then we traverse the right nodes.
    if (cur != NULL) {
      push(st, cur);
      cur = cur->left;
    } else {
      // When we get to a leaf node, free this node and traverse on its right node.
      cur = (struct huffman_tree*) pop(st);
      temp = cur;
      cur = cur->right;
      free(temp);
    }
  }

  destroy_stack(st);
  return;
}

struct bit_code* create_dict(uint32_t* bit_arr, int* file_size) {
  // Create a dictionary where we can look up the byte and get the bit code.
  struct bit_code* code_arr = malloc(256 * sizeof(struct bit_code));
  memset(code_arr, 0, 256 * sizeof(struct bit_code));

  int offset = 0;

  uint8_t lenbyte_be = 0;

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

    if (offset >= (*file_size * 8)) {
      break;
    }

    code_arr[i].length = lenbyte_be;

    // Read the bit code.
    for (int j = 0; j < lenbyte_be; j++) {
      if (get_bit(bit_arr, offset, 32)) {
        set_bit(code_arr[i].bit_code, j);
      }
      offset += 1;
    }
  }

  return code_arr;
}

void decompress_payload(struct message* msg, struct huffman_tree* root) {
  // Calculate the total number of bits in the compressed payload (except padding)
  uint64_t compressed_bits = (msg->payload_len - 1) * 8;
  uint8_t padding = msg->payload[msg->payload_len - 1];
  uint64_t total_bits = compressed_bits - padding;

  struct huffman_tree* cur = root;

  // Create the decompressed payload and at the end copy this into msg->payload.
  uint64_t decomp_len = 0;
  int decomp_cap = 1;
  uint8_t* decomp_payl = malloc(sizeof(*decomp_payl));

  uint64_t len = ceil(msg->payload_len / 4.0);

  uint32_t* payload_32 = malloc(len * sizeof(uint32_t));
  memset(payload_32, 0, len * sizeof(uint32_t));
  memcpy(payload_32, msg->payload, msg->payload_len);

  // Use a loop to convert to big endian.
  for (int i = 0; i < ceil(msg->payload_len / 4.0); i++) {
    uint32_t payload_32_be = htobe32(payload_32[i]);
    memcpy((payload_32 + i), &payload_32_be, 4);
  }

  /** Traverse the huffman tree to decompress, if the bit is 1 it is a right node.
  *   If it is 0 we go to the left node. We do this until we hit a non-internal node. */
  for (int i = 0; i < total_bits; i++) {
    if (get_bit(payload_32, i, 32)) {
      cur = cur->right;

      if (!cur->internal) {
        memcpy((decomp_payl + decomp_len), &cur->input_byte, 1);
        decomp_len += 1;

        // Resize the payload.
        decomp_cap += 1;
        decomp_payl = realloc(decomp_payl, decomp_cap * sizeof(*(decomp_payl)));

        cur = root;
      }
    } else {
      cur = cur->left;

      if (!cur->internal) {
        memcpy((decomp_payl + decomp_len), &cur->input_byte, 1);
        decomp_len += 1;

        // Resize the payload.
        decomp_cap += 1;
        decomp_payl = realloc(decomp_payl, decomp_cap * sizeof(*(decomp_payl)));

        cur = root;
      }
    }
  }

  // Copy the decompressed payload into msg->payload.
  msg->payload = realloc(msg->payload, decomp_len * sizeof(*msg->payload));

  memcpy(&msg->payload_len, &decomp_len, 8);
  memcpy(msg->payload, decomp_payl, decomp_len);

  free(payload_32);
  free(decomp_payl);
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

      code_offset += 1;
      byte_ctr += 1;
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
