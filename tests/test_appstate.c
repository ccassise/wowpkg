#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "addon.h"
#include "appstate.h"
#include "list.h"
#include "osstring.h"

static const char *const json_input = "{\n"
                                      "\"installed\": [\n"
                                      "    {\n"
                                      "        \"name\": \"test_name_installed_one\",\n"
                                      "        \"desc\": \"test_desc_installed_one\",\n"
                                      "        \"url\": \"test_url_installed_one\",\n"
                                      "        \"version\": \"test_version_installed_one\",\n"
                                      "        \"dirs\": [ \"test_dirs_installed_one_1\", \"test_dirs_installed_one_2\" ]\n"
                                      "    },\n"
                                      "    {\n"
                                      "        \"name\": \"test_name_installed_two\",\n"
                                      "        \"desc\": \"test_desc_installed_two\",\n"
                                      "        \"url\": \"test_url_installed_two\",\n"
                                      "        \"version\": \"test_version_installed_two\",\n"
                                      "        \"dirs\": []\n"
                                      "    }\n"
                                      "],\n"
                                      "\"latest\": [\n"
                                      "    {\n"
                                      "        \"name\": \"test_name_latest_one\",\n"
                                      "        \"desc\": \"test_desc_latest_one\",\n"
                                      "        \"url\": \"test_url_latest_one\",\n"
                                      "        \"version\": \"test_version_latest_one\",\n"
                                      "        \"dirs\": [ \"test_dirs_latest_one_1\" ]\n"
                                      "    },\n"
                                      "    {\n"
                                      "        \"name\": \"test_name_latest_two\",\n"
                                      "        \"desc\": \"test_desc_latest_two\",\n"
                                      "        \"url\": \"test_url_latest_two\",\n"
                                      "        \"version\": \"test_version_latest_two\",\n"
                                      "        \"dirs\": []\n"
                                      "    }\n"
                                      "]\n"
                                      "}\n";

static void test_appstate_from_json(void)
{
    AppState *state = appstate_create();

    assert(appstate_from_json(state, json_input) == 0);

    ListNode *node = NULL;
    Addon *addon = NULL;
    ListNode *dir = NULL;

    node = state->installed->head;
    addon = node->value;

    assert(strcmp(addon->name, "test_name_installed_two") == 0);
    assert(strcmp(addon->desc, "test_desc_installed_two") == 0);
    assert(strcmp(addon->url, "test_url_installed_two") == 0);
    assert(strcmp(addon->version, "test_version_installed_two") == 0);
    dir = addon->dirs->head;
    assert(dir == NULL);

    node = node->next;
    addon = node->value;

    assert(strcmp(addon->name, "test_name_installed_one") == 0);
    assert(strcmp(addon->desc, "test_desc_installed_one") == 0);
    assert(strcmp(addon->url, "test_url_installed_one") == 0);
    assert(strcmp(addon->version, "test_version_installed_one") == 0);
    dir = addon->dirs->head;
    assert(strcmp(dir->value, "test_dirs_installed_one_2") == 0);
    dir = dir->next;
    assert(strcmp(dir->value, "test_dirs_installed_one_1") == 0);
    dir = dir->next;
    assert(dir == NULL);

    assert(node->next == NULL);

    node = state->latest->head;
    addon = node->value;

    assert(strcmp(addon->name, "test_name_latest_two") == 0);
    assert(strcmp(addon->desc, "test_desc_latest_two") == 0);
    assert(strcmp(addon->url, "test_url_latest_two") == 0);
    assert(strcmp(addon->version, "test_version_latest_two") == 0);
    dir = addon->dirs->head;
    assert(dir == NULL);

    node = node->next;
    addon = node->value;

    assert(strcmp(addon->name, "test_name_latest_one") == 0);
    assert(strcmp(addon->desc, "test_desc_latest_one") == 0);
    assert(strcmp(addon->url, "test_url_latest_one") == 0);
    assert(strcmp(addon->version, "test_version_latest_one") == 0);
    dir = addon->dirs->head;
    assert(strcmp(dir->value, "test_dirs_latest_one_1") == 0);
    dir = dir->next;
    assert(dir == NULL);

    assert(node->next == NULL);

    appstate_destroy(state);
}

