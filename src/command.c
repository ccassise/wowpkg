#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#include "addon.h"
#include "appstate.h"
#include "catalog.h"
#include "command.h"
#include "config.h"
#include "context.h"
#include "list.h"
#include "osapi.h"
#include "osstring.h"
#include "term.h"
#include "wowpkg.h"

// #define CMD_ECREATE_TMP_DIR_STR "failed to create temporary directory"
// #define CMD_EMOVE_STR "failed to move file/directory"
// #define CMD_EOPEN_DIR_STR "failed to open directory"
// #define CMD_EDOWNLOAD_STR "failed to make HTTP request"
#define CMD_EEXTRACT_STR "failed to extract addon"
#define CMD_EINVALID_ARGS_STR "invalid arguments"
#define CMD_EMETADATA_STR "failed to get metadata"
#define CMD_ENAMETOOLONG_STR "name too long"
#define CMD_ENOT_FOUND_STR "could not find addon"
#define CMD_ENO_MEM_STR "memory allocation failed"
#define CMD_EPACKAGE_STR "failed to package addon"
#define CMD_ERATE_LIMIT_STR "rate limit exceeded"
#define CMD_EREMOVE_DIR_STR "failed to remove existing directory"

#define PRINT_ERROR1(error_str) PRINT_ERROR("%s\n", error_str)
#define PRINT_ERROR2(error_str, proc_name) PRINT_ERROR("%s: %s\n", proc_name, error_str)
#define PRINT_ERROR3(error_str, proc_name, name) PRINT_ERROR("%s: %s '%s'\n", proc_name, error_str, name)
#define PRINT_ERROR3_FMT(error_str, proc_name, fmt, ...) PRINT_ERROR("%s: %s '" fmt "'\n", proc_name, error_str, __VA_ARGS__)

#define PRINT_WARNING3(error_str, proc_name, name) PRINT_WARNING("%s: %s '%s'\n", proc_name, error_str, name)

#define PRINT_NO_UPDATED_META_WARNING(proc_name, name)               \
    do {                                                             \
        PRINT_WARNING("%s skipping addon '%s'\n", proc_name, name);  \
        PRINT_WARNING("'%s' has no updated metadata entry\n", name); \
        PRINT_WARNING("this should not happen\n");                   \
        PRINT_WARNING("try running 'update' to fix this problem\n"); \
    } while (0)

#define PRINT_STATUS(stream, ...) fprintf(stream, "==> " __VA_ARGS__)
#define PRINT_STATUS_ADDON(stream, msg, addon) fprintf(stream, "==> " TERM_WRAP(TERM_BOLD, msg) " " TERM_WRAP(TERM_BOLD_BLUE, "%s") "\n", addon)

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
    return strcasecmp((const char *)str, ((const Addon *)addon)->name);
}

int cmd_help(Context *ctx, int argc, const char *argv[], FILE *stream)
{
    UNUSED(ctx);
    UNUSED(argc);
    UNUSED(argv);

    fprintf(stream, "Example usage:\n");
    fprintf(stream, "\t" WOWPKG_NAME " info ADDON...\n");
    fprintf(stream, "\t" WOWPKG_NAME " install ADDON...\n");
    fprintf(stream, "\t" WOWPKG_NAME " list\n");
    fprintf(stream, "\t" WOWPKG_NAME " outdated\n");
    fprintf(stream, "\t" WOWPKG_NAME " remove ADDON...\n");
    fprintf(stream, "\t" WOWPKG_NAME " search TEXT\n");
    fprintf(stream, "\t" WOWPKG_NAME " update [ADDON...]\n");
    fprintf(stream, "\t" WOWPKG_NAME " upgrade [ADDON...]\n");

    return 0;
}

