#pragma once

#include <stdio.h>

struct Context;

int cmd_help(struct Context *ctx, int argc, const char *argv[], FILE *stream);

int cmd_info(struct Context *ctx, int argc, const char *argv[], FILE *stream);

int cmd_install(struct Context *ctx, int argc, const char *argv[], FILE *stream);

int cmd_list(struct Context *ctx, int argc, const char *argv[], FILE *stream);

int cmd_outdated(struct Context *ctx, int argc, const char *argv[], FILE *stream);

int cmd_remove(struct Context *ctx, int argc, const char *argv[], FILE *stream);

int cmd_search(struct Context *ctx, int argc, const char *argv[], FILE *stream);

int cmd_update(struct Context *ctx, int argc, const char *argv[], FILE *stream);

int cmd_upgrade(struct Context *ctx, int argc, const char *argv[], FILE *stream);
