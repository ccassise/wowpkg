#pragma once

#include <stdio.h>

#include <cjson/cJSON.h>

#include "list.h"

enum {
    ADDON_OK = 0,

    ADDON_ENOTFOUND, /* Addon could not be found. */
    ADDON_ENOENT, /* File/directory doesn't exist. */
    ADDON_EBADJSON, /* Failed to parse JSON. */
    ADDON_ENO_ZIP_ASSET, /* Could not find .zip asset in GitHub release. */
    ADDON_ENAMETOOLONG, /* Path or filename is too long. */
    ADDON_EUNZIP, /* Failed to extract .zip. */
    ADDON_EINTERNAL, /* Internal error. */
    ADDON_ERATE_LIMIT, /* Failed because rate limit to external API exceeded. */
    ADDON_ECONFIG, /* Config file bad format. */
};

typedef struct Addon {
    char *name;
    char *desc;
    char *url;
    char *version;
    List *dirs;

    char *_package_path;

    uint8_t *_zip;
    size_t _zip_size;
} Addon;

#define ADDON_NAME "name"
#define ADDON_DESC "desc"
#define ADDON_URL "url"
#define ADDON_VERSION "version"
#define ADDON_DIRS "dirs"

Addon *addon_create(void);

/**
 * Frees all memory used by given addon. Also deletes any files that addon
 * currently has a handle to.
 *
 * Passing a NULL pointer will make this function return immediately with no
 * action.
 */
void addon_destroy(Addon *a);

/**
 * Deletes all files that addon currently has a handle to. If files were
 * extracted with addon_extract then those files will not be deleted.
 *
 * NOTE: This function is called implicitly by addon_destroy. Calling it after
 * addon_destroy does nothing.
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
 *
 * If new string is NULL then function returns immediately with no action.
 */
void addon_set_str(char **restrict oldstr, char *restrict newstr);

/**
 * Retrieves addon metadata from the catalog.
 *
 * Returns 0, non-zero on error.
 */
int addon_fetch_catalog_meta(Addon *a, const char *name);

/**
 * Retrieves addon metadata from GitHub.
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
 * Prepares addon for extraction.
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
