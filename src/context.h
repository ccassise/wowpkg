#pragma once

struct AppState;
struct Config;

typedef struct Context {
    struct AppState *state;
    struct Config *config;
} Context;
