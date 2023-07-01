#pragma once

#include <stdio.h>

#include "context.h"

int cmd_install(Context *restrict ctx, int argc, const char *restrict argv[], FILE *out);

int cmd_help(Context *restrict ctx, int argc, const char *restrict argv[], FILE *out);

int cmd_list(Context *restrict ctx, int argc, const char *restrict argv[], FILE *out);

int cmd_outdated(Context *restrict ctx, int argc, const char *restrict argv[], FILE *out);

int cmd_remove(Context *restrict ctx, int argc, const char *restrict argv[], FILE *out);

int cmd_search(Context *restrict ctx, int argc, const char *restrict argv[], FILE *out);

int cmd_update(Context *restrict ctx, int argc, const char *restrict argv[], FILE *out);

int cmd_upgrade(Context *restrict ctx, int argc, const char *restrict argv[], FILE *out);
