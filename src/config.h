#pragma once

typedef struct Config {
    char *addon_path;
} Config;

#define config_free(c) (free((c)->addon_path))

int config_from_json(Config *cfg, const char *json);
char *config_to_json(Config *cfg);
