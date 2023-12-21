#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

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
static int try_save_state(Context *ctx, const char *path, int err)
{
    if (err == 0 && appstate_save(ctx->state, path) != APPSTATE_OK) {
        PRINT_ERROR("failed to save addon data\n");
        PRINT_ERROR("this should never happen\n");
        PRINT_ERROR("it is possible the saved addon data is no\n");
        PRINT_ERROR("longer in sync with the addons directory\n");
        PRINT_ERROR("\n");
        PRINT_ERROR("re-running the last command may fix this\n");
        return -1;
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
 * Return -1 on error, 0 otherwise.
 */
static int chdir_to_executable_path(const char *str)
{
    if (str == NULL) {
        return -1;
    }

    int err = 0;
    char *tofree = strdup(str);

    /* Find the last separator in path. */
    char *last_sep = NULL;
    for (char *s = tofree; *s; s++) {
        if (OS_IS_SEP(*s)) {
            last_sep = s;
        }
    }

    if (last_sep != NULL) {
        /* The string contains a separator so it is either a relative or absolute
         * path. Try to change into its directory name. */
        char tmp = *last_sep;
        *last_sep = '\0';

        err = os_chdir(tofree);
        *last_sep = tmp;

        goto cleanup;
    }

    free(tofree);
    tofree = NULL;

    /* At this point the string is not a path, so search PATH environment for the
     * running executable. */
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

/**
 * Gets a path to files that are per user. Config files, app state files, and
 * any other future per user file.
 *
 * Function has similar semantics as snprintf(3). The buffer is always null
 * terminated.
 *
 * Returns the amount of characters that was wrote or would have been wrote if
 * the buffer were to have enough space.
 */
static int snuser_file_path(char *s, size_t n, const char *filename)
{
#if defined(WOWPKG_USER_FILE_DIR)

    return snprintf(s, n, "%s%c%s", WOWPKG_USER_FILE_DIR, OS_SEPARATOR, filename);
#elif !defined(WOWPKG_USER_FILE_DIR) && defined(_WIN32)

    /* Windows stores the user files in %APPDATA%\wowpkg */
    const char *user_dir = getenv("APPDATA");
    if (user_dir == NULL) {
        PRINT_ERROR("expected to find APPDATA environment variable but it was empty\n");
        return -1;
    }
    return snprintf(s, n, "%s\\%s\\%s", user_dir, WOWPKG_NAME, filename);
#else

    /* macOS and Linux store the user files in $HOME/.config/wowpkg */
    const char *user_dir = getenv("HOME");
    if (user_dir == NULL) {
        PRINT_ERROR("expected to find HOME environment variable but it was empty\n");
        return -1;
    }
    return snprintf(s, n, "%s/%s/%s/%s", user_dir, ".config", WOWPKG_NAME, filename);
#endif
}

/**
 * Creates wowpkg directory in user's path and creates a default config.ini and
 * saved.wowpkg.
 *
 * IMPORTANT: If this function encounters an error, it will attempt to delete
 * all passed in paths. As such, this function should only be called if the
 * user_path does not exist. Otherwise, it will overwrite/remove the user's
 * data.
 */
static int perform_first_time_setup(const char *user_path, const char *saved_file_path, const char *config_file_path)
{
    int err = 0;
    FILE *fsaved = NULL;
    FILE *fconfig = NULL;

    /* Create wowpkg directory in the wowpkg path. */
    if (os_mkdir(user_path, 0755) != 0) {
        PRINT_ERROR("%s\n", strerror(errno));
        err = -1;
        goto cleanup;
    }

    /* Create wowpkg.saved */
    fsaved = fopen(saved_file_path, "w");
    if (fsaved == NULL) {
        PRINT_ERROR("%s\n", strerror(errno));
        err = -1;
        goto cleanup;
    }
    const char saved_default[] = "{}\n";
    if (fwrite(saved_default, sizeof(saved_default) - 1, 1, fsaved) != 1) {
        PRINT_ERROR("failed to perform first time setup\n");
        err = -1;
        goto cleanup;
    }

    /* Create config.ini */
    fconfig = fopen(config_file_path, "w");
    if (fconfig == NULL) {
        PRINT_ERROR("%s\n", strerror(errno));
        err = -1;
    }
    const char config_default[] = "[Config]\n"
                                  "\n"
                                  "[Retail]\n"
                                  "; Absolute path the the World of Warcraft AddOns directory.\n"
                                  ";\n"
                                  "; Windows example:\n"
                                  "addons_path = C:\\Program Files (x86)\\World of Warcraft\\_retail_\\Interface\\AddOns\n"
                                  "\n"
                                  "; macOS example:\n"
                                  "; addons_path = /Applications/World of Warcraft/_retail_/Interface/AddOns\n";
    if (fwrite(config_default, sizeof(config_default) - 1, 1, fconfig) != 1) {
        PRINT_ERROR("failed to perform first time setup\n");
        remove(config_file_path);
        err = -1;
        goto cleanup;
    }

cleanup:
    if (fsaved != NULL) {
        fclose(fsaved);
    }
    if (fconfig != NULL) {
        fclose(fconfig);
    }
    if (err != 0) {
        remove(saved_file_path);
        remove(config_file_path);
        os_rmdir(user_path);
    }
    return err;
}

int main(int argc, const char *argv[])
{
    if (argc <= 1) {
        fprintf(stderr, "Usage: wowpkg COMMAND [ARGS...]\n");
        exit(EXIT_FAILURE);
    }

    if (chdir_to_executable_path(argv[0]) != 0) {
        PRINT_ERROR("could not find program executable path\n");
        exit(EXIT_FAILURE);
    }

    Context ctx;
    memset(&ctx, 0, sizeof(ctx));

    ctx.config = config_create();
    ctx.state = appstate_create();
    if (ctx.config == NULL || ctx.state == NULL) {
        PRINT_ERROR("%s\n", strerror(ENOMEM));
        exit(EXIT_FAILURE);
    }

    int err = 0;
    int n;

    char user_path[OS_MAX_PATH];
    n = snuser_file_path(user_path, ARRAY_SIZE(user_path), "");
    if (n < 0) {
        err = -1;
        goto cleanup;
    } else if ((size_t)n >= ARRAY_SIZE(user_path)) {
        PRINT_ERROR("path to config file is too long\n");
        err = -1;
        goto cleanup;
    }

    char config_file_path[OS_MAX_PATH];
    n = snuser_file_path(config_file_path, ARRAY_SIZE(config_file_path), "config.ini");
    if (n < 0) {
        err = -1;
        goto cleanup;
    } else if ((size_t)n >= ARRAY_SIZE(config_file_path)) {
        PRINT_ERROR("path to config file is too long\n");
        err = -1;
        goto cleanup;
    }

    char saved_file_path[OS_MAX_PATH];
    n = snuser_file_path(saved_file_path, ARRAY_SIZE(saved_file_path), "saved.wowpkg");
    if (n < 0) {
        err = -1;
        goto cleanup;
    } else if ((size_t)n >= ARRAY_SIZE(saved_file_path)) {
        PRINT_ERROR("path to saved addon data file is too long\n");
        err = -1;
        goto cleanup;
    }

    struct os_stat s;
    if (os_stat(user_path, &s) != 0 && errno == ENOENT) {
        PRINT_WARNING("performing first time setup\n");
        if (perform_first_time_setup(user_path, saved_file_path, config_file_path) != 0) {
            err = -1;
            goto cleanup;
        }
    }

    if (config_load(ctx.config, config_file_path) != 0) {
        PRINT_ERROR("failed to load user config file\n");
        PRINT_ERROR("ensure file exists and has valid entries\n");
        err = -1;
        goto cleanup;
    }

    /* Test that addon path actually exists and is a directory. */
    if (os_stat(ctx.config->addons_path, &s) != 0 || !S_ISDIR(s.st_mode)) {
        PRINT_ERROR("addons path from config file does not exist or\n");
        PRINT_ERROR("is not a directory\n");
        err = -1;
        goto cleanup;
    }

    err = appstate_load(ctx.state, saved_file_path);
    if (err == APPSTATE_ENOENT) {
        /* Assuming that since the config file was found with valid data that it
         * should be safe to create a new saved file in the expected location. */
        err = 0;
        PRINT_WARNING("could not find any saved addon data\n");
        PRINT_WARNING("creating new saved addon data\n");
        if (try_save_state(&ctx, saved_file_path, 0) != 0) {
            err = -1;
            goto cleanup;
        }
    } else if (err != APPSTATE_OK) {
        PRINT_ERROR("failed to load saved program data\n");
        PRINT_ERROR("this should never happen\n");
        PRINT_ERROR("saved data is stored in saved.wowpkg\n");
        PRINT_ERROR("the saved program data may be corrupted and needs to be manually fixed\n");
        PRINT_ERROR("or the file can be deleted but will reset all saved data\n");
    }

    if (strcasecmp(argv[1], "info") == 0) {
        err = cmd_info(&ctx, argc - 1, &argv[1], stdout);
    } else if (strcasecmp(argv[1], "install") == 0) {
        err = cmd_install(&ctx, argc - 1, &argv[1], stdout);
        err = try_save_state(&ctx, saved_file_path, err);
    } else if (strcasecmp(argv[1], "list") == 0) {
        err = cmd_list(&ctx, argc - 1, &argv[1], stdout);
    } else if (strcasecmp(argv[1], "outdated") == 0) {
        err = cmd_outdated(&ctx, argc - 1, &argv[1], stdout);
    } else if (strcasecmp(argv[1], "search") == 0) {
        err = cmd_search(&ctx, argc - 1, &argv[1], stdout);
    } else if (strcasecmp(argv[1], "remove") == 0) {
        err = cmd_remove(&ctx, argc - 1, &argv[1], stdout);
        err = try_save_state(&ctx, saved_file_path, err);
    } else if (strcasecmp(argv[1], "update") == 0) {
        err = cmd_update(&ctx, argc - 1, &argv[1], stdout);
        err = try_save_state(&ctx, saved_file_path, err);
    } else if (strcasecmp(argv[1], "upgrade") == 0) {
        err = cmd_upgrade(&ctx, argc - 1, &argv[1], stdout);
        err = try_save_state(&ctx, saved_file_path, err);
    } else if (strcasecmp(argv[1], "help") == 0) {
        err = cmd_help(&ctx, argc - 1, &argv[1], stdout);
    } else {
        PRINT_ERROR("unknown command '%s'\n", argv[1]);
        err = -1;
    }

cleanup:
    config_destroy(ctx.config);
    appstate_destroy(ctx.state);

    return err == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