static void test_appstate_to_json(void)
{
    AppState *state = appstate_create();

    Addon *installed1 = addon_create();
    Addon *installed2 = addon_create();
    Addon *latest1 = addon_create();
    Addon *latest2 = addon_create();

    installed1->name = strdup("test_name_installed_one");
    installed1->desc = strdup("test_desc_installed_one");
    installed1->url = strdup("test_url_installed_one");
    installed1->version = strdup("test_version_installed_one");
    list_insert(installed1->dirs, strdup("test_dirs_installed_one_2"));
    list_insert(installed1->dirs, strdup("test_dirs_installed_one_1"));

    installed2->name = strdup("test_name_installed_two");
    installed2->desc = strdup("test_desc_installed_two");
    installed2->url = strdup("test_url_installed_two");
    installed2->version = strdup("test_version_installed_two");

    latest1->name = strdup("test_name_latest_one");
    latest1->desc = strdup("test_desc_latest_one");
    latest1->url = strdup("test_url_latest_one");
    latest1->version = strdup("test_version_latest_one");
    list_insert(latest1->dirs, strdup("test_dirs_latest_one_1"));

    latest2->name = strdup("test_name_latest_two");
    latest2->desc = strdup("test_desc_latest_two");
    latest2->url = strdup("test_url_latest_two");
    latest2->version = strdup("test_version_latest_two");

    /* Transfer ownership of Addons to state. */
    list_insert(state->installed, installed2);
    list_insert(state->installed, installed1);
    list_insert(state->latest, latest2);
    list_insert(state->latest, latest1);

    char *json_str = appstate_to_json(state);
    assert(json_str != NULL);

    cJSON *json = cJSON_Parse(json_str);

    cJSON *installed = cJSON_GetObjectItemCaseSensitive(json, "installed");
    assert(installed != NULL);

    cJSON *latest = cJSON_GetObjectItemCaseSensitive(json, "latest");
    assert(latest != NULL);

    Addon *addon = NULL;
    cJSON *addon_json = NULL;
    ListNode *node = NULL;

    addon_json = cJSON_GetArrayItem(installed, 0);
    assert(addon_json != NULL);
    addon = addon_create();
    addon_from_json(addon, addon_json);
    assert(strcmp(addon->name, "test_name_installed_one") == 0);
    assert(strcmp(addon->desc, "test_desc_installed_one") == 0);
    assert(strcmp(addon->url, "test_url_installed_one") == 0);
    assert(strcmp(addon->version, "test_version_installed_one") == 0);
    node = addon->dirs->head;
    assert(strcmp(node->value, "test_dirs_installed_one_2") == 0);
    node = node->next;
    assert(strcmp(node->value, "test_dirs_installed_one_1") == 0);
    node = node->next;
    assert(node == NULL);
    addon_destroy(addon);

    addon_json = cJSON_GetArrayItem(installed, 1);
    assert(addon_json != NULL);
    addon = addon_create();
    addon_from_json(addon, addon_json);
    assert(strcmp(addon->name, "test_name_installed_two") == 0);
    assert(strcmp(addon->desc, "test_desc_installed_two") == 0);
    assert(strcmp(addon->url, "test_url_installed_two") == 0);
    assert(strcmp(addon->version, "test_version_installed_two") == 0);
    node = addon->dirs->head;
    assert(node == NULL);
    addon_destroy(addon);

    addon_json = cJSON_GetArrayItem(latest, 0);
    assert(addon_json != NULL);
    addon = addon_create();
    addon_from_json(addon, addon_json);
    assert(strcmp(addon->name, "test_name_latest_one") == 0);
    assert(strcmp(addon->desc, "test_desc_latest_one") == 0);
    assert(strcmp(addon->url, "test_url_latest_one") == 0);
    assert(strcmp(addon->version, "test_version_latest_one") == 0);
    node = addon->dirs->head;
    assert(strcmp(node->value, "test_dirs_latest_one_1") == 0);
    node = node->next;
    assert(node == NULL);
    addon_destroy(addon);

    addon_json = cJSON_GetArrayItem(latest, 1);
    assert(addon_json != NULL);
    addon = addon_create();
    addon_from_json(addon, addon_json);
    assert(strcmp(addon->name, "test_name_latest_two") == 0);
    assert(strcmp(addon->desc, "test_desc_latest_two") == 0);
    assert(strcmp(addon->url, "test_url_latest_two") == 0);
    assert(strcmp(addon->version, "test_version_latest_two") == 0);
    node = addon->dirs->head;
    assert(node == NULL);
    addon_destroy(addon);

    free(json_str);
    cJSON_Delete(json);
    appstate_destroy(state);
}

