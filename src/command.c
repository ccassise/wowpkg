#include <ctype.h>
#include <stdlib.h>

#include "addon.h"
#include "app_state.h"
#include "command.h"
#include "list.h"
#include "osapi.h"
#include "osstring.h"

#define UNUSED(a) ((void)(a))

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

// int cmd_install(Context *ctx, int argc, const char *argv[], FILE *out);

int cmd_list(Context *ctx, int argc, const char *argv[], FILE *out)
{
    UNUSED(argv);

    if (argc > 1) {
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

// int cmd_remove(Context *ctx, int argc, const char *argv[], FILE *out);

int cmd_search(Context *ctx, int argc, const char *argv[], FILE *out)
{
    UNUSED(ctx);

    if (argc != 2) {
        return -1;
    }

    int result = 0;
    List *found = list_create();
    list_set_free_fn(found, free);
    OsDir *dir = os_opendir("../../catalog");
    if (dir == NULL) {
        result = -1;
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
                result = -1;
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

    list_sort(found, (int (*)(const void *, const void *))strcmp);

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

    return result;
}

// int cmd_update(Context *ctx, int argc, const char *argv[], FILE *out);

// int cmd_upgrade(Context *ctx, int argc, const char *argv[], FILE *out);
