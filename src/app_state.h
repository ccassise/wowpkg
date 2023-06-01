#pragma once

typedef struct AppState {
    struct List *installed;
    struct List *latest;
} AppState;

int appstate_from_json(AppState *state, const char *json_str);
char *appstate_to_json(AppState *state);

void appstate_free(AppState *state);
