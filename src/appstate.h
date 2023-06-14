#pragma once

typedef struct AppState {
    struct List *installed;
    struct List *latest;
} AppState;

AppState *appstate_create(void);
void appstate_free(AppState *state);

int appstate_from_json(AppState *state, const char *json_str);
char *appstate_to_json(AppState *state);

/**
 * Saves or loads the appstate to/from a given JSON file.
 */
int appstate_save(AppState *state, const char *path);
int appstate_load(AppState *state, const char *path);
