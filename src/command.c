#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#include <curl/curl.h>

#include "addon.h"
#include "command.h"
#include "context.h"
#include "list.h"
#include "osapi.h"
#include "osstring.h"
#include "wowpkg.h"

#define CMD_EMETADATA_STR "failed to get metadata"
// #define CMD_ECREATE_TMP_DIR_STR "failed to create temp directory"
#define CMD_EDOWNLOAD_STR "failed to make HTTP request"
#define CMD_EEXTRACT_STR "failed to extract addon"
#define CMD_EINVALID_ARGS_STR "invalid args"
// #define CMD_EMOVE_STR "failed to move file/directory"
#define CMD_ENAMETOOLONG_STR "name too long"
#define CMD_ENOT_FOUND_STR "could not find addon"
#define CMD_ENO_MEM_STR "memory allocation failed"
// #define CMD_EOPEN_DIR_STR "failed to open directory"
#define CMD_EPACKAGE_STR "failed to package addon"
#define CMD_EREMOVE_DIR_STR "failed to remove existing directory"

#define PRINT_ERROR(...) fprintf(stderr, "Error: " __VA_ARGS__)

#define PRINT_ERROR1(error_str) PRINT_ERROR("%s\n", error_str)
#define PRINT_ERROR2(error_str, proc_name) PRINT_ERROR("%s: %s\n", proc_name, error_str)
#define PRINT_ERROR3(error_str, proc_name, name) PRINT_ERROR("%s: %s '%s'\n", proc_name, error_str, name)
#define PRINT_ERROR3_FMT(error_str, proc_name, fmt, ...) PRINT_ERROR("%s: %s '" fmt "'\n", proc_name, error_str, __VA_ARGS__)

#define PRINT_WARNING(...) fprintf(stderr, "Warning: " __VA_ARGS__)

#define PRINT_WARNING3(error_str, proc_name, name) PRINT_WARNING("%s: %s '%s'\n", proc_name, error_str, name)

#define PRINT_NO_UPDATED_META_WARNING(proc_name, name)               \
    do {                                                             \
        PRINT_WARNING("%s skipping addon '%s'\n", proc_name, name);  \
        PRINT_WARNING("'%s' has no updated metadata entry\n", name); \
        PRINT_WARNING("this should not happen\n");                   \
        PRINT_WARNING("try running 'update' to fix this problem\n"); \
    } while (0)

/**
 * Searches haystack to see if needle appears in the string, ignoring case.
 *
 * Returns the beginning of the first occurrence of needle in haystack. Returns
 * NULL if needle was not found.
 */
static const char *cmd_strcasestr(const char *haystack, const char *needle)
{
    const char *result = NULL;
    const char *haystackp = haystack;
    const char *needlep = needle;
    bool isstart = true;

    while (*haystackp) {
        if (*needlep == '\0') {
            break;
        }

        if (tolower(*haystackp) == tolower(*needlep)) {
            if (isstart) {
                result = haystackp;
                isstart = false;
            }
            needlep++;
        } else {
            needlep = needle;
            isstart = true;
        }

        haystackp++;
    }

    if (*needlep != '\0') {
        result = NULL;
    }

    return result;
}

static int cmp_addon(const void *a, const void *b)
{
    const Addon *aa = a;
    const Addon *bb = b;

    return strcmp(aa->name, bb->name);
}

/**
 * Compares a string a to the Addon b's name.
 */
static int cmp_str_to_addon(const void *str, const void *addon)
{
    const char *s = str;
    const Addon *a = addon;

    return strcasecmp(s, a->name);
}

