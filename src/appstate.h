#pragma once

typedef struct AppState {
    struct List *installed;
    struct List *latest;
} AppState;

AppState *appstate_create(void);
void appstate_free(AppState *restrict state);

int appstate_from_json(AppState *restrict state, const char *restrict json_str);
char *appstate_to_json(AppState *restrict state);

/**
 * Saves or loads the appstate to/from a given JSON file.
 */
int appstate_save(AppState *restrict state, const char *restrict path);
int appstate_load(AppState *restrict state, const char *restrict path);
