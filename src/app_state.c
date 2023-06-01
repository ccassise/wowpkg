#include <stdlib.h>
#include <string.h>

#include <cJSON/cJSON.h>

#include "addon.h"
#include "app_state.h"
#include "list.h"

int appstate_from_json(AppState *state, const char *json_str)
{
    state->installed = list_create();
    state->latest = list_create();
    list_set_free_fn(state->installed, addon_free);
    list_set_free_fn(state->latest, addon_free);

    cJSON *json = cJSON_Parse(json_str);
    if (json == NULL) {
        return -1;
    }

    cJSON *installed = cJSON_GetObjectItemCaseSensitive(json, "installed");
    if (!cJSON_IsArray(installed)) {
        return -1;
    }

    cJSON *addon_json = NULL;

    addon_json = NULL;
    cJSON_ArrayForEach(addon_json, installed)
    {
        Addon *addon = malloc(sizeof(*addon));
        memset(addon, 0, sizeof(*addon));
        addon_from_json(addon, addon_json);

        list_insert(state->installed, addon);
    }

    cJSON *latest = cJSON_GetObjectItemCaseSensitive(json, "latest");
    if (!cJSON_IsArray(latest)) {
        return -1;
    }

    addon_json = NULL;
    cJSON_ArrayForEach(addon_json, latest)
    {
        Addon *addon = malloc(sizeof(*addon));
        memset(addon, 0, sizeof(*addon));
        addon_from_json(addon, addon_json);

        list_insert(state->latest, addon);
    }

    return 0;
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
}
