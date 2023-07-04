#pragma once

#include <stdio.h>

#include "context.h"

int cmd_help(Context *ctx, int argc, const char *argv[], FILE *out);

int cmd_info(Context *ctx, int argc, const char *argv[], FILE *out);

int cmd_install(Context *ctx, int argc, const char *argv[], FILE *out);

int cmd_list(Context *ctx, int argc, const char *argv[], FILE *out);

int cmd_outdated(Context *ctx, int argc, const char *argv[], FILE *out);

int cmd_remove(Context *ctx, int argc, const char *argv[], FILE *out);

int cmd_search(Context *ctx, int argc, const char *argv[], FILE *out);

int cmd_update(Context *ctx, int argc, const char *argv[], FILE *out);

int cmd_upgrade(Context *ctx, int argc, const char *argv[], FILE *out);
