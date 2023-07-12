#include <stdlib.h>
#include <sys/stat.h>

#include <curl/curl.h>

#include "addon.h"
#include "ini.h"
#include "osapi.h"
#include "osstring.h"
#include "wowpkg.h"
#include "zipper.h"

typedef struct Response {
    size_t size;
    char *data;
} Response;

static size_t write_str_cb(void *restrict data, size_t size, size_t nmemb, void *restrict userdata)
{
    size_t realsize = size * nmemb;
    Response *res = userdata;

    char *ptr = realloc(res->data, res->size + realsize + 1);
    if (ptr == NULL) {
        return 0;
    }

    res->data = ptr;
    memcpy(&(res->data[res->size]), data, realsize);
    res->size += realsize;
    res->data[res->size] = '\0';

    return realsize;
}

static int json_check_string(const cJSON *value)
{
    return cJSON_IsString(value) && value->valuestring != NULL;
}

/**
 * Duplicates the string value for the given property if it exists.
 *
 * On success return a newly allocated string that the caller shall free. If the
 * property is not found or has no value then NULL is returned.
 */
static char *create_str_from_property(const cJSON *json, const char *property)
{
    cJSON *prop = cJSON_GetObjectItemCaseSensitive(json, property);
    if (json_check_string(prop)) {
        return strdup(prop->valuestring);
    }

    return NULL;
}

static struct curl_slist *set_github_headers(struct curl_slist *list)
{
    list = curl_slist_append(list, "Accept: application/vnd.github+json");
    list = curl_slist_append(list, "X-GitHub-Api-Version: 2022-11-28");

    return list;
}

Addon *addon_create(void)
{
    Addon *result = malloc(sizeof(*result));
    if (result != NULL) {
        memset(result, 0, sizeof(*result));
        result->dirs = list_create();
        if (result->dirs == NULL) {
            free(result);
            return NULL;
        }

        list_set_free_fn(result->dirs, free);
    }

    return result;
}

void addon_free(Addon *a)
{
    if (a == NULL) {
        return;
    }

    free(a->name);
    free(a->desc);
    free(a->url);
    free(a->version);
    list_free(a->dirs);
    addon_cleanup_files(a);

    free(a);
}

static int move_filename(const char *restrict srcdir, const char *restrict destdir, const char *restrict filename)
{
    int n;

    char dest[OS_MAX_PATH];
    n = snprintf(dest, ARRAY_SIZE(dest), "%s%c%s", destdir, OS_SEPARATOR, filename);
    if (n < 0 || (size_t)n >= ARRAY_SIZE(dest)) {
        return ADDON_ENAMETOOLONG;
    }

    struct os_stat s;
    if (os_stat(dest, &s) == 0) {
        if (os_remove_all(dest) != 0) {
            return ADDON_EINTERNAL;
        }
    }

    char src[OS_MAX_PATH];
    n = snprintf(src, ARRAY_SIZE(src), "%s%c%s", srcdir, OS_SEPARATOR, filename);
    if (n < 0 || (size_t)n >= ARRAY_SIZE(src)) {
        return ADDON_ENAMETOOLONG;
    }

    // TODO: If this fails should check if errno is EXDEV and if so it should
    // copy the directory and then delete.
    if (os_rename(src, dest) != 0) {
        return ADDON_EINTERNAL;
    }

    return ADDON_OK;
}

/**
 * Returns a reference to a cJSON string that contains the .zip download link.
 *
 * IMPORTANT: It shall not attempt to be freed. It shall not outlive the
 * lifetime of the passed in cJSON object.
 */
static const char *gh_resp_find_asset_zip(const cJSON *json)
{
    char *result = NULL;
    cJSON *assets = cJSON_GetObjectItemCaseSensitive(json, "assets");
    if (!cJSON_IsArray(assets)) {
        return NULL;
    }

    cJSON *asset = NULL;
    cJSON_ArrayForEach(asset, assets)
    {
        cJSON *content_type = cJSON_GetObjectItemCaseSensitive(asset, "content_type");
        if (!json_check_string(content_type)) {
            continue;
        }

        if (strcmp(content_type->valuestring, "application/zip") != 0) {
            continue;
        }

        cJSON *download_url = cJSON_GetObjectItemCaseSensitive(asset, "browser_download_url");
        if (!json_check_string(download_url)) {
            continue;
        }

        result = download_url->valuestring;
    }

    return result;
}

