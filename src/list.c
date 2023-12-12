#include <stdbool.h>
#include <stdlib.h>

#include "list.h"

List *list_create(void)
{
    List *result = malloc(sizeof(*result));
    if (result != NULL) {
        result->head = NULL;
        result->free = NULL;
        result->len = 0;
    }

    return result;
}

void list_free(List *l)
{
    if (l == NULL) {
        return;
    }

    ListNode *node = NULL;
    while ((node = l->head) != NULL) {
        list_remove(l, node);
    }

    free(l);
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
        l->len++;
    }

    return node;
}

ListNode *list_search(List *l, const void *value, ListCompareFn cmp)
{
    ListNode *result = NULL;
    list_foreach(result, l)
    {
        if (cmp(value, result->value) == 0) {
            break;
        }
    }

    return result;
}

ListNode *list_search_ptr(List *l, const void *value)
{
    ListNode *result = NULL;
    list_foreach(result, l)
    {
        if (result->value == value) {
            break;
        }
    }

    return result;
}

void list_remove(List *l, ListNode *node)
{
    if (node == NULL) {
        return;
    }

    if (node->prev != NULL) {
        node->prev->next = node->next;
    } else {
        l->head = node->next;
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    }

    if (l->free != NULL) {
        l->free(node->value);
    }

    l->len--;

    free(node);
}

void list_sort(List *l, ListCompareFn cmp)
{
    /* Based on Simon Tatham's Mergesort For Linked List.
     * https://www.chiark.greenend.org.uk/~sgtatham/algorithms/listsort.html */
    size_t insize = 1;

    while (1) {
        ListNode *p = l->head;
        ListNode *q = p;
        ListNode *new_head = NULL;
        ListNode *tail = NULL;
        ListNode *node = NULL;

        size_t nmerges = 0;

        while (p) {
            nmerges++;

            size_t psize = 0;
            size_t qsize = 0;

            for (size_t i = 0; i < insize && q; i++) {
                psize++;
                q = q->next;
            }

            qsize = insize;

            while (psize > 0 || (qsize > 0 && q)) {
                if (psize == 0) {
                    node = q;
                    q = q->next;
                    qsize--;
                } else if (qsize == 0 || !q) {
                    node = p;
                    p = p->next;
                    psize--;
                } else if (cmp(p->value, q->value) <= 0) {
                    node = p;
                    p = p->next;
                    psize--;
                } else {
                    node = q;
                    q = q->next;
                    qsize--;
                }

                if (tail) {
                    tail->next = node;
                } else {
                    new_head = node;
                }

                node->prev = tail;

                tail = node;
            }

            p = q;
        }

        if (tail) {
            tail->next = NULL;
        }

        l->head = new_head;

        if (nmerges <= 1) {
            break;
        }

        insize *= 2;
    }
}
