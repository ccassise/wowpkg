#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <cJSON/cJSON.h>

#include "addon.h"

static void test_addon_from_json(void)
{
    cJSON *json = cJSON_CreateObject();
    assert(json != NULL);

    Addon actual;
    memset(&actual, 0, sizeof(actual));

    assert(cJSON_AddStringToObject(json, ADDON_HANDLER, "test_handler") != NULL);
    assert(cJSON_AddStringToObject(json, ADDON_NAME, "test_name") != NULL);
    assert(cJSON_AddStringToObject(json, ADDON_DESC, "test_desc") != NULL);
    assert(cJSON_AddStringToObject(json, ADDON_URL, "test_url") != NULL);
    assert(cJSON_AddStringToObject(json, ADDON_VERSION, "test_version") != NULL);

    cJSON *dirs = cJSON_AddArrayToObject(json, "dirs");
    assert(dirs != NULL);
    cJSON_AddItemToArray(dirs, cJSON_CreateString("dir_1"));
    cJSON_AddItemToArray(dirs, cJSON_CreateString("dir_2"));

    addon_from_json(&actual, json);

    assert(strcmp(actual.handler, "test_handler") == 0);
    assert(strcmp(actual.name, "test_name") == 0);
    assert(strcmp(actual.desc, "test_desc") == 0);
    assert(strcmp(actual.url, "test_url") == 0);
    assert(strcmp(actual.version, "test_version") == 0);

    ListNode *node = NULL;
    node = actual.dirs->head;
    assert(strcmp("dir_2", node->value) == 0);
    node = node->next;
    assert(strcmp("dir_1", node->value) == 0);

    cJSON_Delete(json);
    addon_free(&actual);
}

static void test_addon_from_json_partial(void)
{
    cJSON *json = cJSON_CreateObject();
    assert(json != NULL);

    Addon actual;
    memset(&actual, 0, sizeof(actual));

    actual.handler = _strdup("should not be changed");

    assert(cJSON_AddStringToObject(json, ADDON_NAME, "test_name") != NULL);
    assert(cJSON_AddStringToObject(json, ADDON_DESC, "test_desc") != NULL);

    addon_from_json(&actual, json);

    assert(strcmp(actual.handler, "should not be changed") == 0);
    assert(strcmp(actual.name, "test_name") == 0);
    assert(strcmp(actual.desc, "test_desc") == 0);
    assert(actual.url == NULL);
    assert(actual.version == NULL);
    assert(actual.dirs == NULL);

    cJSON_Delete(json);
    addon_free(&actual);
}

static void test_addon_from_json_overwrite(void)
{
    cJSON *json = cJSON_CreateObject();
    assert(json != NULL);

    Addon actual;
    memset(&actual, 0, sizeof(actual));

    actual.name = _strdup("test_name_overwrite");

    assert(cJSON_AddStringToObject(json, ADDON_NAME, "test_name") != NULL);

    addon_from_json(&actual, json);

    assert(strcmp(actual.name, "test_name") == 0);

    cJSON_Delete(json);
    addon_free(&actual);
}

static void test_addon_to_json(void)
{
    Addon addon;
    memset(&addon, 0, sizeof(addon));

    addon.handler = "test handler";
    addon.name = "test name";
    addon.desc = "test desc";
    addon.url = "test url";
    addon.version = "test version";
    addon.dirs = list_create();
    list_insert(addon.dirs, "dirs_2");
    list_insert(addon.dirs, "dirs_1");

    char *json_str = addon_to_json(&addon);
    assert(json_str != NULL);

    cJSON *json = cJSON_Parse(json_str);
    assert(json != NULL);

    cJSON *actual = NULL;

    actual = cJSON_GetObjectItemCaseSensitive(json, ADDON_HANDLER);
    assert(strcmp(actual->valuestring, "test handler") == 0);

    actual = cJSON_GetObjectItemCaseSensitive(json, ADDON_NAME);
    assert(strcmp(actual->valuestring, "test name") == 0);

    actual = cJSON_GetObjectItemCaseSensitive(json, ADDON_DESC);
    assert(strcmp(actual->valuestring, "test desc") == 0);

    actual = cJSON_GetObjectItemCaseSensitive(json, ADDON_URL);
    assert(strcmp(actual->valuestring, "test url") == 0);

    actual = cJSON_GetObjectItemCaseSensitive(json, ADDON_VERSION);
    assert(strcmp(actual->valuestring, "test version") == 0);

    cJSON *item = NULL;
    actual = cJSON_GetObjectItemCaseSensitive(json, ADDON_DIRS);
    item = cJSON_GetArrayItem(actual, 0);
    assert(strcmp(item->valuestring, "dirs_1") == 0);
    item = cJSON_GetArrayItem(actual, 1);
    assert(strcmp(item->valuestring, "dirs_2") == 0);

    free(json_str);
    cJSON_Delete(json);
    list_free(addon.dirs);
}

int main(void)
{
    test_addon_from_json();
    test_addon_from_json_partial();
    test_addon_from_json_overwrite();
    test_addon_to_json();

    return 0;
}
