#include <stdio.h>
#include <stdlib.h>

// #include <time.h>

#include "command.h"
#include "context.h"
#include "osapi.h"
#include "osstring.h"
#include "wowpkg.h"

/**
 * Determines if '.exe' should be appended to the program's name when trying to
 * search for the executable.
 */
#ifdef _WIN32
#define USE_EXE_EXT 1
#else
#define USE_EXE_EXT 0
#endif

/**
 * Attempts to save app state if err is 0. Otherwise does not attempt to write to disk.
 *
 * Returns -1 if the save fails, otherwise returns err.
 */
static int try_save_state(Context *ctx, int err)
{
    if (err == 0) {
        if (appstate_save(ctx->state, WOWPKG_SAVED_FILE_PATH) != 0) {
            fprintf(stderr, "Error: failed to save addon data\n");
            fprintf(stderr, "Error: this should never happen\n");
            fprintf(stderr, "Error: it is possible the saved addon data is no\n");
            fprintf(stderr, "Error: longer in sync with the WoW addon directory\n");
            fprintf(stderr, "Error: \n");
            fprintf(stderr, "Error: re-running the last command may fix this\n");
            return -1;
        }
    }

    return err;
}

/**
 * Attempts to change the directory to the path that contains the running
 * executable.
 *
 * First, attempts to get the directory name of the passed in string and if
 * successful tries to change to its directory. If that fails it will search
 * PATH env variable for the path that has a base name that matches the passed
 * in string.
 *
 * The passed in string may be modified.
 *
 * Return -1 on error, 0 otherwise.
 */
static int chdir_to_executable_path(const char *str)
{
    int err = 0;
    char *tofree = strdup(str);

    // Find the last separator in path.
    size_t valid_sep_len = strlen(OS_VALID_SEPARATORS);
    char *last_sep = NULL;
    for (char *s = tofree; *s; s++) {
        for (size_t i = 0; i < valid_sep_len; i++) {
            if (*s == OS_VALID_SEPARATORS[i]) {
                last_sep = s;
            }
        }
    }

    if (last_sep != NULL) {
        // The string contains a separator so it is either a relative or
        // absolute path. Try to change into its directory name.
        char tmp = *last_sep;
        *last_sep = '\0';

        err = os_chdir(tofree);
        *last_sep = tmp;

        goto cleanup;
    }

    free(tofree);
    tofree = NULL;

    // At this point the string is not a path, so search PATH environment for
    // the running executable.
    const char *path_env = getenv("PATH");
    if (path_env == NULL) {
        return -1;
    }

    char *path = NULL;
    char *paths = strdup(path_env);
    tofree = paths;

    char exec_path[OS_MAX_PATH];

    path = strtok(paths, OS_PATH_ENV_DELIMITER);
    while (path != NULL) {
        int n = snprintf(exec_path, ARRAY_SIZE(exec_path), "%s%c%s%s", path, OS_SEPARATOR, str, USE_EXE_EXT ? ".exe" : "");
        if (n < 0 || (size_t)n >= ARRAY_SIZE(exec_path)) {
            err = -1;
            goto cleanup;
        }

        struct os_stat s;
        if (os_stat(exec_path, &s) == 0 && S_ISREG(s.st_mode)) {
            err = os_chdir(path);
            if (err == 0) {
                break;
            }
        }

        path = strtok(NULL, OS_PATH_ENV_DELIMITER);
    }

cleanup:
    free(tofree);
    return err;
}

int main(int argc, const char *argv[])
{
    // clock_t start = clock();

    if (argc <= 1) {
        fprintf(stderr, "Usage: wowpkg COMMAND [ARGS...]\n");
        exit(1);
    }

    if (chdir_to_executable_path(argv[0]) != 0) {
        fprintf(stderr, "Error: could not find program executable path\n");
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

    int err = 0;

    if (config_load(ctx.config, WOWPKG_CONFIG_FILE_PATH) != 0) {
        fprintf(stderr, "Error: failed to load user config file\n");
        fprintf(stderr, "Error: ensure file exists and has valid entries\n");

        err = -1;
        goto cleanup;
    }

    // Test that addon path actually exists and is a directory.
    struct os_stat s;
    if (os_stat(ctx.config->addons_path, &s) != 0 || !S_ISDIR(s.st_mode)) {
        fprintf(stderr, "Error: addons path from config file does not exist or\n");
        fprintf(stderr, "Error: is not a directory\n");

        err = -1;
        goto cleanup;
    }

    // Assuming that since the config file was found with valid data that it
    // should be safe to create a new saved file in the expected location.
    if (appstate_load(ctx.state, WOWPKG_SAVED_FILE_PATH) != 0) {
        fprintf(stderr, "Warning: could not find any saved addon data\n");
        fprintf(stderr, "Warning: \n");
        fprintf(stderr, "Warning: if this is the first time running the program\n");
        fprintf(stderr, "Warning: then this message can safely be ignored\n\n");

        if (try_save_state(&ctx, 0) != 0) {
            err = -1;
            goto cleanup;
        }
    }

    if (strcasecmp(argv[1], "info") == 0) {
        err = cmd_info(&ctx, argc - 1, &argv[1], stdout);
    } else if (strcasecmp(argv[1], "install") == 0) {
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

cleanup:
    config_free(ctx.config);
    appstate_free(ctx.state);

    // clock_t end = clock();
    // printf("Complete in: %.2f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    return err < 0 ? 1 : err;
}