int cmd_install(Context *restrict ctx, int argc, const char *restrict argv[], FILE *out)
{
    if (argc < 2) {
        PRINT_ERROR1(CMD_EINVALID_ARGS_STR);
        return -1;
    }

    int err = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    List *addons = list_create();
    if (addons == NULL) {
        PRINT_ERROR2(CMD_ENO_MEM_STR, argv[0]);
        err = -1;
        goto end;
    }

    for (int i = 1; i < argc; i++) {
        fprintf(out, "==> Fetching %s\n", argv[i]);

        Addon *addon = addon_create();
        if (addon == NULL) {
            PRINT_ERROR2(CMD_ENO_MEM_STR, argv[0]);
            err = -1;
            goto end;
        }

        err = addon_fetch_all_meta(addon, argv[i]);
        if (err == ADDON_ENOTFOUND) {
            PRINT_WARNING3(CMD_ENOT_FOUND_STR, argv[0], argv[i]);
            err = 0;
            goto loop_error;
        } else if (err != ADDON_OK) {
            PRINT_ERROR3(CMD_EMETADATA_STR, argv[0], argv[i]);
            err = -1;
            goto loop_error;
        }

        fprintf(out, "==> Downloading %s\n", addon->url);
        if (addon_fetch_zip(addon) != ADDON_OK) {
            PRINT_ERROR3(CMD_EDOWNLOAD_STR, argv[0], addon->name);
            err = -1;
            goto loop_error;
        }

        list_insert(addons, addon);

        continue;
    loop_error:
        addon_free(addon);
        if (err != 0) {
            goto end;
        }
    }

    ListNode *node = NULL;
    list_foreach(node, addons)
    {
        Addon *addon = node->value;

        ListNode *found = list_search(ctx->state->installed, addon, cmp_addon);
        if (found) {
            fprintf(out, "==> Found existing addon %s\n", addon->name);

            const char *args[2];
            args[0] = argv[0];
            args[1] = addon->name;

            if (cmd_remove(ctx, ARRLEN(args), args, out) != 0) {
                PRINT_WARNING("failed to remove existing addon %s\n", addon->name);
                PRINT_WARNING("attempting to reinstall anyway...\n");
            }
        }

        fprintf(out, "==> Packaging %s\n", addon->name);
        if (addon_package(addon) != ADDON_OK) {
            PRINT_ERROR3(CMD_EPACKAGE_STR, argv[0], addon->name);
            err = -1;
            goto end;
        }

        fprintf(out, "==> Extracting %s\n", addon->name);
        if (addon_extract(addon, ctx->config->addon_path) != ADDON_OK) {
            PRINT_ERROR3(CMD_EEXTRACT_STR, argv[0], addon->name);
            err = -1;
            goto end;
        }

        addon_cleanup_files(addon);
    }

    node = NULL;
    list_foreach(node, addons)
    {
        Addon *addon = node->value;

        fprintf(out, "==> Installed addon %s\n", addon->name);

        ListNode *n = NULL;

        // At this point no more errors can occur so we can transfer ownership
        // of each addon without worry.
        n = list_search(ctx->state->installed, addon, cmp_addon);
        list_remove(ctx->state->installed, n);
        list_insert(ctx->state->installed, addon);

        n = list_search(ctx->state->latest, addon, cmp_addon);
        list_remove(ctx->state->latest, n);
        list_insert(ctx->state->latest, addon_dup(addon));
    }

end:
    // Addons list should only contain addons that have had their ownership
    // transferred to appstate. However if there was an error then ownership may
    // not have been transferred and need to be cleaned up.
    if (err != 0) {
        list_set_free_fn(addons, (ListFreeFn)addon_free);
    }

    list_free(addons);

    curl_global_cleanup();

    return err;
}

int cmd_help(Context *restrict ctx, int argc, const char *restrict argv[], FILE *out)
{
    UNUSED(ctx);
    UNUSED(argc);
    UNUSED(argv);

    fprintf(out, "Example usage:\n");
    fprintf(out, "\twowpkg install ADDON...\n");
    fprintf(out, "\twowpkg list\n");
    fprintf(out, "\twowpkg outdated\n");
    fprintf(out, "\twowpkg remove ADDON...\n");
    fprintf(out, "\twowpkg search TEXT\n");
    fprintf(out, "\twowpkg update [ADDON...]\n");
    fprintf(out, "\twowpkg upgrade [ADDON...]\n");

    return 0;
}