int cmd_info(Context *ctx, int argc, const char *argv[], FILE *stream)
{
    if (argc < 2) {
        PRINT_ERROR1(CMD_EINVALID_ARGS_STR);
        return -1;
    }

    for (int i = 1; i < argc; i++) {
        int err = 0;
        Addon *addon = NULL;

        addon = addon_create();
        if (addon == NULL) {
            PRINT_ERROR2(CMD_ENO_MEM_STR, argv[0]);
            err = -1;
            goto cleanup;
        }

        err = addon_info(addon, ctx, argv[i]);
        if (err != ADDON_OK) {
            if (err == ADDON_ENOTFOUND) {
                PRINT_WARNING3(CMD_ENOT_FOUND_STR, argv[0], argv[i]);
            } else {
                PRINT_ERROR3(CMD_EMETADATA_STR, argv[0], argv[i]);
            }

            err = -1;
            goto cleanup;
        }

        ListNode *installed_node = list_search(ctx->state->installed, addon, cmp_addon);

        int width = 16;
        if (installed_node != NULL) {
            width = 24;
        }
        /* \b removes an extra space. */
        PRINT_STATUS_ADDON(stream, "\b", addon->name);
        fprintf(stream, TERM_WRAP(TERM_BOLD, "%*s") " %s\n", width, "Name:", addon->name);
        fprintf(stream, TERM_WRAP(TERM_BOLD, "%*s") " %s\n", width, "Description:", addon->desc);
        fprintf(stream, TERM_WRAP(TERM_BOLD, "%*s") " %s\n", width, "Uri:", addon->uri);
        fprintf(stream, TERM_WRAP(TERM_BOLD, "%*s") " %s\n", width, "Version:", addon->version);
        fprintf(stream, TERM_WRAP(TERM_BOLD, "%*s"), width, "ZIP:");
        ListNode *asset_node = NULL;
        list_foreach(asset_node, addon->assets)
        {
            AddonAsset *asset = asset_node->value;
            fprintf(stream, " %s", asset->uri);
            if (asset_node->next != NULL) {
                fprintf(stream, ";");
            }
        }
        fprintf(stream, "\n");
        if (installed_node) {
            Addon *installed = installed_node->value;
            fprintf(stream, TERM_WRAP(TERM_BOLD, "%*s") " %s\n", width, "Installed-Version:", installed->version);
            fprintf(stream, TERM_WRAP(TERM_BOLD, "%*s"), width, "Installed-Directories:");
            ListNode *dirs = NULL;
            list_foreach(dirs, installed->dirs)
            {
                const char *dir = dirs->value;
                fprintf(stream, " %s", dir);
                if (dirs->next != NULL) {
                    fprintf(stream, ";");
                }
            }
            fprintf(stream, "\n");
        }

    cleanup:
        addon_destroy(addon);
    }

    return 0;
}

