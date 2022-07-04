#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  // expression
  char expr[65536];
  // last time's val
  uint32_t old_val;
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

void new_wp(char *expr_arg) {
  if (free_ == NULL) {
    assert(0);
  }
  WP* res = free_;
  free_ = free_->next;
  res->next = head;
  head = res;
  strcpy(head->expr, expr_arg);
  bool success;
  head->old_val = expr(expr_arg, &success);
  if (!success) {
    printf("A syntax error in expression.\n");
  }
}

void free_wp(int NO) {
  // remove this node from the used list
  if (!head) {
    printf("No such watchpoint.\n");
    return;
  }
  WP *dummy = head;
  WP *deleted = NULL;
  if (dummy->NO == NO) {
    deleted = head;
    head = head->next;
  } else {
    while (dummy->next && dummy->next->NO != NO) {
      dummy = dummy->next; 
      assert(dummy);
    }
    deleted = dummy->next;
    if (!deleted) {
      printf("No such watchpoint.\n");
      // It means the given NO doesn't represent any watchpoint.
      return;
    }
    dummy->next = deleted->next; 
  }
  // add this node to the free list
  deleted->next = free_;
  free_ = deleted;
}

/**
 *  Display the info of all watchpoints.
 */
void watchpoints_display() {
  printf("Num     What\n");
  WP* dummy = head;
  while (dummy != NULL) {
    printf("%-8d%-10s\n", dummy->NO, dummy->expr);
    dummy = dummy->next;
  }
  printf("\n");
}

/**
 * Check whether the value of watchpoints has changed.
 */
void check_watchpoints() {
  WP *dummy = head;
  while (dummy != NULL) {
    bool success;
    uint32_t res = expr(dummy->expr, &success);
    if (!success || res != dummy->old_val) {
      nemu_state.state = NEMU_STOP;
      printf("Hardware watchpoint %d: %s\n\n", dummy->NO, dummy->expr);
      printf("Old value = %u\n", dummy->old_val);
      printf("New value = %u\n", res);
      dummy->old_val = res;
    } 
    dummy = dummy->next;
  } 
}

#endif