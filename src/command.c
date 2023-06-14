#include <ctype.h>
#include <stdlib.h>

#include <curl/cURL.h>

#include "addon.h"
#include "command.h"
#include "context.h"
#include "list.h"
#include "osapi.h"
#include "osstring.h"

#define UNUSED(a) ((void)(a))
#define ARRLEN(a) (sizeof(a) / sizeof(*(a)))

#define CMD_EMETADATA "failed to get metadata"
#define CMD_ECREATE_TMP_DIR_STR "failed to create temp directory"
#define CMD_EDOWNLOAD_STR "failed to make HTTP request"
#define CMD_EEXTRACT_STR "failed to extract addon"
#define CMD_EINVALID_ARGS_STR "invalid args"
#define CMD_EMOVE_STR "failed to move file/directory"
#define CMD_ENAME_TOO_LONG_STR "name too long"
#define CMD_ENOT_FOUND_STR "could not find addon"
#define CMD_ENO_MEM_STR "memory allocation failed"
#define CMD_EOPEN_DIR_STR "failed to open directory"
#define CMD_EPACKAGE_STR "failed to package addon"
#define CMD_EREMOVE_DIR_STR "failed to remove existing directory"

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

int cmd_install(Context *ctx, int argc, const char *argv[], FILE *out)
{
    if (argc < 2) {
        fprintf(stderr, "error: %s\n", CMD_EINVALID_ARGS_STR);
        return -1;
    }

    int err = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    List *addons = list_create();
    if (addons == NULL) {
        fprintf(stderr, "error: %s %s\n", argv[0], CMD_ENO_MEM_STR);
        err = -1;
        goto end;
    }

    for (size_t i = 1; i < argc; i++) {
        fprintf(out, "==> Fetching %s\n", argv[i]);

        Addon *addon = addon_create();
        if (addon == NULL) {
            fprintf(stderr, "error: %s %s\n", argv[0], CMD_ENO_MEM_STR);
            err = -1;
            goto end;
        }

        err = addon_fetch_all_meta(addon, argv[i]);
        if (err == ADDON_ENOTFOUND) {
            fprintf(stderr, "error: %s '%s'\n", CMD_ENOT_FOUND_STR, argv[i]);
            err = 0;
            goto loop_error;
        } else if (err != ADDON_OK) {
            fprintf(stderr, "error: %s %s '%s'\n", argv[0], CMD_EMETADATA, argv[i]);
            err = -1;
            goto loop_error;
        }

        list_insert(addons, addon);

        fprintf(out, "==> Downloading %s\n", addon->url);
        if (addon_fetch_zip(addon) != ADDON_OK) {
            fprintf(stderr, "error: %s %s '%s'\n", argv[0], CMD_EDOWNLOAD_STR, addon->name);
            err = -1;
            goto loop_error;
        }

        continue;
    loop_error:
        addon_free(addon);
        if (err != 0) {
            goto end;
        }
    }

    // if (list_isempty(addons)) {
    //     err = -1;
    //     goto end;
    // }

    ListNode *node = NULL;
    list_foreach(node, addons)
    {
        Addon *addon = node->value;

        const char *addon_path = "../../dev_only/addons";

        fprintf(out, "==> Packaging %s\n", addon->name);
        if (addon_package(addon) != ADDON_OK) {
            fprintf(stderr, "error: %s %s '%s'\n", argv[0], CMD_EPACKAGE_STR, addon->name);
            err = -1;
            goto end;
        }

        fprintf(out, "==> Extracting %s\n", addon->name);
        if (addon_extract(addon, addon_path) != ADDON_OK) {
            fprintf(stderr, "error: %s %s '%s' (%d)\n", argv[0], CMD_EEXTRACT_STR, addon->name, err);
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
        list_set_free_fn(addons, addon_free);
    }

    list_free(addons);

    curl_global_cleanup();

    return err;
}

int cmd_list(Context *ctx, int argc, const char *argv[], FILE *out)
{
    UNUSED(argv);

    if (argc != 1) {
        fprintf(stderr, "error: %s %s\n", argv[0], CMD_EINVALID_ARGS_STR);
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

// int cmd_outdated(Context *ctx, int argc, const char *argv[], FILE *out);

/**
 * Compares a string a to the Addon b's name.
 */
static int cmp_str_to_addon(const void *str, const void *addon)
{
    const char *s = str;
    const Addon *a = addon;

    return strcasecmp(s, a->name);
}

int cmd_remove(Context *ctx, int argc, const char *argv[], FILE *out)
{
    if (argc <= 1) {
        fprintf(stderr, "error: %s\n", CMD_EINVALID_ARGS_STR);
        return -1;
    }

    for (size_t i = 1; i < argc; i++) {
        ListNode *node = list_search(ctx->state->installed, argv[i], (ListCompareFn)cmp_str_to_addon);
        if (node == NULL) {
            fprintf(stderr, "error: %s '%s'\n", CMD_ENOT_FOUND_STR, argv[i]);
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
            if (n < 0 || n >= ARRLEN(remove_path)) {
                fprintf(stderr, "error: %s %s '%s%c%s'\n", argv[0], CMD_ENAME_TOO_LONG_STR, ctx->config->addon_path, OS_SEPARATOR, dirname);
                // result = -1;
                // goto end;
                return -1;
            }

            if (os_remove_all(remove_path) != 0) {
                // TODO: What should the program do if this error occurs? If it
                // was successful in removing one or more directories then the
                // addon directory would now be corrupted. How can it be
                // recovered? How should the user be notified? Should the
                // program try to continue?
                fprintf(stderr, "error: %s %s '%s'\n", argv[0], CMD_EREMOVE_DIR_STR, dirname);
                // result = -1;
                // goto end;
                return -1;
            }
        }

        fprintf(out, "==> Removed addon %s\n", addon->name);

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

int cmd_search(Context *ctx, int argc, const char *argv[], FILE *out)
{
    UNUSED(ctx);

    if (argc != 2) {
        fprintf(stderr, "error: %s %s\n", argv[0], CMD_EINVALID_ARGS_STR);
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

int cmd_update(Context *ctx, int argc, const char *argv[], FILE *out)
{
    if (argc < 1) {
        fprintf(stderr, "error: %s\n", CMD_EINVALID_ARGS_STR);
        return -1;
    }

    int err = 0;
    List *addons = list_create();
    if (addons == NULL) {
        fprintf(stderr, "error: %s %s\n", argv[0], CMD_ENO_MEM_STR);
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
        for (size_t i = 1; i < argc; i++) {
            ListNode *found = list_search(ctx->state->installed, argv[i], cmp_str_to_addon);
            if (!found) {
                fprintf(stderr, "error: %s '%s'\n", CMD_ENOT_FOUND_STR, argv[i]);
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
            fprintf(stderr, "error: %s '%s'\n", CMD_ENOT_FOUND_STR, addon->name);
            err = 0;
            continue;
        } else if (err != ADDON_OK) {
            fprintf(stderr, "error: %s %s '%s'\n", argv[0], CMD_EMETADATA, addon->name);
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
        list_set_free_fn(addons, addon_free);
    }
    list_free(addons);

    return err;
}

int cmd_upgrade(Context *ctx, int argc, const char *argv[], FILE *out)
{
    if (argc < 1) {
        fprintf(stderr, "error: %s\n", CMD_EINVALID_ARGS_STR);
        return -1;
    }

    int err = 0;
    List *addons = list_create();
    if (addons == NULL) {
        fprintf(stderr, "error: %s %s\n", argv[0], CMD_ENO_MEM_STR);
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
                fprintf(stderr, "warning: %s skipping addon '%s'\n", argv[0], installed->name);
                fprintf(stderr, "warning: '%s' has no updated metadata entry\n", installed->name);
                fprintf(stderr, "warning: this should not happen\n");
                fprintf(stderr, "warning: try running 'update' to fix this problem\n");
                continue;
            }

            Addon *latest = found->value;

            if (strcmp(latest->version, installed->version) != 0) {
                fprintf(out, "==> Upgrading %s\n", installed->name);
                list_insert(addons, addon_dup(latest));
            }
        }
    } else {
        // Only upgrade the addons that are in args.
        for (size_t i = 1; i < argc; i++) {
            ListNode *found_installed = list_search(ctx->state->installed, argv[i], cmp_str_to_addon);
            if (!found_installed) {
                fprintf(stderr, "error: %s '%s'\n", CMD_ENOT_FOUND_STR, argv[i]);
                continue;
            }

            Addon *installed = found_installed->value;

            ListNode *found_latest = list_search(ctx->state->latest, argv[i], cmp_str_to_addon);
            if (found_latest == NULL) {
                fprintf(stderr, "warning: %s skipping addon '%s'\n", argv[0], installed->name);
                fprintf(stderr, "warning: '%s' has no updated metadata entry\n", installed->name);
                fprintf(stderr, "warning: this should not happen\n");
                fprintf(stderr, "warning: try running 'update' to fix this problem\n");
                continue;
            }

            Addon *latest = found_latest->value;

            if (strcmp(latest->version, installed->version) != 0) {
                fprintf(out, "==> Upgrading %s\n", installed->name);
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
            fprintf(stderr, "error: %s %s '%s'\n", argv[0], CMD_EDOWNLOAD_STR, addon->name);
            err = -1;
            goto end;
        }
    }

    node = NULL;
    list_foreach(node, addons)
    {
        Addon *addon = node->value;

        const char *args[2];
        args[0] = argv[0];
        args[1] = addon->name;

        fprintf(out, "==> Packaging %s\n", addon->name);
        if (addon_package(addon) != ADDON_OK) {
            fprintf(stderr, "error: %s %s '%s'\n", argv[0], CMD_EPACKAGE_STR, addon->name);
            err = -1;
            goto end;
        }

        if (cmd_remove(ctx, ARRLEN(args), args, out) != 0) {
            err = -1;
            goto end;
        }

        fprintf(out, "==> Extracting %s\n", addon->name);
        if (addon_extract(addon, ctx->config->addon_path) != ADDON_OK) {
            fprintf(stderr, "error: %s %s '%s' (%d)\n", argv[0], CMD_EEXTRACT_STR, addon->name, err);
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
        list_set_free_fn(addons, addon_free);
    }

    list_free(addons);

    return err;
}
