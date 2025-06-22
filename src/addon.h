#pragma once

/**
 * OVERVIEW
 * --------
 *
 * Addon module handles entire lifecycle of an addon. The lifecycle generally goes as such:
 *   addon_info -> addon_fetch -> addon_package -> addon_extract
 */

#include <stdint.h>

struct cJSON;
struct Context;

enum {
    ADDON_OK = 0,

    ADDON_EBADJSON, /* could not parse JSON */
    ADDON_ECATALOG, /* could not get item from catalog */
    ADDON_ECONFIG, /* could not parse config.ini */
    ADDON_EHTTPREQ, /* HTTP return status did not indicate success */
    ADDON_EINTERNAL, /* addon internal */
    ADDON_ENAMETOOLONG, /* path or filename too long */
    ADDON_ENOENT, /* no such file or directory */
    ADDON_ENOTFOUND, /* could not find addon */
    ADDON_ENO_ZIP_ASSET, /* could not find ZIP URL */
    ADDON_ERATE_LIMIT, /* rate limit exceeded */
    ADDON_EUNAUTHORIZED, /* unauthorized request */
    ADDON_EUNZIP, /* could not extract ZIP */
};

typedef struct Addon {
    char *name;
    char *desc;
    char *version;
    char *uri;
    struct List *dirs; /* List of strings */
    struct List *assets; /* List of AddonAsset */

    char *_package_path;
} Addon;

typedef struct AddonAsset {
    size_t size;
    uint8_t *data;
    char *uri;
} AddonAsset;

#define ADDON_KEY_NAME "name"
#define ADDON_KEY_DESC "desc"
#define ADDON_KEY_URI "uri"
#define ADDON_KEY_VERSION "version"
#define ADDON_KEY_DIRS "dirs"
#define ADDON_KEY_ASSETS "assets"

#define ADDON_SET_STRING(astr, str) \
    do {                            \
        if ((astr) != NULL)         \
            free((astr));           \
        if (str == NULL)            \
            (astr) = NULL;          \
        else                        \
            (astr) = strdup(str);   \
    } while (0)

/**
 * Duplicates the string and sets the addon property to it. If str is NULL then
 * the addon property is released and set to NULL.
 */
#define ADDON_SET_NAME(a, str) ADDON_SET_STRING((a)->name, str)
#define ADDON_SET_DESC(a, str) ADDON_SET_STRING((a)->desc, str)
#define ADDON_SET_VERSION(a, str) ADDON_SET_STRING((a)->version, str)
#define ADDON_SET_URI(a, str) ADDON_SET_STRING((a)->uri, str)

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
// TODO: REMOVE
// void addon_cleanup_files(Addon *a);

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
int addon_from_json(Addon *a, const struct cJSON *json);
char *addon_to_json(Addon *a);

/**
 * Returns the error string for the given error code. If the code is out of
 * range or is not an error then NULL is returned.
 */
const char *addon_strerror(int errcode);

/**
 * Fetches addon metadata.
 *
 * Returns ADDON_OK on success, otherwise an addon error number.
 */
int addon_info(Addon *a, struct Context *ctx, const char *name);

/**
 * Fetches the given addon asset.
 *
 * NOTE: This should generally be called after addon_info.
 *
 * Returns ADDON_OK on success, otherwise an addon error number.
 */
int addon_fetch(Addon *a, struct Context *ctx, AddonAsset *asset);

/**
 * Prepares addon for extraction.
 *
 * NOTE: This should generally be called after addon_fetch.
 *
 * Returns ADDON_OK on success, otherwise an addon error number.
 */
int addon_package(Addon *a, struct Context *ctx);

/**
 * Moves all packaged files from the package directory to the given path. First
 * checks and removes any files/directories from the given path before moving
 * the packaged files.
 *
 * NOTE: addon_package shall be called before this function.
 *
 * Returns ADDON_OK on success, otherwise an addon error number.
 */
int addon_extract(Addon *a, struct Context *ctx, const char *path);
