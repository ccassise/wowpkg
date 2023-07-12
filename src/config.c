#include <stdio.h>
#include <stdlib.h>

#include <cjson/cJSON.h>

#include "config.h"
#include "ini.h"
#include "osapi.h"
#include "osstring.h"

Config *config_create(void)
{
    Config *result = malloc(sizeof(*result));
    if (result) {
        memset(result, 0, sizeof(*result));
    }

    return result;
}

void config_free(Config *cfg)
{
    if (cfg == NULL) {
        return;
    }

    free(cfg->addons_path);
    free(cfg);
}

int config_from_json(Config *cfg, const char *json_str)
{
    int err = 0;

    cJSON *json = cJSON_Parse(json_str);
    if (json == NULL) {
        err = -1;
        goto cleanup;
    }

    cJSON *addons_path = cJSON_GetObjectItemCaseSensitive(json, "addons_path");
    if (addons_path == NULL || !cJSON_IsString(addons_path)) {
        err = -1;
        goto cleanup;
    }

    cfg->addons_path = strdup(addons_path->valuestring);

cleanup:
    cJSON_Delete(json);

    return err;
}

char *config_to_json(Config *cfg)
{
    int err = 0;
    char *result = NULL;

    cJSON *json = cJSON_CreateObject();
    if (json == NULL) {
        err = -1;
        goto cleanup;
    }

    if (cJSON_AddStringToObject(json, "addons_path", cfg->addons_path) == NULL) {
        err = -1;
        goto cleanup;
    }

cleanup:
    if (err == 0) {
        result = cJSON_PrintUnformatted(json);
    }

    cJSON_Delete(json);

    return result;
}

int config_load(Config *cfg, const char *path)
{
    INI *ini = ini_open(path);
    if (ini == NULL) {
        return -1;
    }

    int err = 0;

    INIKey *key = NULL;
    while ((key = ini_readkey(ini)) != NULL) {
        if (strcasecmp(key->section, "retail") == 0
            && strcasecmp(key->name, "addons_path") == 0) {

            cfg->addons_path = strdup(key->value);
        }
    }

    if (ini_last_error(ini) != INI_EEOF || cfg->addons_path == NULL) {
        err = -1;
    }

    ini_close(ini);

    return err;
}
