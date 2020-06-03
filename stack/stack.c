#include "stack.h"

struct stack* stack_init() {
  struct stack* st = malloc(sizeof(*st));
  st->front = -1;
  st->cap = 3;
  st->array = malloc(st->cap * sizeof(*(st->array)));

  return st;
}

int is_empty(struct stack* st) {
  return st->front < 0;
}

int near_full(struct stack* st) {
  return st->front == st->cap - 1;
}

void push(struct stack* st, void* item) {
  if (near_full(st)) {
    st->cap += 2;
    st->array = realloc(st->array, st->cap * sizeof(*(st->array)));
  }

  st->front += 1;
  st->array[st->front] = item;

  return;
}

void* pop(struct stack* st) {
  if (is_empty(st)) {
    return NULL;
  }

  void* item = st->array[st->front];
  st->front -= 1;

  return item;
}

void destroy_stack(struct stack* st) {
  free(st->array);
  free(st);
}
