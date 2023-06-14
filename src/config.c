#include <stdlib.h>

#include <cjson/cJSON.h>

#include "config.h"
#include "osstring.h"

Config *config_create(void)
{
    Config *result = malloc(sizeof(*result));
    if (result) {
        memset(result, 0, sizeof(*result));
    }

    return result;
}

int config_from_json(Config *cfg, const char *json_str)
{
    int err = 0;

    cJSON *json = cJSON_Parse(json_str);
    if (json == NULL) {
        err = -1;
        goto end;
    }

    cJSON *addon_path = cJSON_GetObjectItemCaseSensitive(json, "addon_path");
    if (addon_path == NULL || !cJSON_IsString(addon_path)) {
        err = -1;
        goto end;
    }

    cfg->addon_path = strdup(addon_path->valuestring);

end:
    if (json != NULL) {
        cJSON_Delete(json);
    }

    return err;
}

char *config_to_json(Config *cfg)
{
    int err = 0;
    char *result = NULL;

    cJSON *json = cJSON_CreateObject();
    if (json == NULL) {
        err = -1;
        goto end;
    }

    if (cJSON_AddStringToObject(json, "addon_path", cfg->addon_path) == NULL) {
        err = -1;
        goto end;
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

void config_free(Config *cfg)
{
    free(cfg->addon_path);
    free(cfg);
}
