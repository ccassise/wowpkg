#pragma once

#include <stdbool.h>

typedef int (*ListCompareFn)(const void *, const void *);
typedef void (*ListFreeFn)(void *);

typedef struct ListNode ListNode;
typedef struct ListNode {
    ListNode *next;
    ListNode *prev;
    void *value;
} ListNode;

typedef struct List {
    ListNode *head;
    ListFreeFn free;
} List;

#define list_foreach(n, l) for ((n) = (n) == NULL ? (l)->head : (n); (n); (n) = (n)->next)

#define list_set_free_fn(l, fn) ((l)->free = (fn))

#define list_isempty(l) ((l)->head == NULL)

List *list_create(void);
void list_free(List *restrict l);

/**
 * Creates a new node and inserts it at the beginning of the list with the given
 * value.
 *
 * Returns the newly inserted node. On error returns NULL.
 */
ListNode *list_insert(List *restrict l, void *restrict value);

/**
 * Removes the given node from the list. If node is NULL then the list is
 * unchanged.
 */
void list_remove(List *restrict l, ListNode *restrict node);

/**
 * Search the list for the node that contains the given value.
 *
 * list_search will use the compare_fn in order to compare the given value to
 * the value in the list's nodes.
 *
 * list_search_ptr will search the nodes for the pointer to the node's value
 * that matches the given pointer.
 *
 */
ListNode *list_search(List *restrict l, const void *restrict value, ListCompareFn cmp);
ListNode *list_search_ptr(List *restrict l, const void *restrict value);

/**
 * Sorts the list based off the given compare function. The compare function
 * should return less than 0 if a < b, 0 if a == b, and greater than 0 if a > b.
 *
 * The sort is stable.
 */
void list_sort(List *restrict l, ListCompareFn cmp);