/**
 * Finds the catalog item path and stores it in s. s will always be null
 * terminated. If the amount of characters that would be written to s is equal
 * to or larger than n, then the characters that could fit will be in s and the
 * returned value will equal the amount of characters that would have been wrote
 * if s had sufficient space.
 */
static int snfind_catalog_path(char *restrict s, size_t n, const char *restrict name)
{
    int result = -1;

    OsDir *dir = dir = os_opendir(WOWPKG_CATALOG_PATH);
    if (dir == NULL) {
        return -1;
    }

    OsDirEnt *entry;
    while ((entry = os_readdir(dir)) != NULL) {
        if (strcmp(entry->name, ".") == 0 || strcmp(entry->name, "..") == 0) {
            continue;
        }

        char *ext_start = strstr(entry->name, ".ini");
        if (ext_start == NULL) {
            continue;
        }

        size_t filename_len = (size_t)(ext_start - entry->name);
        if (strncasecmp(entry->name, name, filename_len) == 0 && strlen(name) == filename_len) {
            result = snprintf(s, n, "%s%c%s", WOWPKG_CATALOG_PATH, OS_SEPARATOR, entry->name);

            break;
        }
    }

    os_closedir(dir);

    return result;
}

void addon_cleanup_files(Addon *a)
{
    if (a->_zip_path != NULL) {
        remove(a->_zip_path);
        free(a->_zip_path);
        a->_zip_path = NULL;
    }

    if (a->_package_path != NULL) {
        os_remove_all(a->_package_path);
        free(a->_package_path);
        a->_package_path = NULL;
    }
}

Addon *addon_dup(Addon *a)
{
    Addon *result = addon_create();
    if (result) {
        result->name = a->name == NULL ? NULL : strdup(a->name);
        result->desc = a->desc == NULL ? NULL : strdup(a->desc);
        result->version = a->version == NULL ? NULL : strdup(a->version);
        result->url = a->url == NULL ? NULL : strdup(a->url);
    }

    return result;
}

int addon_from_json(Addon *a, const cJSON *json)
{
    addon_set_str(&a->name, create_str_from_property(json, ADDON_NAME));
    addon_set_str(&a->desc, create_str_from_property(json, ADDON_DESC));
    addon_set_str(&a->url, create_str_from_property(json, ADDON_URL));
    addon_set_str(&a->version, create_str_from_property(json, ADDON_VERSION));

    cJSON *dirs = cJSON_GetObjectItemCaseSensitive(json, ADDON_DIRS);
    if (cJSON_IsArray(dirs)) {
        cJSON *dir = NULL;
        cJSON_ArrayForEach(dir, dirs)
        {
            if (!cJSON_IsString(dir) || dir->valuestring == NULL) {
                continue;
            }

            list_insert(a->dirs, strdup(dir->valuestring));
        }
    }

    return ADDON_OK;
}

char *addon_to_json(Addon *a)
{
    int err = ADDON_OK;
    char *result = NULL;
    cJSON *json = cJSON_CreateObject();
    if (json == NULL) {
        err = ADDON_EINTERNAL;
        goto cleanup;
    }

    if (cJSON_AddStringToObject(json, ADDON_NAME, a->name) == NULL) {
        err = ADDON_EINTERNAL;
        goto cleanup;
    }

    if (cJSON_AddStringToObject(json, ADDON_DESC, a->desc) == NULL) {
        err = ADDON_EINTERNAL;
        goto cleanup;
    }

    if (cJSON_AddStringToObject(json, ADDON_VERSION, a->version) == NULL) {
        err = ADDON_EINTERNAL;
        goto cleanup;
    }

    if (cJSON_AddStringToObject(json, ADDON_URL, a->url) == NULL) {
        err = ADDON_EINTERNAL;
        goto cleanup;
    }

    cJSON *dirs = cJSON_AddArrayToObject(json, ADDON_DIRS);
    if (dirs == NULL) {
        err = ADDON_EINTERNAL;
        goto cleanup;
    }

    ListNode *node = NULL;
    list_foreach(node, a->dirs)
    {
        cJSON_AddItemToArray(dirs, cJSON_CreateString((char *)node->value));
    }

cleanup:
    if (err == ADDON_OK) {
        result = cJSON_PrintUnformatted(json);
    }

    cJSON_Delete(json);

    return result;
}

