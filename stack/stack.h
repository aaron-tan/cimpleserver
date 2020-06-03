#ifndef STACK_H
#define STACK_H
#include <stdlib.h>

struct stack {
  int front;
  int cap;
  void** array;
};

// Stack data structure to be used mainly for destroying the huffman tree.
struct stack* stack_init();

// Check if the stack is empty.
int is_empty(struct stack* st);

// Check if the stack is full.
int is_full(struct stack* st);

// Push an item on to the stack.
void push(struct stack* st, void* item);

// Pop the first item off the stack.
void* pop(struct stack* st);

// Destroy the stack.
void destroy_stack(struct stack* st);

#endif