int cmd_list(Context *restrict ctx, int argc, const char *restrict argv[], FILE *out)
{
    UNUSED(argv);

    if (argc != 1) {
        PRINT_ERROR1(CMD_EINVALID_ARGS_STR);
        return -1;
    }

    list_sort(ctx->state->installed, cmp_addon);

    ListNode *node = NULL;
    list_foreach(node, ctx->state->installed)
    {
        Addon *addon = node->value;
        fprintf(out, "%s (%s)\n", addon->name, addon->version);
    }

    return 0;
}

int cmd_outdated(Context *restrict ctx, int argc, const char *restrict argv[], FILE *out)
{
    if (argc != 1) {
        PRINT_ERROR1(CMD_EINVALID_ARGS_STR);
        return -1;
    }

    list_sort(ctx->state->installed, cmp_addon);

    ListNode *node = NULL;
    list_foreach(node, ctx->state->installed)
    {
        Addon *installed = node->value;

        ListNode *found = list_search(ctx->state->latest, installed, cmp_addon);
        if (!found) {
            PRINT_NO_UPDATED_META_WARNING(argv[0], installed->name);
            continue;
        }

        Addon *latest = found->value;

        if (strcmp(installed->version, latest->version) != 0) {
            fprintf(out, "%s (%s) < (%s)\n", installed->name, installed->version, latest->version);
        }
    }

    return 0;
}

int cmd_remove(Context *restrict ctx, int argc, const char *restrict argv[], FILE *out)
{
    if (argc <= 1) {
        PRINT_ERROR1(CMD_EINVALID_ARGS_STR);
        return -1;
    }

    for (int i = 1; i < argc; i++) {
        ListNode *node = list_search(ctx->state->installed, argv[i], (ListCompareFn)cmp_str_to_addon);
        if (node == NULL) {
            PRINT_WARNING3(CMD_ENOT_FOUND_STR, argv[0], argv[i]);
            continue;
        }

        Addon *addon = node->value;

        fprintf(out, "==> Removing %s\n", addon->name);

        ListNode *dirnode = NULL;
        list_foreach(dirnode, addon->dirs)
        {
            const char *dirname = dirnode->value;

            char remove_path[OS_MAX_PATH];
            int n = snprintf(remove_path, ARRLEN(remove_path), "%s%c%s", ctx->config->addon_path, OS_SEPARATOR, dirname);
            if (n < 0 || (size_t)n >= ARRLEN(remove_path)) {
                PRINT_ERROR3_FMT(CMD_ENAMETOOLONG_STR, argv[0], "%s%c%s", ctx->config->addon_path, OS_SEPARATOR, dirname);
                return -1;
            }

            fprintf(out, "Remove: %s\n", remove_path);
            if (os_remove_all(remove_path) != 0) {
                if (errno == ENOENT) {
                    PRINT_WARNING("directory does not exist %s\n", remove_path);
                } else {
                    // TODO: What should the program do if this error occurs? If it
                    // was successful in removing one or more directories then the
                    // addon directory would now be corrupted. How can it be
                    // recovered? How should the user be notified? Should the
                    // program try to continue?
                    PRINT_ERROR3(CMD_EREMOVE_DIR_STR, argv[0], dirname);
                    return -1;
                }
            }
        }

        ListNode *n = NULL;

        // Order here is important. Addon should first be removed from latest
        // because Addon is a reference to an addon in installed. Removing from
        // installed first would cause a double free.
        n = list_search(ctx->state->latest, addon, cmp_addon);
        list_remove(ctx->state->latest, n);

        n = list_search(ctx->state->installed, addon, cmp_addon);
        list_remove(ctx->state->installed, n);
        addon = NULL; // Do not use addon from this point. It should be destroyed.
    }

    return 0;
}