static void test_appstate_save_load(void)
{
    const char *test_path = "__test_appstate_save_load__.json";
    remove(test_path);

    AppState *state = appstate_create();
    assert(state != NULL);

    Addon *installed = addon_create();
    Addon *latest = addon_create();

    installed->name = strdup("Installed");
    installed->desc = strdup("Installed desc");
    installed->version = strdup("v1.2.3");
    installed->url = strdup("installed_url");
    list_insert(installed->dirs, strdup("Installed"));
    list_insert(installed->dirs, strdup("Installed_Core"));
    list_insert(installed->dirs, strdup("Installed_Plugins"));

    latest->name = strdup("Latest");
    latest->desc = strdup("Latest desc");
    latest->version = strdup("v4.5.6");
    latest->url = strdup("latest_url");
    list_insert(latest->dirs, strdup("Latest"));

    list_insert(state->installed, installed);
    list_insert(state->latest, latest);

    assert(appstate_save(state, test_path) == 0);

    AppState *actual = appstate_create();

    assert(appstate_load(actual, test_path) == 0);

    assert(actual->installed->head != NULL);

    Addon *installed_actual = actual->installed->head->value;
    assert(strcmp(installed_actual->name, "Installed") == 0);
    assert(strcmp(installed_actual->desc, "Installed desc") == 0);
    assert(strcmp(installed_actual->version, "v1.2.3") == 0);
    assert(strcmp(installed_actual->url, "installed_url") == 0);
    assert(installed_actual->dirs->head != NULL);
    ListNode *installed_dir = installed_actual->dirs->head;
    assert(installed_dir != NULL);
    assert(strcmp((const char *)installed_dir->value, "Installed") == 0);
    installed_dir = installed_dir->next;
    assert(installed_dir != NULL);
    assert(strcmp((const char *)installed_dir->value, "Installed_Core") == 0);
    installed_dir = installed_dir->next;
    assert(installed_dir != NULL);
    assert(strcmp((const char *)installed_dir->value, "Installed_Plugins") == 0);

    Addon *latest_actual = actual->latest->head->value;
    assert(strcmp(latest_actual->name, "Latest") == 0);
    assert(strcmp(latest_actual->desc, "Latest desc") == 0);
    assert(strcmp(latest_actual->version, "v4.5.6") == 0);
    assert(strcmp(latest_actual->url, "latest_url") == 0);
    assert(latest_actual->dirs->head != NULL);
    ListNode *latest_dir = latest_actual->dirs->head;
    assert(latest_dir != NULL);
    assert(strcmp((const char *)latest_dir->value, "Latest") == 0);

    appstate_destroy(state);
    appstate_destroy(actual);
}

int main(void)
{
    test_appstate_from_json();
    test_appstate_to_json();
    test_appstate_save_load();

    return 0;
}
