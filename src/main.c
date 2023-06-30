#include <stdio.h>
#include <stdlib.h>
// #include <time.h>

#include "command.h"
#include "context.h"
#include "osapi.h"
#include "osstring.h"

#define STATE_JSON_PATH "../../dev_only/state.json"

/**
 * Attempts to save app state if err is 0. Otherwise does not attempt to write to disk.
 *
 * Returns -1 if the save fails, otherwise returns err.
 */
static int try_save_state(Context *ctx, int err)
{
    if (err == 0) {
        if (appstate_save(ctx->state, STATE_JSON_PATH) != 0) {
            fprintf(stderr, "Error: failed to save managed addon data\n");
            fprintf(stderr, "Error: this should never happen\n");
            fprintf(stderr, "Error: it is possible the managed addon data is no longer in sync with the WoW addon directory\n");
            fprintf(stderr, "Error: re-running the last command max fix this\n");
            return -1;
        }
    }

    return err;
}

int main(int argc, const char *argv[])
{
    // clock_t start = clock();

    if (argc <= 1) {
        fprintf(stderr, "Usage: wowpkg COMMAND [ARGS...]\n");
        exit(1);
    }

    Context ctx;
    memset(&ctx, 0, sizeof(ctx));

    ctx.config = config_create();
    ctx.state = appstate_create();
    if (ctx.config == NULL || ctx.state == NULL) {
        fprintf(stderr, "Error: failed to allocate memory\n");
        exit(1);
    }

    ctx.config->addon_path = strdup("../../dev_only/addons");

    int err = 0;

    if (appstate_load(ctx.state, STATE_JSON_PATH) != 0) {
        char *cwd = os_getcwd(NULL, 0);

        fprintf(stderr, "Error: failed to load managed addon data from %s%c%s\n",
            cwd == NULL ? "." : cwd,
            OS_SEPARATOR,
            STATE_JSON_PATH);
        fprintf(stderr, "Error: ensure file exists, is valid JSON, and satisfies the minimal expected JSON object\n");
        fprintf(stderr, "Error: the minimal expected JSON object is: {\"installed\":[],\"latest\":[]}\n");

        free(cwd);

        err = -1;
        goto end;
    }

    if (strcasecmp(argv[1], "install") == 0) {
        err = cmd_install(&ctx, argc - 1, &argv[1], stdout);
        err = try_save_state(&ctx, err);
    } else if (strcasecmp(argv[1], "list") == 0) {
        err = cmd_list(&ctx, argc - 1, &argv[1], stdout);
    } else if (strcasecmp(argv[1], "outdated") == 0) {
        err = cmd_outdated(&ctx, argc - 1, &argv[1], stdout);
    } else if (strcasecmp(argv[1], "search") == 0) {
        err = cmd_search(&ctx, argc - 1, &argv[1], stdout);
    } else if (strcasecmp(argv[1], "remove") == 0) {
        err = cmd_remove(&ctx, argc - 1, &argv[1], stdout);
        err = try_save_state(&ctx, err);
    } else if (strcasecmp(argv[1], "update") == 0) {
        err = cmd_update(&ctx, argc - 1, &argv[1], stdout);
        err = try_save_state(&ctx, err);
    } else if (strcasecmp(argv[1], "upgrade") == 0) {
        err = cmd_upgrade(&ctx, argc - 1, &argv[1], stdout);
        err = try_save_state(&ctx, err);
    } else if (strcasecmp(argv[1], "help") == 0) {
        err = cmd_help(&ctx, argc - 1, &argv[1], stdout);
    } else {
        fprintf(stderr, "Error: unknown command '%s'\n", argv[1]);
        err = -1;
    }

end:
    config_free(ctx.config);
    appstate_free(ctx.state);

    // clock_t end = clock();
    // printf("Complete in: %.2f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    return err < 0 ? 1 : err;
}