int cmd_search(Context *restrict ctx, int argc, const char *restrict argv[], FILE *out)
{
    UNUSED(ctx);

    if (argc != 2) {
        PRINT_ERROR1(CMD_EINVALID_ARGS_STR);
        return -1;
    }

    int err = 0;
    List *found = list_create();
    list_set_free_fn(found, free);
    OsDir *dir = os_opendir("../../catalog");
    if (dir == NULL) {
        err = -1;
        goto end;
    }

    OsDirEnt *entry = NULL;
    while ((entry = os_readdir(dir)) != NULL) {
        if (strcmp(entry->name, ".") == 0 || strcmp(entry->name, "..") == 0) {
            continue;
        }

        if (cmd_strcasestr(entry->name, argv[1]) != NULL) {
            char *basename = strdup(entry->name);
            if (basename == NULL) {
                err = -1;
                goto end;
            }

            char *ext_start = strstr(basename, ".json");
            if (ext_start == NULL) {
                continue;
            }

            *ext_start = '\0';

            list_insert(found, basename);
        }
    }

    list_sort(found, (ListCompareFn)strcmp);

    ListNode *node = NULL;
    list_foreach(node, found)
    {
        fprintf(out, "%s\n", (char *)node->value);
    }

end:
    if (dir != NULL) {
        os_closedir(dir);
    }

    list_free(found);

    return err;
}

