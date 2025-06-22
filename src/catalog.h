#pragma once

enum {
    CATALOG_OK = 0,

    CATALOG_EINVALID,
    CATALOG_ENAMETOOLONG,
    CATALOG_ENOENT,
};

static const char *CATALOG_ERROR_STRINGS[] = {
    /* CATALOG_OK */ "OK",

    /* CATALOG_EINVALID */ "catalog file not in expected format",
    /* CATALOG_ENAMETOOLONG */ "catalog file path or value in file is too long",
    /* CATALOG_ENOENT */ "could not open catalog file",
};

/**
 * Returns the string value that matches the given error.
 *
 * IMPORTANT: Only values in the catalog error enum are valid.
 */
#define CATALOG_STRERROR(e) CATALOG_ERROR_STRINGS[e]

#define CATALOG_ITEM_MAX_SIZE 512 /* Should match INI prop max size. */

typedef struct CatalogItem {
    char name[CATALOG_ITEM_MAX_SIZE];
    char desc[CATALOG_ITEM_MAX_SIZE];
    char uri[CATALOG_ITEM_MAX_SIZE];
} CatalogItem;

/**
 * Finds the filename in catalog path that matches the given name. The given
 * name should not include the extension.
 * On success, CatalogItem is filled with the values found in the catalog file.
 *
 * RETURNS:
 *  CATALOG_OK              On success.
 *
 * ERRORS:
 *  CATALOG_ENOENT          Could not open the given path.
 *  CATALOG_ENAMETOOLONG    Either the path to the catalog file is too long or a
 *                          value's size in the catalog file could not fit in
 *                          CatalogItem.
 * CATALOG_EINVALID         Catalog file was not in expected format.
 */
int catalog_find(CatalogItem *item, const char *path, const char *name);
