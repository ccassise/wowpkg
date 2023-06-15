#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "command.h"
#include "context.h"
#include "osstring.h"

/**
 * Attempts to save app state if err is 0. Otherwise does not attempt to write to disk.
 *
 * Returns -1 if the save fails, otherwise returns err.
 */
static int try_save_state(Context *ctx, const char *appname, int err)
{
    if (err == 0) {
        if (appstate_save(ctx->state, "../../dev_only/state.json") != 0) {
            fprintf(stderr, "error: %s failed to save app state file\n", appname);
            return -1;
        }
    }

    return err;
}

int main(int argc, const char *argv[])
{
    clock_t start = clock();

    if (argc <= 1) {
        printf("PRINT USAGE\n");
        exit(1);
    }

    Context ctx;
    memset(&ctx, 0, sizeof(ctx));

    ctx.config = config_create();
    ctx.state = appstate_create();
    if (ctx.config == NULL || ctx.state == NULL) {
        fprintf(stderr, "error: %s out of memory\n", argv[0]);
        exit(1);
    }

    ctx.config->addon_path = strdup("../../dev_only/addons");

    int err = 0;

    if (appstate_load(ctx.state, "../../dev_only/state.json") != 0) {
        fprintf(stderr, "error: %s failed to load state.json\n", argv[0]);
        err = -1;
        goto end;
    }

    if (strcasecmp(argv[1], "install") == 0) {
        err = cmd_install(&ctx, argc - 1, &argv[1], stdout);
        err = try_save_state(&ctx, argv[0], err);
    } else if (strcasecmp(argv[1], "list") == 0) {
        err = cmd_list(&ctx, argc - 1, &argv[1], stdout);
    } else if (strcasecmp(argv[1], "outdated") == 0) {
        err = cmd_outdated(&ctx, argc - 1, &argv[1], stdout);
    } else if (strcasecmp(argv[1], "search") == 0) {
        err = cmd_search(&ctx, argc - 1, &argv[1], stdout);
    } else if (strcasecmp(argv[1], "remove") == 0) {
        err = cmd_remove(&ctx, argc - 1, &argv[1], stdout);
        err = try_save_state(&ctx, argv[0], err);
    } else if (strcasecmp(argv[1], "update") == 0) {
        err = cmd_update(&ctx, argc - 1, &argv[1], stdout);
        err = try_save_state(&ctx, argv[0], err);
    } else if (strcasecmp(argv[1], "upgrade") == 0) {
        err = cmd_upgrade(&ctx, argc - 1, &argv[1], stdout);
        err = try_save_state(&ctx, argv[0], err);
    } else {
        fprintf(stderr, "error: %s unknown command '%s'\n", argv[0], argv[1]);
        err = -1;
    }

end:
    config_free(ctx.config);
    appstate_free(ctx.state);

    clock_t end = clock();
    printf("Complete in: %.2f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    return err == 0 ? 0 : 1;
}
