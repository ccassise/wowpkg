#include <assert.h>
#include <stdlib.h>

#include "addon.h"
#include "cjson/cjson.h"
#include "osstring.h"

static void test_addon_from_json(void)
{
    cJSON *json = cJSON_CreateObject();
    assert(json != NULL);

    Addon *actual = addon_create();

    assert(cJSON_AddStringToObject(json, ADDON_HANDLER, "test_handler") != NULL);
    assert(cJSON_AddStringToObject(json, ADDON_NAME, "test_name") != NULL);
    assert(cJSON_AddStringToObject(json, ADDON_DESC, "test_desc") != NULL);
    assert(cJSON_AddStringToObject(json, ADDON_URL, "test_url") != NULL);
    assert(cJSON_AddStringToObject(json, ADDON_VERSION, "test_version") != NULL);

    cJSON *dirs = cJSON_AddArrayToObject(json, "dirs");
    assert(dirs != NULL);
    cJSON_AddItemToArray(dirs, cJSON_CreateString("dir_1"));
    cJSON_AddItemToArray(dirs, cJSON_CreateString("dir_2"));

    addon_from_json(actual, json);

    assert(strcmp(actual->handler, "test_handler") == 0);
    assert(strcmp(actual->name, "test_name") == 0);
    assert(strcmp(actual->desc, "test_desc") == 0);
    assert(strcmp(actual->url, "test_url") == 0);
    assert(strcmp(actual->version, "test_version") == 0);

    ListNode *node = NULL;
    node = actual->dirs->head;
    assert(strcmp("dir_2", node->value) == 0);
    node = node->next;
    assert(strcmp("dir_1", node->value) == 0);

    cJSON_Delete(json);
    addon_free(actual);
}

static void test_addon_from_json_partial(void)
{
    cJSON *json = cJSON_CreateObject();
    assert(json != NULL);

    Addon *actual = addon_create();

    actual->handler = strdup("should not be changed");

    assert(cJSON_AddStringToObject(json, ADDON_NAME, "test_name") != NULL);
    assert(cJSON_AddStringToObject(json, ADDON_DESC, "test_desc") != NULL);

    addon_from_json(actual, json);

    assert(strcmp(actual->handler, "should not be changed") == 0);
    assert(strcmp(actual->name, "test_name") == 0);
    assert(strcmp(actual->desc, "test_desc") == 0);
    assert(actual->url == NULL);
    assert(actual->version == NULL);
    assert(actual->dirs != NULL);
    assert(actual->dirs->head == NULL);

    cJSON_Delete(json);
    addon_free(actual);
}

static void test_addon_from_json_overwrite(void)
{
    cJSON *json = cJSON_CreateObject();
    assert(json != NULL);

    Addon *actual = addon_create();

    actual->name = strdup("test_name_overwrite");

    assert(cJSON_AddStringToObject(json, ADDON_NAME, "test_name") != NULL);

    addon_from_json(actual, json);

    assert(strcmp(actual->name, "test_name") == 0);

    cJSON_Delete(json);
    addon_free(actual);
}

static void test_addon_to_json(void)
{
    Addon *addon = addon_create();

    addon->handler = strdup("test handler");
    addon->name = strdup("test name");
    addon->desc = strdup("test desc");
    addon->url = strdup("test url");
    addon->version = strdup("test version");
    list_insert(addon->dirs, strdup("dirs_2"));
    list_insert(addon->dirs, strdup("dirs_1"));

    char *json_str = addon_to_json(addon);
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
    addon_free(addon);
}

void test_addon_metadata_from_catalog(void)
{
    cJSON *json = addon_metadata_from_catalog("weakauras", NULL);
    assert(json != NULL);

    Addon *addon = addon_create();

    assert(addon_from_json(addon, json) == 0);

    assert(strcmp(addon->handler, "github:latest") == 0);
    assert(strcmp(addon->name, "WeakAuras") == 0);
    assert(strcmp(addon->desc, "A powerful, comprehensive utility for displaying graphics and information based on buffs, debuffs, and other triggers.") == 0);
    assert(strcmp(addon->url, "https://api.github.com/repos/WeakAuras/WeakAuras2/releases/latest") == 0);
    assert(addon->version == NULL);
    assert(addon->dirs != NULL);
    assert(addon->dirs->head == NULL);

    addon_free(addon);
    cJSON_Delete(json);
}

int main(void)
{
    test_addon_from_json();
    test_addon_from_json_partial();
    test_addon_from_json_overwrite();
    test_addon_to_json();
    test_addon_metadata_from_catalog();

    return 0;
}