void addon_set_str(char **restrict oldstr, char *restrict newstr)
{
    if (newstr == NULL) {
        return;
    }

    if (*oldstr != NULL) {
        free(*oldstr);
        *oldstr = NULL;
    }
    *oldstr = newstr;
}

int addon_fetch_catalog_meta(Addon *a, const char *name)
{
    int err = ADDON_OK;

    char path[OS_MAX_PATH];
    int n = snfind_catalog_path(path, ARRAY_SIZE(path), name);
    if (n < 0 || (size_t)n >= ARRAY_SIZE(path)) {
        err = ADDON_ENOTFOUND;
        return err;
    }

    INI *ini = ini_open(path);
    if (ini == NULL) {
        err = ADDON_ENOENT;
        goto cleanup;
    }

    INIKey *key = NULL;
    while ((key = ini_readkey(ini)) != NULL) {
        if (strcasecmp(key->name, ADDON_NAME) == 0) {
            addon_set_str(&a->name, strdup(key->value));
        } else if (strcasecmp(key->name, ADDON_DESC) == 0) {
            addon_set_str(&a->desc, strdup(key->value));
        } else if (strcasecmp(key->name, ADDON_URL) == 0) {
            addon_set_str(&a->url, strdup(key->value));
        }
    }

    if (ini_last_error(ini) != INI_EEOF
        || a->name == NULL
        || a->desc == NULL
        || a->url == NULL) {

        err = ADDON_ECONFIG;
        goto cleanup;
    }

cleanup:
    ini_close(ini);

    return err;
}

cJSON *addon_fetch_github_meta(const char *url, int *out_err)
{
    int err = ADDON_OK;
    Response res = { .data = NULL, .size = 0 };
    cJSON *resp = NULL;
    cJSON *result = NULL;
    struct curl_slist *headers = NULL;

    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        err = ADDON_EINTERNAL;
        goto cleanup;
    }

    headers = set_github_headers(headers);

    // curl_easy_setopt(curl, CURLOPT_VERBOSE, true);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, WOWPKG_USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_str_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&res);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode status = curl_easy_perform(curl);
    if (status != CURLE_OK) {
        err = ADDON_EINTERNAL;
        goto cleanup;
    }

    long http_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        if (http_code == 403) {
            err = ADDON_ERATE_LIMIT;
        } else {
            err = ADDON_EINTERNAL;
        }
        goto cleanup;
    }

    resp = cJSON_Parse(res.data);
    if (resp == NULL) {
        err = ADDON_EINTERNAL;
        goto cleanup;
    }

    cJSON *tag_name = cJSON_GetObjectItemCaseSensitive(resp, "tag_name");
    if (!json_check_string(tag_name)) {
        err = ADDON_EBADJSON;
        goto cleanup;
    }

    result = cJSON_CreateObject();
    if (result == NULL) {
        err = ADDON_EINTERNAL;
        goto cleanup;
    }

    if (cJSON_AddStringToObject(result, "version", tag_name->valuestring) == NULL) {
        err = ADDON_EINTERNAL;
        goto cleanup;
    }

    const char *zip_url = gh_resp_find_asset_zip(resp);
    if (zip_url == NULL) {
        err = ADDON_ENO_ZIP_ASSET;
        goto cleanup;
    }

    if (cJSON_AddStringToObject(result, "url", zip_url) == NULL) {
        err = ADDON_EINTERNAL;
        goto cleanup;
    }

cleanup:
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    free(res.data);
    cJSON_Delete(resp);

    if (out_err != NULL) {
        *out_err = err;
    }

    if (err != ADDON_OK) {
        cJSON_Delete(result);
        result = NULL;
    }

    return result;
}

