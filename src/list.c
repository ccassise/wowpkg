#include <stdbool.h>
#include <stdlib.h>

#include "list.h"

List *list_create(void)
{
    List *result = malloc(sizeof(*result));
    if (result != NULL) {
        result->head = NULL;
        result->free = NULL;
    }

    return result;
}

ListNode *list_insert(List *l, void *value)
{
    ListNode *node = malloc(sizeof(*node));
    if (node != NULL) {
        node->value = value;
        node->next = l->head;
        if (l->head != NULL) {
            l->head->prev = node;
        }
        l->head = node;
        node->prev = NULL;
    }

    return node;
}

ListNode *list_search(List *l, const void *value, bool (*compare_fn)(const void *a, const void *b))
{
    ListNode *result = l->head;
    while (result && !compare_fn(value, result->value)) {
        result = result->next;
    }

    return result;
}

ListNode *list_search_ptr(List *l, const void *value)
{
    ListNode *result = l->head;
    while (result && result->value != value) {
        result = result->next;
    }

    return result;
}

void list_remove(List *l, ListNode *node)
{
    if (node->prev != NULL) {
        node->prev->next = node->next;
    } else {
        l->head = node->next;
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    }

    if (l->free != NULL) {
        l->free(node);
    }
}

void list_free(List *l)
{
    ListNode *node = l->head;
    while (node) {
        ListNode *next = node->next;
        if (l->free != NULL) {
            l->free(node->value);
        }
        node = next;
    }

    free(l);
}