int cmd_install(Context *ctx, int argc, const char *argv[], FILE *stream)
{

    /* TODO: If an addon successfully installs/upgrades but another addon errors, the
     * app state will not be saved for next session even though files may have
     * already been deleted/moved from/to user's addon folder. */
    if (argc < 2) {
        PRINT_ERROR1(CMD_EINVALID_ARGS_STR);
        return -1;
    }

    int err = 0;

    List *addons = list_create();
    if (addons == NULL) {
        PRINT_ERROR2(CMD_ENO_MEM_STR, argv[0]);
        err = -1;
        goto cleanup;
    }

    for (int i = 1; i < argc; i++) {
        PRINT_STATUS_ADDON(stream, "Info", argv[i]);

        Addon *addon = addon_create();
        if (addon == NULL) {
            PRINT_ERROR2(CMD_ENO_MEM_STR, argv[0]);
            err = -1;
            goto cleanup;
        }

        err = addon_info(addon, ctx, argv[i]);
        if (err == ADDON_ENOTFOUND) {
            PRINT_WARNING3(addon_strerror(err), argv[0], argv[i]);
            err = 0;
            goto loop_error;
        } else if (err == ADDON_ERATE_LIMIT) {
            PRINT_ERROR2(addon_strerror(err), argv[i]);
            err = -1;
            goto loop_error;
        } else if (err != ADDON_OK) {
            PRINT_ERROR3(CMD_EMETADATA_STR, argv[0], argv[i]);
            err = -1;
            goto loop_error;
        }

        list_insert(addons, addon);

        continue;
    loop_error:
        addon_destroy(addon);
        if (err != 0) {
            goto cleanup;
        }
    }

    ListNode *node = NULL;
    list_foreach(node, addons)
    {
        Addon *addon = node->value;

        ListNode *found = list_search(ctx->state->installed, addon, cmp_addon);
        if (found) {
            PRINT_STATUS_ADDON(stream, "Found existing addon", addon->name);

            const char *args[2];
            args[0] = argv[0];
            args[1] = addon->name;

            if (cmd_remove(ctx, ARRAY_SIZE(args), args, stream) != 0) {
                PRINT_WARNING("failed to remove existing addon " TERM_WRAP(TERM_BOLD_BLUE, "%s") "\n", addon->name);
                PRINT_WARNING("attempting to reinstall anyway...\n");
            }
        }

        PRINT_STATUS_ADDON(stream, "Fetching", addon->name);
        ListNode *asset_node = NULL;
        list_foreach(asset_node, addon->assets)
        {
            AddonAsset *asset = asset_node->value;
            fprintf(stream, "Download: %s\n", asset->uri);
            if ((err = addon_fetch(addon, ctx, asset)) != ADDON_OK) {
                PRINT_ERROR2(addon_strerror(err), addon->name);
                err = -1;
                goto cleanup;
            }
        }

        PRINT_STATUS_ADDON(stream, "Packaging", addon->name);
        if (addon_package(addon, ctx) != ADDON_OK) {
            PRINT_ERROR3(CMD_EPACKAGE_STR, argv[0], addon->name);
            err = -1;
            goto cleanup;
        }

        PRINT_STATUS_ADDON(stream, "Extracting", addon->name);

        ListNode *string_node = NULL;
        list_foreach(string_node, addon->dirs)
        {
            char *dir = string_node->value;
            printf("Move: %s -> %s\n", dir, ctx->config->addons_path);
        }

        if (addon_extract(addon, ctx, ctx->config->addons_path) != ADDON_OK) {
            PRINT_ERROR3(CMD_EEXTRACT_STR, argv[0], addon->name);
            err = -1;
            goto cleanup;
        }

        // addon_cleanup_files(addon);
    }

    node = NULL;
    list_foreach(node, addons)
    {
        Addon *addon = node->value;

        PRINT_STATUS_ADDON(stream, "Installed addon", addon->name);

        ListNode *n = NULL;

        /* At this point no more errors can occur so we can transfer ownership of
         * each addon without worry. */
        n = list_search(ctx->state->installed, addon, cmp_addon);
        list_remove(ctx->state->installed, n);
        list_insert(ctx->state->installed, addon);

        n = list_search(ctx->state->latest, addon, cmp_addon);
        list_remove(ctx->state->latest, n);
        list_insert(ctx->state->latest, addon_dup(addon));
    }

cleanup:
    /* Addons list should only contain addons that have had their ownership
     * transferred to appstate. However if there was an error then ownership may
     * not have been transferred and need to be cleaned up. */
    if (err != 0) {
        list_set_free_fn(addons, (ListFreeFn)addon_destroy);
    }

    list_destroy(addons);

    return err;
}

int cmd_list(Context *ctx, int argc, const char *argv[], FILE *stream)
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
        fprintf(stream, "%s (%s)\n", addon->name, addon->version);
    }

    return 0;
}

int cmd_outdated(Context *ctx, int argc, const char *argv[], FILE *stream)
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
            fprintf(stream, "%s (%s) < (%s)\n", installed->name, installed->version, latest->version);
        }
    }

    return 0;
}

