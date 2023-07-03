#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>

#include "addon.h"
#include "appstate.h"
#include "list.h"
#include "osapi.h"

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
            list_set_free_fn(result->installed, (ListFreeFn)addon_free);
            list_set_free_fn(result->latest, (ListFreeFn)addon_free);
        }
    }

    return result;
}

void appstate_free(AppState *state)
{
    if (state == NULL) {
        return;
    }

    list_free(state->installed);
    list_free(state->latest);
    free(state);
}

int appstate_from_json(AppState *state, const char *json_str)
{
    int err = 0;

    cJSON *json = cJSON_Parse(json_str);
    if (json == NULL) {
        err = -1;
        goto cleanup;
    }

    cJSON *installed = cJSON_GetObjectItemCaseSensitive(json, "installed");
    if (!cJSON_IsArray(installed)) {
        err = -1;
        goto cleanup;
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
        err = -1;
        goto cleanup;
    }

    addon_json = NULL;
    cJSON_ArrayForEach(addon_json, latest)
    {
        Addon *addon = addon_create();
        addon_from_json(addon, addon_json);

        list_insert(state->latest, addon);
    }

cleanup:
    cJSON_Delete(json);

    return err;
}

char *appstate_to_json(AppState *state)
{
    int err = 0;
    char *result = NULL;
    ListNode *node = NULL;

    cJSON *json = cJSON_CreateObject();
    if (json == NULL) {
        err = -1;
        goto cleanup;
    }

    // Installed
    cJSON *installed = cJSON_AddArrayToObject(json, "installed");
    if (json == NULL) {
        err = -1;
        goto cleanup;
    }

    node = NULL;
    list_foreach(node, state->installed)
    {
        char *addon_json_str = addon_to_json((Addon *)node->value);
        if (addon_json_str == NULL) {
            err = -1;
            goto cleanup;
        }

        cJSON *addon_json = cJSON_Parse(addon_json_str);
        if (addon_json == NULL) {
            free(addon_json_str);
            err = -1;
            goto cleanup;
        }

        cJSON_AddItemToArray(installed, addon_json);

        free(addon_json_str);
    }

    // Latest
    cJSON *latest = cJSON_AddArrayToObject(json, "latest");
    if (json == NULL) {
        err = -1;
        goto cleanup;
    }

    node = NULL;
    list_foreach(node, state->latest)
    {
        char *addon_json_str = addon_to_json((Addon *)node->value);
        if (addon_json_str == NULL) {
            err = -1;
            goto cleanup;
        }

        cJSON *addon_json = cJSON_Parse(addon_json_str);
        if (addon_json == NULL) {
            free(addon_json_str);
            err = -1;
            goto cleanup;
        }

        cJSON_AddItemToArray(latest, addon_json);

        free(addon_json_str);
    }

cleanup:
    if (err == 0) {
        result = cJSON_PrintUnformatted(json);
    }

    cJSON_Delete(json);

    return result;
}

int appstate_save(AppState *state, const char *path)
{
    int err = 0;
    FILE *f = NULL;
    char *json_str = NULL;

    f = fopen(path, "wb");
    if (f == NULL) {
        err = -1;
        goto cleanup;
    }

    json_str = appstate_to_json(state);
    if (json_str == NULL) {
        err = -1;
        goto cleanup;
    }

    size_t json_strlen = strlen(json_str);
    if (fwrite(json_str, sizeof(*json_str), json_strlen, f) != json_strlen) {
        err = -1;
        goto cleanup;
    }

cleanup:
    if (f != NULL) {
        fclose(f);
    }

    free(json_str);

    return err;
}

int appstate_load(AppState *state, const char *path)
{
    int err = 0;
    FILE *f = NULL;
    char *buf = NULL;

    f = fopen(path, "rb");
    if (f == NULL) {
        err = -1;
        goto cleanup;
    }

    struct os_stat s;
    if (os_stat(path, &s) != 0) {
        err = -1;
        goto cleanup;
    }

    if (s.st_size < 0) {
        err = -1;
        goto cleanup;
    }
    size_t bufsz = (size_t)s.st_size;
    buf = malloc(sizeof(*buf) * bufsz + 1);

    if (fread(buf, sizeof(*buf), bufsz, f) != bufsz) {
        err = -1;
        goto cleanup;
    }

    buf[bufsz] = '\0';

    if (appstate_from_json(state, buf) != 0) {
        err = -1;
        goto cleanup;
    }

cleanup:
    if (f != NULL) {
        fclose(f);
    }

    free(buf);

    return err;
}
