#pragma once

struct OsDir;

enum {
    CATALOG_OK = 0,

    CATALOG_EINVALID,
    CATALOG_ENAMETOOLONG,
    CATALOG_ENOENT,
};

#define CATALOG_ITEM_MAX_SIZE 512 /* Should match INI prop max size. */

typedef struct CatalogItem {
    char name[CATALOG_ITEM_MAX_SIZE];
    char desc[CATALOG_ITEM_MAX_SIZE];
    char uri[CATALOG_ITEM_MAX_SIZE];
} CatalogItem;

typedef struct CatalogSearch {
    const char *path; /* Path of the catalog. */
    const char *search_term; /* The string that is being searched for. */

    /* For internal use only. */
    /* Stores the current item being processed. This item may not contain the search term. */
    CatalogItem *_found;
    struct OsDir *_dir;
} CatalogSearch;

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

/**
 * Initializes the catalog search. Will search all catalog items in path that
 * contain the search term.
 *
 * IMPORTANT: path and text are borrowed. The caller shall ensure their data
 * will live as long as the CatalogSearch.
 *
 * RETURNS:
 *  CatalogSearch on success, otherwise returns NULL if allocation failed or if
 *  path is not a valid directory.
 */
CatalogSearch *catalog_search_begin(const char *path, const char *text);

/**
 * Searches catalog for the next item that contains the search term. If the
 * CatalogItem returned is not NULL then it is guaranteed to contain null
 * terminated strings.
 *
 * NOTES: The search is case insensitive. If this function encounters any errors
 * it just ignores them.
 *
 * RETURNS:
 *  The found item that contains the search term, or NULL if it was not found or
 *  the end of the search has been reached. The returned structure uses internal
 *  values and thus may change on subsequent calls to search_inext. If the
 *  values need to be saved then they should be copied.
 */
CatalogItem *catalog_search_inext(CatalogSearch *cs);

/**
 * Releases all resources used by the search.
 */
void catalog_search_end(CatalogSearch *cs);
