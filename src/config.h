#pragma once

typedef struct Config {
    char *addons_path;
    char *github_token;
} Config;

Config *config_create(void);

/**
 * Destroys config.
 *
 * Passing a NULL pointer will make this function return immediately with no
 * action.
 */
void config_free(Config *cfg);

int config_load(Config *cfg, const char *path);