int addon_fetch_all_meta(Addon *a, const char *name)
{
    int err = ADDON_OK;

    cJSON *gh_json = NULL;

    err = addon_fetch_catalog_meta(a, name);
    if (err != ADDON_OK) {
        goto cleanup;
    }

    gh_json = addon_fetch_github_meta(a->url, &err);
    if (err != ADDON_OK) {
        goto cleanup;
    }

    err = addon_from_json(a, gh_json);
    if (err != ADDON_OK) {
        goto cleanup;
    }

cleanup:
    cJSON_Delete(gh_json);

    return err;
}

int addon_fetch_zip(Addon *a)
{
    int err = ADDON_OK;
    FILE *fzip = NULL;
    Response res = { .data = NULL, .size = 0 };
    struct curl_slist *headers = NULL;

    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        return ADDON_EINTERNAL;
    }

    headers = set_github_headers(headers);

    curl_easy_setopt(curl, CURLOPT_URL, a->url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, WOWPKG_USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_str_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&res);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode status = curl_easy_perform(curl);
    if (status != CURLE_OK) {
        err = ADDON_EINTERNAL;
        goto cleanup;
    }

    long http_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        if (http_code == 403) {
            err = ADDON_ERATE_LIMIT;
        } else {
            err = ADDON_EINTERNAL;
        }
        goto cleanup;
    }

    if (res.size > 0) {
        res.size--; // Remove terminating null since this should be raw data.
    }

    const char *zip_ext = ".zip";
    char zippath[OS_MAX_PATH];

    // Creates a string with a value 'path/to/temp/wowpkg_<addon_name>_<addon_version>_XXXXXX.zip'.
    int nwrote = snprintf(zippath, ARRAY_SIZE(zippath), "%s%c%s_%s_%s_XXXXXX%s", os_tempdir(), OS_SEPARATOR, WOWPKG_NAME, a->name, a->version, zip_ext);
    if (nwrote < 0 || (size_t)nwrote >= ARRAY_SIZE(zippath)) {
        err = ADDON_ENAMETOOLONG;
        goto cleanup;
    }

    fzip = os_mkstemps(zippath, (int)strlen(zip_ext));
    if (fzip == NULL) {
        err = ADDON_EINTERNAL;
        goto cleanup;
    }

    if (fwrite(res.data, sizeof(*res.data), res.size, fzip) != res.size) {
        err = ADDON_EINTERNAL;
        goto cleanup;
    }

    addon_set_str(&a->_zip_path, strdup(zippath));

cleanup:
    if (fzip != NULL) {
        fclose(fzip);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(res.data);

    return err;
}

int addon_package(Addon *a)
{
    char tmpdir[OS_MAX_PATH];

    // Creates a string with a value 'path/to/temp/wowpkg_<addon_name>_<addon_version>_XXXXXX'.
    int n = snprintf(tmpdir, ARRAY_SIZE(tmpdir), "%s%c%s_%s_%s_XXXXXX", os_tempdir(), OS_SEPARATOR, WOWPKG_NAME, a->name, a->version);
    if (n < 0 || (size_t)n >= ARRAY_SIZE(tmpdir)) {
        return ADDON_ENAMETOOLONG;
    }

    if (os_mkdtemp(tmpdir) == NULL) {
        return ADDON_EINTERNAL;
    }

    if (zipper_unzip(a->_zip_path, tmpdir) != 0) {
        return ADDON_EUNZIP;
    }

    addon_set_str(&a->_package_path, strdup(tmpdir));

    return ADDON_OK;
}

int addon_extract(Addon *a, const char *path)
{
    int err = ADDON_OK;

    OsDir *dir = os_opendir(a->_package_path);
    if (dir == NULL) {
        return ADDON_ENOENT;
    }

    OsDirEnt *entry = NULL;
    while ((entry = os_readdir(dir)) != NULL) {
        if (strcmp(entry->name, ".") == 0 || strcmp(entry->name, "..") == 0) {
            continue;
        }

        if ((err = move_filename(a->_package_path, path, entry->name)) != ADDON_OK) {
            goto cleanup;
        }

        list_insert(a->dirs, strdup(entry->name));
    }

cleanup:
    os_closedir(dir);

    return err;
}
