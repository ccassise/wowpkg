#include <stdio.h>
#include <stdlib.h>

#include <cjson/cJSON.h>

#include "config.h"
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

void config_free(Config *restrict cfg)
{
    free(cfg->addon_path);
    free(cfg);
}

int config_from_json(Config *restrict cfg, const char *restrict json_str)
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

char *config_to_json(Config *restrict cfg)
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

int config_load(Config *restrict cfg, const char *restrict path)
{
    int err = 0;
    FILE *f = NULL;
    char *buf = NULL;

    f = fopen(path, "rb");
    if (f == NULL) {
        err = -1;
        goto end;
    }

    struct os_stat s;
    if (os_stat(path, &s) != 0) {
        err = -1;
        goto end;
    }

    if (s.st_size < 0) {
        err = -1;
        goto end;
    }
    size_t bufsz = (size_t)s.st_size;
    buf = malloc(sizeof(*buf) * bufsz + 1);

    if (fread(buf, sizeof(*buf), bufsz, f) != bufsz) {
        err = -1;
        goto end;
    }

    buf[bufsz] = '\0';

    if (config_from_json(cfg, buf) != 0) {
        err = -1;
        goto end;
    }

end:
    if (f != NULL) {
        fclose(f);
    }

    if (buf != NULL) {
        free(buf);
    }

    return err;
}
