#undef NDEBUG

#include <assert.h>
#include <stdlib.h>

#include <cjson/cJSON.h>

#include "addon.h"
#include "list.h"
#include "osstring.h"

static void test_addon_dup(void)
{
    Addon *expect = addon_create();
    ADDON_SET_NAME(expect, (const char *)"Test");
    ADDON_SET_DESC(expect, (const char *)"Test Desc");
    ADDON_SET_URI(expect, (const char *)"test_uri");
    ADDON_SET_VERSION(expect, (const char *)"v1.2.3");
    list_insert(expect->dirs, strdup("dont_copy_me"));

    Addon *actual = addon_dup(expect);

    assert(actual != expect);
    assert(strcmp(actual->name, "Test") == 0);
    assert(strcmp(actual->desc, "Test Desc") == 0);
    assert(strcmp(actual->uri, "test_uri") == 0);
    assert(strcmp(actual->version, "v1.2.3") == 0);
    assert(list_isempty(actual->dirs));

    addon_destroy(expect);
    addon_destroy(actual);
}

static void test_addon_from_json(void)
{
    cJSON *json = cJSON_CreateObject();
    assert(json != NULL);

    Addon *actual = addon_create();

    assert(cJSON_AddStringToObject(json, ADDON_KEY_NAME, "test_name") != NULL);
    assert(cJSON_AddStringToObject(json, ADDON_KEY_DESC, "test_desc") != NULL);
    assert(cJSON_AddStringToObject(json, ADDON_KEY_URI, "test_uri") != NULL);
    assert(cJSON_AddStringToObject(json, ADDON_KEY_VERSION, "test_version") != NULL);

    cJSON *dirs = cJSON_AddArrayToObject(json, "dirs");
    assert(dirs != NULL);
    cJSON_AddItemToArray(dirs, cJSON_CreateString("dir_1"));
    cJSON_AddItemToArray(dirs, cJSON_CreateString("dir_2"));

    addon_from_json(actual, json);

    assert(strcmp(actual->name, "test_name") == 0);
    assert(strcmp(actual->desc, "test_desc") == 0);
    assert(strcmp(actual->uri, "test_uri") == 0);
    assert(strcmp(actual->version, "test_version") == 0);

    ListNode *node = NULL;
    node = actual->dirs->head;
    assert(strcmp("dir_2", node->value) == 0);
    node = node->next;
    assert(strcmp("dir_1", node->value) == 0);

    cJSON_Delete(json);
    addon_destroy(actual);
}

static void test_addon_from_json_partial(void)
{
    cJSON *json = cJSON_CreateObject();
    assert(json != NULL);

    Addon *actual = addon_create();

    ADDON_SET_VERSION(actual, (const char *)"should not be changed");

    assert(cJSON_AddStringToObject(json, ADDON_KEY_NAME, "test_name") != NULL);
    assert(cJSON_AddStringToObject(json, ADDON_KEY_DESC, "test_desc") != NULL);

    addon_from_json(actual, json);

    assert(strcmp(actual->version, "should not be changed") == 0);
    assert(strcmp(actual->name, "test_name") == 0);
    assert(strcmp(actual->desc, "test_desc") == 0);
    assert(actual->uri == NULL);
    assert(actual->dirs != NULL);
    assert(list_isempty(actual->dirs));

    cJSON_Delete(json);
    addon_destroy(actual);
}

static void test_addon_from_json_overwrite(void)
{
    cJSON *json = cJSON_CreateObject();
    assert(json != NULL);

    Addon *actual = addon_create();

    ADDON_SET_NAME(actual, (const char *)"test_name_overwrite");

    assert(cJSON_AddStringToObject(json, ADDON_KEY_NAME, "test_name") != NULL);

    addon_from_json(actual, json);

    assert(strcmp(actual->name, "test_name") == 0);

    cJSON_Delete(json);
    addon_destroy(actual);
}

static void test_addon_to_json(void)
{
    Addon *addon = addon_create();

    ADDON_SET_NAME(addon, (const char *)"test name");
    ADDON_SET_DESC(addon, (const char *)"test desc");
    ADDON_SET_URI(addon, (const char *)"test uri");
    ADDON_SET_VERSION(addon, (const char *)"test version");
    list_insert(addon->dirs, strdup("dirs_2"));
    list_insert(addon->dirs, strdup("dirs_1"));

    char *json_str = addon_to_json(addon);
    assert(json_str != NULL);

    cJSON *json = cJSON_Parse(json_str);
    assert(json != NULL);

    cJSON *actual = NULL;

    actual = cJSON_GetObjectItemCaseSensitive(json, ADDON_KEY_NAME);
    assert(strcmp(actual->valuestring, "test name") == 0);

    actual = cJSON_GetObjectItemCaseSensitive(json, ADDON_KEY_DESC);
    assert(strcmp(actual->valuestring, "test desc") == 0);

    actual = cJSON_GetObjectItemCaseSensitive(json, ADDON_KEY_URI);
    assert(strcmp(actual->valuestring, "test uri") == 0);

    actual = cJSON_GetObjectItemCaseSensitive(json, ADDON_KEY_VERSION);
    assert(strcmp(actual->valuestring, "test version") == 0);

    cJSON *item = NULL;
    actual = cJSON_GetObjectItemCaseSensitive(json, ADDON_KEY_DIRS);
    item = cJSON_GetArrayItem(actual, 0);
    assert(strcmp(item->valuestring, "dirs_1") == 0);
    item = cJSON_GetArrayItem(actual, 1);
    assert(strcmp(item->valuestring, "dirs_2") == 0);

    free(json_str);
    cJSON_Delete(json);
    addon_destroy(addon);
}

int main(void)
{
    test_addon_dup();
    test_addon_from_json();
    test_addon_from_json_partial();
    test_addon_from_json_overwrite();
    test_addon_to_json();

    return 0;
}
