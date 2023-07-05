#include <assert.h>
#include <stdlib.h>

#include <cjson/cJSON.h>

#include "addon.h"
#include "osstring.h"

static void test_addon_dup(void)
{
    Addon *expect = addon_create();
    expect->name = strdup("Test");
    expect->desc = strdup("Test Desc");
    expect->url = strdup("test_url");
    expect->version = strdup("v1.2.3");
    list_insert(expect->dirs, strdup("dont_copy_me"));

    Addon *actual = addon_dup(expect);

    assert(actual != expect);
    assert(strcmp(actual->name, "Test") == 0);
    assert(strcmp(actual->desc, "Test Desc") == 0);
    assert(strcmp(actual->url, "test_url") == 0);
    assert(strcmp(actual->version, "v1.2.3") == 0);
    assert(list_isempty(actual->dirs));

    addon_free(expect);
    addon_free(actual);
}

static void test_addon_from_json(void)
{
    cJSON *json = cJSON_CreateObject();
    assert(json != NULL);

    Addon *actual = addon_create();

    assert(cJSON_AddStringToObject(json, ADDON_NAME, "test_name") != NULL);
    assert(cJSON_AddStringToObject(json, ADDON_DESC, "test_desc") != NULL);
    assert(cJSON_AddStringToObject(json, ADDON_URL, "test_url") != NULL);
    assert(cJSON_AddStringToObject(json, ADDON_VERSION, "test_version") != NULL);

    cJSON *dirs = cJSON_AddArrayToObject(json, "dirs");
    assert(dirs != NULL);
    cJSON_AddItemToArray(dirs, cJSON_CreateString("dir_1"));
    cJSON_AddItemToArray(dirs, cJSON_CreateString("dir_2"));

    addon_from_json(actual, json);

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

    actual->version = strdup("should not be changed");

    assert(cJSON_AddStringToObject(json, ADDON_NAME, "test_name") != NULL);
    assert(cJSON_AddStringToObject(json, ADDON_DESC, "test_desc") != NULL);

    addon_from_json(actual, json);

    assert(strcmp(actual->version, "should not be changed") == 0);
    assert(strcmp(actual->name, "test_name") == 0);
    assert(strcmp(actual->desc, "test_desc") == 0);
    assert(actual->url == NULL);
    assert(actual->dirs != NULL);
    assert(list_isempty(actual->dirs));

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
    Addon *addon = addon_create();

    assert(addon_fetch_catalog_meta(addon, "weakauras") == ADDON_OK);

    assert(strcmp(addon->name, "WeakAuras") == 0);
    assert(strcmp(addon->desc, "A powerful, comprehensive utility for displaying graphics and information based on buffs, debuffs, and other triggers.") == 0);
    assert(strcmp(addon->url, "https://api.github.com/repos/WeakAuras/WeakAuras2/releases/latest") == 0);
    assert(addon->version == NULL);
    assert(addon->dirs != NULL);
    assert(list_isempty(addon->dirs));

    addon_free(addon);
}

int main(void)
{
    test_addon_dup();
    test_addon_from_json();
    test_addon_from_json_partial();
    test_addon_from_json_overwrite();
    test_addon_to_json();
    test_addon_metadata_from_catalog();

    return 0;
}