int cmd_update(Context *restrict ctx, int argc, const char *restrict argv[], FILE *out)
{
    if (argc < 1) {
        PRINT_ERROR1(CMD_EINVALID_ARGS_STR);
        return -1;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    int err = 0;
    List *addons = list_create();
    if (addons == NULL) {
        PRINT_ERROR2(CMD_ENO_MEM_STR, argv[0]);
        return -1;
    }

    if (argc == 1) {
        // Update all installed addons.
        ListNode *node = NULL;
        list_foreach(node, ctx->state->installed)
        {
            Addon *a = node->value;
            list_insert(addons, addon_dup(a));
        }
    } else {
        // Only update the addons that are in args.
        for (int i = 1; i < argc; i++) {
            ListNode *found = list_search(ctx->state->installed, argv[i], cmp_str_to_addon);
            if (!found) {
                PRINT_WARNING3(CMD_ENOT_FOUND_STR, argv[0], argv[i]);
                continue;
            }

            Addon *found_addon = found->value;
            list_insert(addons, addon_dup(found_addon));
        }
    }

    ListNode *node = NULL;
    list_foreach(node, addons)
    {
        Addon *addon = node->value;

        fprintf(out, "==> Fetching %s\n", addon->name);

        err = addon_fetch_all_meta(addon, addon->name);
        if (err == ADDON_ENOTFOUND) {
            PRINT_WARNING3(CMD_ENOT_FOUND_STR, argv[0], addon->name);
            err = 0;
            continue;
        } else if (err != ADDON_OK) {
            PRINT_ERROR3(CMD_EMETADATA_STR, argv[0], addon->name);
            err = -1;
            goto end;
        }
    }

    list_foreach(node, addons)
    {
        Addon *addon = node->value;

        ListNode *n = NULL;

        n = list_search(ctx->state->latest, addon, cmp_addon);
        list_remove(ctx->state->latest, n);
        list_insert(ctx->state->latest, addon);
    }

end:
    if (err != 0) {
        list_set_free_fn(addons, (ListFreeFn)addon_free);
    }
    list_free(addons);

    curl_global_cleanup();

    return err;
}

int cmd_upgrade(Context *restrict ctx, int argc, const char *restrict argv[], FILE *out)
{
    if (argc < 1) {
        PRINT_ERROR1(CMD_EINVALID_ARGS_STR);
        return -1;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    int err = 0;
    List *addons = list_create();
    if (addons == NULL) {
        PRINT_ERROR2(CMD_ENO_MEM_STR, argv[0]);
        return -1;
    }

    if (argc == 1) {
        // Upgrade all outdated addons.
        ListNode *node = NULL;
        list_foreach(node, ctx->state->installed)
        {
            Addon *installed = node->value;

            ListNode *found = list_search(ctx->state->latest, installed, cmp_addon);
            if (found == NULL) {
                PRINT_NO_UPDATED_META_WARNING(argv[0], installed->name);
                continue;
            }

            Addon *latest = found->value;

            if (strcmp(latest->version, installed->version) != 0) {
                fprintf(out, "==> Upgrading %s (%s) -> (%s)\n", installed->name, installed->version, latest->version);
                list_insert(addons, addon_dup(latest));
            }
        }
    } else {
        // Only upgrade the addons that are in args.
        for (int i = 1; i < argc; i++) {
            ListNode *found_installed = list_search(ctx->state->installed, argv[i], cmp_str_to_addon);
            if (!found_installed) {
                PRINT_WARNING3(CMD_ENOT_FOUND_STR, argv[0], argv[i]);
                continue;
            }

            Addon *installed = found_installed->value;

            ListNode *found_latest = list_search(ctx->state->latest, argv[i], cmp_str_to_addon);
            if (found_latest == NULL) {
                PRINT_NO_UPDATED_META_WARNING(argv[0], installed->name);
                continue;
            }

            Addon *latest = found_latest->value;

            if (strcmp(latest->version, installed->version) != 0) {
                fprintf(out, "==> Upgrading %s (%s) -> (%s)\n", installed->name, installed->version, latest->version);
                list_insert(addons, addon_dup(latest));
            } else {
                fprintf(out, "==> Addon is up-to-date %s\n", installed->name);
            }
        }
    }

    ListNode *node = NULL;
    list_foreach(node, addons)
    {
        Addon *addon = node->value;

        fprintf(out, "==> Downloading %s\n", addon->url);

        if (addon_fetch_zip(addon) != ADDON_OK) {
            PRINT_ERROR3(CMD_EDOWNLOAD_STR, argv[0], addon->name);
            err = -1;
            goto end;
        }
    }

    node = NULL;
    list_foreach(node, addons)
    {
        Addon *addon = node->value;

        ListNode *found = list_search(ctx->state->installed, addon, cmp_addon);
        if (found) {
            fprintf(out, "==> Cleaning up old addon %s\n", addon->name);

            const char *args[2];
            args[0] = argv[0];
            args[1] = addon->name;

            if (cmd_remove(ctx, ARRLEN(args), args, out) != 0) {
                PRINT_WARNING("failed to remove old addon %s\n", addon->name);
                PRINT_WARNING("attempting to upgrade anyway...\n");
            }
        }

        fprintf(out, "==> Packaging %s\n", addon->name);
        if (addon_package(addon) != ADDON_OK) {
            PRINT_ERROR3(CMD_EPACKAGE_STR, argv[0], addon->name);
            err = -1;
            goto end;
        }

        fprintf(out, "==> Extracting %s\n", addon->name);
        if (addon_extract(addon, ctx->config->addon_path) != ADDON_OK) {
            PRINT_ERROR3(CMD_EEXTRACT_STR, argv[0], addon->name);
            err = -1;
            goto end;
        }

        addon_cleanup_files(addon);
    }

    list_foreach(node, addons)
    {
        Addon *addon = node->value;

        fprintf(out, "==> Upgraded addon %s\n", addon->name);

        ListNode *n = NULL;

        n = list_search(ctx->state->installed, addon, cmp_addon);
        list_remove(ctx->state->installed, n);
        list_insert(ctx->state->installed, addon);

        n = list_search(ctx->state->latest, addon, cmp_addon);
        list_remove(ctx->state->latest, n);
        list_insert(ctx->state->latest, addon_dup(addon));
    }

end:
    if (err != 0) {
        list_set_free_fn(addons, (ListFreeFn)addon_free);
    }

    list_free(addons);

    curl_global_cleanup();

    return err;
}
