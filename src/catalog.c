#include <stdlib.h>

#include "ini.h"
#include "osapi.h"
#include "osstring.h"
#include "wowpkg.h"

#include "catalog.h"

/**
 * Parses the catalog .ini file in path and fills out CatalogItem.
 *
 * RETURNS:
 *  CATALOG_OK              On success.
 *
 * ERRORS:
 *  CATALOG_ENOENT          Failed to open the catalog file.
 *  CATALOG_ENAMETOOLONG    A value in the catalog exceeds maximum capacity.
 *  CATALOG_EINVALID        The catalog file was not in an expected form.
 */
static int parse_ini(CatalogItem *item, const char *path)
{
    INI *ini = ini_open(path);
    if (ini == NULL) {
        return CATALOG_ENOENT;
    }

    int err = CATALOG_OK;

    item->name[0] = '\0';
    item->desc[0] = '\0';
    item->uri[0] = '\0';
    INIKey *key = NULL;
    while ((key = ini_readkey(ini)) != NULL) {
        size_t key_size = strlen(key->value) + 1;
        if (key_size >= CATALOG_ITEM_MAX_SIZE) {
            err = CATALOG_ENAMETOOLONG;
            goto cleanup;
        }
        if (strcasecmp(key->name, "name") == 0) {
            memcpy(item->name, key->value, key_size);
        } else if (strcasecmp(key->name, "desc") == 0) {
            memcpy(item->desc, key->value, key_size);
        } else if (strcasecmp(key->name, "uri") == 0) {
            memcpy(item->uri, key->value, key_size);
        }
    }

    if (ini_last_error(ini) != INI_OK
        || item->name[0] == '\0'
        || item->desc[0] == '\0'
        || item->uri[0] == '\0') {

        err = CATALOG_EINVALID;
        goto cleanup;
    }

cleanup:
    ini_close(ini);

    return err;
}

int catalog_find(CatalogItem *item, const char *path, const char *name)
{
    OsDir *dir = os_opendir(path);
    if (dir == NULL) {
        return CATALOG_ENOENT;
    }

    int err = CATALOG_OK;
    OsDirEnt *entry = NULL;
    while ((entry = os_readdir(dir)) != NULL) {
        if (strcmp(entry->name, ".") == 0 || strcmp(entry->name, "..") == 0) {
            continue;
        }

        char *ext_start = strstr(entry->name, ".ini");
        if (ext_start == NULL) {
            continue;
        }

        err = CATALOG_ENOENT;
        size_t filename_len = (size_t)(ext_start - entry->name);
        if (strncasecmp(entry->name, name, filename_len) == 0 && strlen(name) == filename_len) {
            char item_path[OS_MAX_PATH];
            int n = snprintf(item_path, ARRAY_SIZE(item_path), "%s%c%s", path, OS_SEPARATOR, entry->name);
            if (n < 0 || (size_t)n >= ARRAY_SIZE(item_path)) {
                err = CATALOG_ENAMETOOLONG;
                goto cleanup;
            }
            if ((err = parse_ini(item, item_path)) != CATALOG_OK) {
                goto cleanup;
            }

            err = CATALOG_OK;
            break;
        }
    }

cleanup:
    os_closedir(dir);
    return err;
}

CatalogSearch *catalog_search_begin(const char *path, const char *text)
{
    CatalogSearch *result = malloc(sizeof(*result));
    if (result == NULL) {
        return result;
    }
    result->_dir = os_opendir(path);
    if (result->_dir == NULL) {
        free(result);
        return NULL;
    }
    result->_found = malloc(sizeof(*result->_found));
    if (result->_found == NULL) {
        free(result);
        return NULL;
    }
    result->path = path;
    result->search_term = text;
    return result;
}

CatalogItem *catalog_search_inext(CatalogSearch *cs)
{
    CatalogItem *result = NULL;
    OsDirEnt *entry = NULL;
    char item_path[OS_MAX_PATH];
    while ((entry = os_readdir(cs->_dir)) != NULL) {
        if (strcmp(entry->name, ".") == 0 || strcmp(entry->name, "..") == 0) {
            continue;
        }
        int n = snprintf(item_path, ARRAY_SIZE(item_path), "%s%c%s", cs->path, OS_SEPARATOR, entry->name);
        if (n < 0 || (size_t)n >= ARRAY_SIZE(item_path)) {
            continue;
        }
        if (parse_ini(cs->_found, item_path) != CATALOG_OK) {
            continue;
        }
        if (os_strcasestr(cs->_found->name, cs->search_term) != NULL
            || os_strcasestr(cs->_found->desc, cs->search_term) != NULL) {

            result = cs->_found;
            break;
        }
    }
    return result;
}

void catalog_search_end(CatalogSearch *cs)
{
    os_closedir(cs->_dir);
    free(cs->_found);
    free(cs);
}
