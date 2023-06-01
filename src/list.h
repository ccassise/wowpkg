#pragma once

#include <stdbool.h>

typedef struct ListNode ListNode;
typedef struct ListNode {
    ListNode *next;
    ListNode *prev;
    void *value;
} ListNode;

typedef struct List {
    ListNode *head;
    void (*free)(void *);
} List;

#define list_foreach(n, l) for ((n) = (n) == NULL ? (l)->head : (n); (n); (n) = (n)->next)

#define list_set_free_fn(l, fn) ((l)->free = (fn))

List *list_create(void);

void list_free(List *l);

ListNode *list_insert(List *l, void *value);

void list_remove(List *l, ListNode *node);

ListNode *list_search(List *l, const void *value, bool (*compare_fn)(const void *a, const void *b));
ListNode *list_search_ptr(List *l, const void *value);
