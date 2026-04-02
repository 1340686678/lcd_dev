/*
 * scr_list.c - Screen list helpers for anyui SDK
 *
 * This file is created and owned by anyui.
 *
 * COPYRIGHT 2025 anyui Team
 * All rights reserved.
 *
 * https://anyui.tech/
 *
 * Author: anyui Team
 */
#include "scr_list.h"
#include "mem.h"

typedef struct Node {
  setup_func_t setup;
  struct Node *next;
} Node;

Node *global_head = NULL;

Node *create_node(setup_func_t setup) {
  Node *new_node = (Node *)LV_MEM_ALLOC(sizeof(Node));
  if (new_node == NULL) {
    LV_LOG_ERROR("LV_MEM_ALLOC failed\n");
    return NULL;
  }
  new_node->setup = setup;
  new_node->next = NULL;
  return new_node;
}

void insert_at_head(setup_func_t setup) {
  Node *new_node = create_node(setup);
  if (global_head == NULL) {
    global_head = new_node;
  } else {
    new_node->next = global_head;
    global_head = new_node;
  }
}

Node *find_node(setup_func_t setup) {
  Node *current_scr = global_head;
  while (current_scr != NULL && current_scr->setup != setup) {
    current_scr = current_scr->next;
  }
  return current_scr;
}

void push_scr(setup_func_t setup) { insert_at_head(setup); }

setup_func_t pop_scr() {
  if (global_head == NULL) {
    return NULL;
  }
  setup_func_t setup = global_head->setup;
  Node *temp = global_head;
  global_head = global_head->next;
  LV_MEM_FREE(temp);
  return setup;
}

setup_func_t current_scr() {
  if (global_head == NULL) {
    return NULL;
  } else {
    return global_head->setup;
  }
}

uint32_t count_scr() {
  uint32_t count_scr = 0;
  Node *current_scr = global_head;
  while (current_scr != NULL) {
    count_scr++;
    current_scr = current_scr->next;
  }

  return count_scr;
}
