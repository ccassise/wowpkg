#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>

#include "addon.h"
#include "app_state.h"
#include "list.h"

AppState *appstate_create(void)
{
    AppState *result = malloc(sizeof(*result));
    if (result != NULL) {
        result->installed = list_create();
        result->latest = list_create();

        if (result->installed == NULL || result->latest == NULL) {
            if (result->installed != NULL) {
                list_free(result->installed);
            }

            if (result->latest != NULL) {
                list_free(result->latest);
            }

            free(result);
            result = NULL;
        } else {
            list_set_free_fn(result->installed, (void (*)(void *))addon_free);
            list_set_free_fn(result->latest, (void (*)(void *))addon_free);
        }
    }

    return result;
}

int appstate_from_json(AppState *state, const char *json_str)
{
    int result = 0;

    cJSON *json = cJSON_Parse(json_str);
    if (json == NULL) {
        result = -1;
        goto end;
    }

    cJSON *installed = cJSON_GetObjectItemCaseSensitive(json, "installed");
    if (!cJSON_IsArray(installed)) {
        result = -1;
        goto end;
    }

    cJSON *addon_json = NULL;

    addon_json = NULL;
    cJSON_ArrayForEach(addon_json, installed)
    {
        Addon *addon = addon_create();
        addon_from_json(addon, addon_json);

        list_insert(state->installed, addon);
    }

    cJSON *latest = cJSON_GetObjectItemCaseSensitive(json, "latest");
    if (!cJSON_IsArray(latest)) {
        result = -1;
        goto end;
    }

    addon_json = NULL;
    cJSON_ArrayForEach(addon_json, latest)
    {
        Addon *addon = addon_create();
        addon_from_json(addon, addon_json);

        list_insert(state->latest, addon);
    }

end:
    if (json != NULL) {
        cJSON_Delete(json);
    }

    return result;
}

char *appstate_to_json(AppState *state)
{
    int err = 0;
    char *result = NULL;
    ListNode *node = NULL;

    cJSON *json = cJSON_CreateObject();
    if (json == NULL) {
        err = -1;
        goto end;
    }

    // Installed
    cJSON *installed = cJSON_AddArrayToObject(json, "installed");
    if (json == NULL) {
        err = -1;
        goto end;
    }

    node = NULL;
    list_foreach(node, state->installed)
    {
        char *addon_json_str = addon_to_json((Addon *)node->value);
        if (addon_json_str == NULL) {
            err = -1;
            goto end;
        }

        cJSON *addon_json = cJSON_Parse(addon_json_str);
        if (addon_json == NULL) {
            free(addon_json_str);
            err = -1;
            goto end;
        }

        cJSON_AddItemToArray(installed, addon_json);

        free(addon_json_str);
    }

    // Latest
    cJSON *latest = cJSON_AddArrayToObject(json, "latest");
    if (json == NULL) {
        err = -1;
        goto end;
    }

    node = NULL;
    list_foreach(node, state->latest)
    {
        char *addon_json_str = addon_to_json((Addon *)node->value);
        if (addon_json_str == NULL) {
            err = -1;
            goto end;
        }

        cJSON *addon_json = cJSON_Parse(addon_json_str);
        if (addon_json == NULL) {
            free(addon_json_str);
            err = -1;
            goto end;
        }

        cJSON_AddItemToArray(latest, addon_json);

        free(addon_json_str);
    }

end:
    if (err == 0) {
        result = cJSON_PrintUnformatted(json);
    }

    if (json != NULL) {
        cJSON_Delete(json);
    }

    return result;
}

void appstate_free(AppState *state)
{
    list_free(state->installed);
    list_free(state->latest);
    free(state);
}
