#ifndef COMPRESS_H
#define COMPRESS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include "../server.h"
// #include "../stack/stack.h"

struct huffman_tree {
  uint8_t input_byte;
  uint8_t node_id;
  struct huffman_tree* left;
  struct huffman_tree* right;
};

struct bit_code {
  uint8_t length;
  uint8_t bit_code[32];
};

// Read the compression dictionary into a bit array.
uint32_t* read_compress(const char* dict_name, int* file_size);

// Get the bit at the position specified by bit_pos.
int get_bit(uint32_t* bit_arr, int bit_pos, int bit_num);

// Set the bit to 1 at the position specified by bit_pos.
void set_bit(uint8_t* bit_code, int bit_pos);

// Create a huffman tree for decompression.
void create_huffman_tree(struct bit_code* dict);

// Create a dictionary to use for compression.
struct bit_code* create_dict(uint32_t* bit_arr, int* file_size);

// Decompress the message payload.
void decompress_payload(struct message* msg, struct bit_code* dict);

// Compress the message payload.
void compress_payload(struct message* msg, struct bit_code* dict);

#endif
