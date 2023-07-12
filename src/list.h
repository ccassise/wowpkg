#pragma once

#include <stdbool.h>

typedef int (*ListCompareFn)(const void *, const void *);
typedef void (*ListFreeFn)(void *);

typedef struct ListNode {
    struct ListNode *next;
    struct ListNode *prev;
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

/**
 * Destroys each node in list, calling ListFreeFn on each node and then destroys
 * the list.
 *
 * Passing a NULL pointer will make this function return immediately with no
 * action.
 */
void list_free(List *l);

/**
 * Creates a new node and inserts it at the beginning of the list with the given
 * value.
 *
 * Returns the newly inserted node. On error returns NULL.
 */
ListNode *list_insert(List *l, void *value);

/**
 * Removes the given node from the list. If node is NULL then the list is
 * unchanged.
 */
void list_remove(List *l, ListNode *node);

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
ListNode *list_search(List *l, const void *value, ListCompareFn cmp);
ListNode *list_search_ptr(List *l, const void *value);

/**
 * Sorts the list based off the given compare function. The compare function
 * should return less than 0 if a < b, 0 if a == b, and greater than 0 if a > b.
 *
 * The sort is stable.
 */
void list_sort(List *l, ListCompareFn cmp);
