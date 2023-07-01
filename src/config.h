#pragma once

typedef struct Config {
    char *addon_path;
} Config;

Config *config_create(void);
void config_free(Config *restrict cfg);

int config_from_json(Config *restrict cfg, const char *restrict json);
char *config_to_json(Config *restrict cfg);

int config_load(Config *restrict cfg, const char *restrict path);