int cmd_remove(Context *ctx, int argc, const char *argv[], FILE *stream)
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

        PRINT_STATUS_ADDON(stream, "Removing", addon->name);

        ListNode *dirnode = NULL;
        list_foreach(dirnode, addon->dirs)
        {
            const char *dirname = dirnode->value;

            char remove_path[OS_MAX_PATH];
            int n = snprintf(remove_path, ARRAY_SIZE(remove_path), "%s%c%s", ctx->config->addons_path, OS_SEPARATOR, dirname);
            if (n < 0 || (size_t)n >= ARRAY_SIZE(remove_path)) {
                PRINT_ERROR3_FMT(CMD_ENAMETOOLONG_STR, argv[0], "%s%c%s", ctx->config->addons_path, OS_SEPARATOR, dirname);
                return -1;
            }

            fprintf(stream, "Remove: %s\n", remove_path);
            if (os_remove_all(remove_path) != 0) {
                if (errno == ENOENT) {
                    PRINT_WARNING("directory does not exist %s\n", remove_path);
                } else {
                    /* TODO: What should the program do if this error occurs? If
                     * it was successful in removing one or more directories then
                     * the addon directory would now be corrupted. How can it be
                     * recovered? How should the user be notified? Should the
                     * program try to continue? */
                    PRINT_ERROR3(CMD_EREMOVE_DIR_STR, argv[0], dirname);
                    return -1;
                }
            }
        }

        ListNode *n = NULL;

        /* Order here is important. Addon should first be removed from latest
         * because Addon is a reference to an addon in installed. Removing from
         * installed first would cause a double free. */
        n = list_search(ctx->state->latest, addon, cmp_addon);
        list_remove(ctx->state->latest, n);

        n = list_search(ctx->state->installed, addon, cmp_addon);
        list_remove(ctx->state->installed, n);
        addon = NULL; /* Do not use addon from this point. It should be destroyed. */
    }

    return 0;
}

int cmd_search(Context *ctx, int argc, const char *argv[], FILE *stream)
{
    UNUSED(ctx);

    if (argc != 2) {
        PRINT_ERROR1(CMD_EINVALID_ARGS_STR);
        return -1;
    }

    CatalogSearch *cs = catalog_search_begin(WOWPKG_CATALOG_PATH, argv[1]);
    if (cs == NULL) {
        return -1;
    }

    CatalogItem *item = NULL;
    while ((item = catalog_search_inext(cs)) != NULL) {
        fprintf(stream, "%s\n", item->name);
    }

    catalog_search_end(cs);

    return 0;
}

