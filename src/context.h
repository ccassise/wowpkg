#pragma once

struct AppState;
struct Config;
struct CURL;

typedef struct Context {
    struct AppState *state;
    struct Config *config;
    struct CURL *curl;
} Context;
