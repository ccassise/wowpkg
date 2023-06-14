#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "list.h"
#include "osstring.h"

static void test_list_insert(void)
{
    List *l = list_create();

    int values[] = { 0, 1, 2 };

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

    char str1[] = "test one";
    char str2[] = "test two";
    char str3[] = "test three";
    char str4[] = "test four";
    char str5[] = "test five";

    ListNode *node1 = list_insert(l, str1);
    ListNode *node2 = list_insert(l, str2);
    ListNode *node3 = list_insert(l, str3);
    ListNode *node4 = list_insert(l, str4);
    ListNode *node5 = list_insert(l, str5);

    list_remove(l, node3);

    ListNode *actual = l->head;

    assert(strcmp((const char *)actual->value, str5) == 0);
    actual = actual->next;
    assert(strcmp((const char *)actual->value, str4) == 0);
    actual = actual->next;
    assert(strcmp((const char *)actual->value, str2) == 0);
    actual = actual->next;
    assert(strcmp((const char *)actual->value, str1) == 0);
    actual = actual->next;

    assert(actual == NULL);

    list_remove(l, node1);

    actual = l->head;
    assert(strcmp((const char *)actual->value, str5) == 0);
    actual = actual->next;
    assert(strcmp((const char *)actual->value, str4) == 0);
    actual = actual->next;
    assert(strcmp((const char *)actual->value, str2) == 0);
    actual = actual->next;

    assert(actual == NULL);

    list_remove(l, node5);

    actual = l->head;
    assert(strcmp((const char *)actual->value, str4) == 0);
    actual = actual->next;
    assert(strcmp((const char *)actual->value, str2) == 0);
    actual = actual->next;

    assert(actual == NULL);

    list_remove(l, node4);
    list_remove(l, node2);

    assert(list_isempty(l));

    list_free(l);
}

static void test_list_search(void)
{
    List *l = list_create();

    char str1[] = "test one";
    char str2[] = "test two";
    char str3[] = "test three";
    char str4[] = "test four";
    char str5[] = "test five";

    list_insert(l, str1);
    list_insert(l, str2);
    list_insert(l, str3);
    list_insert(l, str4);
    list_insert(l, str5);

    ListNode *node = list_search(l, "test four", (ListCompareFn)strcmp);
    assert(node != NULL && strcmp((const char *)node->value, str4) == 0);

    node = list_search_ptr(l, str2);
    assert(node != NULL && strcmp((const char *)node->value, str2) == 0);

    const char *should_not_find = "this should not be found";

    node = list_search(l, should_not_find, (ListCompareFn)strcmp);
    assert(node == NULL);

    node = list_search_ptr(l, should_not_find);
    assert(node == NULL);

    list_free(l);
}

static void test_list_foreach(void)
{
    List *l = list_create();

    char str[][256] = {
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

    list_free(l);
}

static void test_list_free_fn(void)
{
    List *l = list_create();
    list_set_free_fn(l, free);

    char *str1 = strdup("test one");
    char *str2 = strdup("test two");
    char *str3 = strdup("test three");
    char *str4 = strdup("test four");
    char *str5 = strdup("test five");

    list_insert(l, str1);
    list_insert(l, str2);
    list_insert(l, str3);
    list_insert(l, str4);
    list_insert(l, str5);

    list_free(l);
}

static int intcmp(const void *a, const void *b)
{
    const int *ap = a;
    const int *bp = b;

    return *ap - *bp;
}

static void test_list_sort(void)
{
    List *l = list_create();

    int values[] = { 4, 3, 1, 0, 4, 5, 9, 2 };
    int values_sorted[] = { 0, 1, 2, 3, 4, 4, 5, 9 };

    list_insert(l, &values[0]);
    list_insert(l, &values[1]);
    list_insert(l, &values[2]);
    list_insert(l, &values[3]);
    list_insert(l, &values[4]);
    list_insert(l, &values[5]);
    list_insert(l, &values[6]);
    list_insert(l, &values[7]);

    list_sort(l, intcmp);

    size_t i = 0;
    ListNode *node = NULL;
    list_foreach(node, l)
    {
        assert(*(int *)node->value == values_sorted[i]);
        i++;
    }

    assert(i == 8);

    list_free(l);
}

int main(void)
{
    test_list_insert();
    test_list_remove();
    test_list_search();
    test_list_foreach();
    test_list_free_fn();
    test_list_sort();

    return 0;
}
