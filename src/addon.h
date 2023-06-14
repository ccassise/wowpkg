#pragma once

#include <stdio.h>

#include <cjson/cJSON.h>

#include "list.h"

enum {
    ADDON_OK = 0,

    ADDON_ENOTFOUND, // Addon could not be found.
    ADDON_ENOENT, // File/directory doesn't exist.
    ADDON_EBADJSON, // Failed to parse JSON.
    ADDON_ENO_ZIP_ASSET, // Could not find .zip asset in Github release.
    ADDON_ENAME_TOO_LONG, // Path or filename is too long.
    ADDON_EUNZIP, // Failed to extract .zip.
    ADDON_EINTERNAL, // Internal error.
};

typedef struct Addon {
    char *name;
    char *desc;
    char *url;
    char *version;
    char *handler;
    List *dirs;

    char *_zip_path;
    char *_package_path;
} Addon;

#define ADDON_NAME "name"
#define ADDON_DESC "desc"
#define ADDON_URL "url"
#define ADDON_VERSION "version"
#define ADDON_HANDLER "handler"
#define ADDON_DIRS "dirs"

Addon *addon_create(void);

/**
 * Frees all memory used by given addon. Also deletes any created files except
 * for the ones that have been extracted to a path with addon_extract.
 */
void addon_free(Addon *a);

/**
 * Removes all files created by given addon. Does not remove files that were
 * extracted with addon_extract.
 *
 * NOTE: This function is called implicitly by addon_free. Calling it after
 * addon_free does nothing.
 */
void addon_cleanup_files(Addon *a);

/**
 * Creates and returns a new addon that was deep copied from the given addon.
 *
 * NOTE: Does not copy the contents of Addon.dirs. The returned addon will just
 * contain an empty list.
 */
Addon *addon_dup(Addon *a);

/**
 * Converts an addon to/from JSON. All public properties will be converted. If a
 * given JSON file does not contain a property then it will be skipped and will
 * contain NULL.
 *
 * addon_from_json returns 0 on success and non-zero on error.
 *
 * addon_to_json returns a string that shall be freed by the caller on success,
 * and NULL on error.
 */
int addon_from_json(Addon *a, const cJSON *json);
char *addon_to_json(Addon *a);

/**
 * Sets the string pointed to by old to the string pointed to by new. If old is
 * not NULL then it will be free'd before getting set.
 */
void addon_set_str(char **old, char *new);

/**
 * Retrieves addon metadata from the catalog.
 *
 * Returns NULL on error and sets out_err.
 */
cJSON *addon_fetch_catalog_meta(const char *name, int *out_err);

/**
 * Retrieves addon metadata from Github.
 *
 * Returns NULL on error and sets out_err.
 */
cJSON *addon_fetch_github_meta(const char *url, int *out_err);

/**
 * Fetches all metadata for addon that matches the given name. On success the
 * addon will have all metadata filled out and its url will point to a .zip
 * download.
 *
 * Returns ADDON_ENOT_FOUND if name doesn't match any known addons.
 */
int addon_fetch_all_meta(Addon *a, const char *name);

/**
 * Downloads the .zip associated to Addon. Addon.url shall be a download link to
 * the .zip before calling this function.
 */
int addon_fetch_zip(Addon *a);

/**
 * Packages an addon.
 *
 * Returns non zero on errors.
 */
int addon_package(Addon *a);

/**
 * Moves all packaged files from the package directory to the given path. First
 * checks and removes any files/directories from the given path before moving
 * the packaged files.
 *
 * NOTE: addon_package shall be called before this function.
 */
int addon_extract(Addon *a, const char *path);
