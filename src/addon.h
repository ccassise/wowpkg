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
    char *handler;
    char *name;
    char *desc;
    char *url;
    char *version;
    List *dirs;

    char *_zip_path;
} Addon;

#define ADDON_HANDLER "handler"
#define ADDON_NAME "name"
#define ADDON_DESC "desc"
#define ADDON_URL "url"
#define ADDON_VERSION "version"
#define ADDON_DIRS "dirs"

/**
 * Searches the given JSON for properties with the same name as the ones in
 * struct Addon and assigns their value to the respective Addon variable. If the
 * given JSON does not include a property then it is skipped. If it does include
 * a property then the current value of the property in struct Addon (if any)
 * will be freed then set to the new property.
 *
 * NOTE: The strings assigned by this function shall be freed by the caller.
 */
int addon_from_json(Addon *a, const cJSON *json);

char *addon_to_json(Addon *a);

/**
 * Frees all properties of addon. If a zip_path is set then the associated file
 * will be removed in addition to the string being freed.
 */
void addon_free(Addon *a);

/**
 * Sets the string pointed to by old to the string pointed to by new. If old is
 * not NULL then it will be free'd before getting set.
 */
void addon_set_str(char **old, char *new);

/**
 * Fills the addon struct with metadata based on the given name. This function
 * may make a HTTP request. If this functions succeeds then the Addon will have
 * its properties filled.
 *
 * Returns non zero on errors.
 */
// int addon_fetch_metadata(Addon *a, const char *name);

cJSON *addon_metadata_from_catalog(const char *name, int *out_err);

cJSON *addon_metadata_from_github(const char *url, int *out_err);

/**
 * Makes a HTTP request to download the addon's .zip. It is expected that the
 * addon's metadata is already filled out and that specifically Addon.url is a
 * download link to the addon's .zip.
 *
 * Returns non zero on errors.
 */
int addon_package(Addon *a);

/**
 * Unzips the addon's .zip file and moves it to the WoW addon directory. This
 * function should generally only be called after `addon_package` has succeeded.
 *
 * Returns non zero on errors.
 */
int addon_extract(Addon *a);

void addon_print(const Addon *a, FILE *out);
