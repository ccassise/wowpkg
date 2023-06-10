#pragma once

typedef struct Config {
    char *addon_path;
} Config;

Config *config_create(void);

int config_from_json(Config *cfg, const char *json);

char *config_to_json(Config *cfg);

void config_free(Config *cfg);
