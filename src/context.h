#pragma once

#include "appstate.h"
#include "config.h"

typedef struct Context {
    AppState *state;
    Config *config;
} Context;
