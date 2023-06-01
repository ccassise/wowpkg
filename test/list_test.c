#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "list.h"

static void test_list_insert(void)
{
    List *l = list_create();

    int values[5] = { 0, 1, 2 };

    list_insert(l, &values[0]);
    list_insert(l, &values[1]);
    list_insert(l, &values[2]);

    ListNode *actual = l->head;

    assert(*(int *)actual->value == values[2]);
    actual = actual->next;
    assert(*(int *)actual->value == values[1]);
    actual = actual->next;
    assert(*(int *)actual->value == values[0]);
    actual = actual->next;

    assert(actual == NULL);

    list_free(l);
}

static void test_list_remove(void)
{
    List *l = list_create();

    char *str1 = "test one";
    char *str2 = "test two";
    char *str3 = "test three";
    char *str4 = "test four";
    char *str5 = "test five";

    ListNode *node1 = list_insert(l, str1);
    ListNode *node2 = list_insert(l, str2);
    ListNode *node3 = list_insert(l, str3);
    ListNode *node4 = list_insert(l, str4);
    ListNode *node5 = list_insert(l, str5);

    list_remove(l, node3);

    ListNode *actual = l->head;

    assert(strcmp((char *)actual->value, str5) == 0);
    actual = actual->next;
    assert(strcmp((char *)actual->value, str4) == 0);
    actual = actual->next;
    assert(strcmp((char *)actual->value, str2) == 0);
    actual = actual->next;
    assert(strcmp((char *)actual->value, str1) == 0);
    actual = actual->next;

    assert(actual == NULL);

    list_remove(l, node1);

    actual = l->head;
    assert(strcmp((char *)actual->value, str5) == 0);
    actual = actual->next;
    assert(strcmp((char *)actual->value, str4) == 0);
    actual = actual->next;
    assert(strcmp((char *)actual->value, str2) == 0);
    actual = actual->next;

    assert(actual == NULL);

    list_remove(l, node5);

    actual = l->head;
    assert(strcmp((char *)actual->value, str4) == 0);
    actual = actual->next;
    assert(strcmp((char *)actual->value, str2) == 0);
    actual = actual->next;

    assert(actual == NULL);

    list_remove(l, node4);
    list_remove(l, node2);

    assert(l->head == NULL);

    list_free(l);
}

static bool str_compare(const void *a, const void *b)
{
    return strcmp((const char *)a, (const char *)b) == 0;
}

static void test_list_search(void)
{
    List *l = list_create();

    char *str1 = "test one";
    char *str2 = "test two";
    char *str3 = "test three";
    char *str4 = "test four";
    char *str5 = "test five";

    list_insert(l, str1);
    list_insert(l, str2);
    list_insert(l, str3);
    list_insert(l, str4);
    list_insert(l, str5);

    ListNode *node = list_search(l, "test four", str_compare);
    assert(node != NULL && strcmp((char *)node->value, str4) == 0);

    node = list_search_ptr(l, str2);
    assert(node != NULL && strcmp((char *)node->value, str2) == 0);

    list_free(l);
}

static void test_list_foreach(void)
{
    List *l = list_create();

    char *str[] = {
        "test one",
        "test two",
        "test three",
    };

    list_insert(l, str[2]);
    list_insert(l, str[1]);
    list_insert(l, str[0]);

    size_t i = 0;
    ListNode *node = NULL;
    list_foreach(node, l)
    {
        assert(strcmp(str[i++], node->value) == 0);
    }

    assert(i == 3);
}

int main(void)
{
    test_list_insert();
    test_list_remove();
    test_list_search();
    test_list_foreach();

    return 0;
}
