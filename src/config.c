#include <stdlib.h>

#include "config.h"
#include "ini.h"
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