int cmd_update(Context *ctx, int argc, const char *argv[], FILE *stream)
{
    if (argc < 1) {
        PRINT_ERROR1(CMD_EINVALID_ARGS_STR);
        return -1;
    }

    int err = 0;
    List *addons = list_create();
    if (addons == NULL) {
        PRINT_ERROR2(CMD_ENO_MEM_STR, argv[0]);
        return -1;
    }

    if (argc == 1) {
        /* Update all installed addons. */
        ListNode *node = NULL;
        list_foreach(node, ctx->state->installed)
        {
            Addon *a = node->value;
            list_insert(addons, addon_dup(a));
        }
    } else {
        /* Only update the addons that are in args. */
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

        PRINT_STATUS_ADDON(stream, "Info", addon->name);

        err = addon_info(addon, ctx, addon->name);
        if (err == ADDON_ENOTFOUND) {
            PRINT_WARNING3(CMD_ENOT_FOUND_STR, argv[0], addon->name);
            err = 0;
            continue;
        } else if (err == ADDON_ERATE_LIMIT) {
            PRINT_ERROR2(CMD_ERATE_LIMIT_STR, addon->name);
            err = -1;
            continue;
        } else if (err != ADDON_OK) {
            PRINT_ERROR3(CMD_EMETADATA_STR, argv[0], addon->name);
            err = -1;
            goto cleanup;
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

cleanup:
    if (err != 0) {
        list_set_free_fn(addons, (ListFreeFn)addon_destroy);
    }
    list_destroy(addons);

    return err;
}

int cmd_upgrade(Context *ctx, int argc, const char *argv[], FILE *stream)
{
    /* TODO: If an addon successfully installs/upgrades but another addon errors,
     * the app state will not be saved for next session even though files may
     * have already been deleted/moved from/to user's addon folder. */

    if (argc < 1) {
        PRINT_ERROR1(CMD_EINVALID_ARGS_STR);
        return -1;
    }

    int err = 0;
    List *addons = list_create();
    if (addons == NULL) {
        PRINT_ERROR2(CMD_ENO_MEM_STR, argv[0]);
        return -1;
    }

    if (argc == 1) {
        /* Upgrade all outdated addons. */
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
                PRINT_STATUS(stream, TERM_WRAP(TERM_BOLD, "Upgrading ") TERM_WRAP(TERM_BOLD_BLUE, "%s") TERM_WRAP(TERM_BOLD, " (%s) -> (%s)") "\n", installed->name, installed->version, latest->version);
                list_insert(addons, addon_dup(latest));
            }
        }
    } else {
        /* Only upgrade the addons that are in args. */
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
                PRINT_STATUS(stream, TERM_WRAP(TERM_BOLD, "Upgrading ") TERM_WRAP(TERM_BOLD_BLUE, "%s") TERM_WRAP(TERM_BOLD, " (%s) -> (%s)") "\n", installed->name, installed->version, latest->version);
                list_insert(addons, addon_dup(latest));
            } else {
                PRINT_STATUS_ADDON(stream, "Addon is up-to-date", installed->name);
            }
        }
    }

    ListNode *node = NULL;
    list_foreach(node, addons)
    {
        Addon *addon = node->value;

        PRINT_STATUS_ADDON(stream, "Fetching", addon->name);
        ListNode *asset_node = NULL;
        list_foreach(asset_node, addon->assets)
        {
            AddonAsset *asset = asset_node->value;
            fprintf(stream, "Download: %s\n", asset->uri);
            if ((err = addon_fetch(addon, ctx, asset)) != ADDON_OK) {
                PRINT_ERROR2(addon_strerror(err), addon->name);
                err = -1;
                goto cleanup;
            }
        }

        PRINT_STATUS_ADDON(stream, "Packaging", addon->name);
        if (addon_package(addon, ctx) != ADDON_OK) {
            PRINT_ERROR3(CMD_EPACKAGE_STR, argv[0], addon->name);
            err = -1;
            goto cleanup;
        }

        ListNode *found = list_search(ctx->state->installed, addon, cmp_addon);
        if (found) {
            PRINT_STATUS_ADDON(stream, "Cleaning up old addon", addon->name);

            const char *args[2];
            args[0] = argv[0];
            args[1] = addon->name;

            if (cmd_remove(ctx, ARRAY_SIZE(args), args, stream) != 0) {
                PRINT_WARNING("failed to remove old addon " TERM_WRAP(TERM_BOLD_BLUE, "%s") "\n", addon->name);
                PRINT_WARNING("attempting to upgrade anyway...\n");
            }
        }

        PRINT_STATUS_ADDON(stream, "Extracting", addon->name);

        ListNode *string_node = NULL;
        list_foreach(string_node, addon->dirs)
        {
            char *dir = string_node->value;
            printf("Move: %s -> %s\n", dir, ctx->config->addons_path);
        }

        if (addon_extract(addon, ctx, ctx->config->addons_path) != ADDON_OK) {
            PRINT_ERROR3(CMD_EEXTRACT_STR, argv[0], addon->name);
            err = -1;
            goto cleanup;
        }

        // addon_cleanup_files(addon);
    }

    list_foreach(node, addons)
    {
        Addon *addon = node->value;

        PRINT_STATUS_ADDON(stream, "Upgraded addon", addon->name);

        ListNode *n = NULL;

        n = list_search(ctx->state->installed, addon, cmp_addon);
        list_remove(ctx->state->installed, n);
        list_insert(ctx->state->installed, addon);

        n = list_search(ctx->state->latest, addon, cmp_addon);
        list_remove(ctx->state->latest, n);
        list_insert(ctx->state->latest, addon_dup(addon));
    }

cleanup:
    if (err != 0) {
        list_set_free_fn(addons, (ListFreeFn)addon_destroy);
    }

    list_destroy(addons);

    return err;
}
