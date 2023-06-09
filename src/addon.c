#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <curl/curl.h>

#include "addon.h"
#include "osapi.h"
#include "zipper.h"

#ifdef _WIN32
#define strcasecmp _stricmp
#endif

#define ARRLEN(a) (sizeof(a) / sizeof((a)[0]))

#define USER_AGENT "CURL"

typedef struct Response {
    char *data;
    size_t size;
} Response;

static size_t write_str_cb(void *data, size_t size, size_t nmemb, void *userdata)
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
        return _strdup(prop->valuestring);
    }

    return NULL;
}

int addon_from_json(Addon *a, const cJSON *json)
{
    addon_set_str(&a->handler, create_str_from_property(json, ADDON_HANDLER));
    addon_set_str(&a->name, create_str_from_property(json, ADDON_NAME));
    addon_set_str(&a->desc, create_str_from_property(json, ADDON_DESC));
    addon_set_str(&a->url, create_str_from_property(json, ADDON_URL));
    addon_set_str(&a->version, create_str_from_property(json, ADDON_VERSION));

    a->dirs = list_create();
    cJSON *dirs = cJSON_GetObjectItemCaseSensitive(json, ADDON_DIRS);
    if (dirs == NULL) {
        return ADDON_OK;
    }

    cJSON *dir = NULL;
    cJSON_ArrayForEach(dir, dirs)
    {
        if (!cJSON_IsString(dir) || dir->valuestring == NULL) {
            continue;
        }

        list_insert(a->dirs, _strdup(dir->valuestring));
    }
    list_set_free_fn(a->dirs, free);

    return ADDON_OK;
}

char *addon_to_json(Addon *a)
{
    int err = ADDON_OK;
    char *result = NULL;
    cJSON *json = cJSON_CreateObject();
    if (json == NULL) {
        err = ADDON_EINTERNAL;
        goto end;
    }

    if (cJSON_AddStringToObject(json, ADDON_HANDLER, a->handler) == NULL) {
        err = ADDON_EINTERNAL;
        goto end;
    }

    if (cJSON_AddStringToObject(json, ADDON_NAME, a->name) == NULL) {
        err = ADDON_EINTERNAL;
        goto end;
    }

    if (cJSON_AddStringToObject(json, ADDON_DESC, a->desc) == NULL) {
        err = ADDON_EINTERNAL;
        goto end;
    }

    if (cJSON_AddStringToObject(json, ADDON_URL, a->url) == NULL) {
        err = ADDON_EINTERNAL;
        goto end;
    }

    if (cJSON_AddStringToObject(json, ADDON_VERSION, a->version) == NULL) {
        err = ADDON_EINTERNAL;
        goto end;
    }

    cJSON *dirs = cJSON_AddArrayToObject(json, ADDON_DIRS);
    if (dirs == NULL) {
        err = ADDON_EINTERNAL;
        goto end;
    }

    ListNode *node = NULL;
    list_foreach(node, a->dirs)
    {
        cJSON_AddItemToArray(dirs, cJSON_CreateString((char *)node->value));
    }

end:
    if (err == ADDON_OK) {
        result = cJSON_PrintUnformatted(json);
    }

    if (json != NULL) {
        cJSON_Delete(json);
    }

    return result;
}

void addon_free(Addon *a)
{
    if (a->handler != NULL) {
        free(a->handler);
    }

    if (a->name != NULL) {
        free(a->name);
    }

    if (a->desc != NULL) {
        free(a->desc);
    }

    if (a->url != NULL) {
        free(a->url);
    }

    if (a->version != NULL) {
        free(a->version);
    }

    if (a->dirs != NULL) {
        list_free(a->dirs);
    }

    if (a->_zip_path != NULL) {
        remove(a->_zip_path);
        free(a->_zip_path);
    }
}

void addon_set_str(char **old, char *new)
{
    if (*old == new || new == NULL) {
        return;
    }

    if (*old != NULL) {
        free(*old);
        *old = NULL;
    }
    *old = new;
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
 * to or larger than n, then ADDON_ENAME_TOO_LONG will be returned. The
 * characters that could fit will be in s.
 *
 * On success returns ADDON_OK. Otherwise returns one of the ADDON_E types.
 */
static int snfind_catalog_path(char *s, size_t n, const char *name)
{
    int result = ADDON_OK;

    const char *catalog_path = "../../catalog";
    OsDir *dir = os_opendir(catalog_path);
    if (dir == NULL) {
        return ADDON_EINTERNAL;
    }

    OsDirEnt *entry;
    while ((entry = os_readdir(dir)) != NULL) {
        if (strcmp(entry->name, ".") == 0 || strcmp(entry->name, "..") == 0) {
            continue;
        }

        char basename[_MAX_FNAME];
        snprintf(basename, sizeof(basename), "%s", entry->name);

        char *ext_start = strstr(basename, ".json");
        if (ext_start == NULL) {
            continue;
        }

        *ext_start = '\0';

        if (strcasecmp(basename, name) == 0) {
            int nwrote = snprintf(s, n, "%s%c%s", catalog_path, OS_SEPARATOR, entry->name);
            if (nwrote >= n) {
                result = ADDON_ENAME_TOO_LONG;
                goto end;
            }

            break;
        }
    }

end:
    os_closedir(dir);

    return result;
}

cJSON *addon_metadata_from_catalog(const char *name, int *out_err)
{
    int err = ADDON_OK;
    FILE *f = NULL;
    char *catalog = NULL;
    cJSON *result = NULL;

    char path[MAX_PATH];
    err = snfind_catalog_path(path, MAX_PATH, name);
    if (err != ADDON_OK) {
        goto end;
    }

    f = fopen(path, "rb");
    if (f == NULL) {
        err = ADDON_ENOENT;
        goto end;
    }

    struct _stat s;
    if (_stat(path, &s) != 0) {
        err = ADDON_ENOENT;
        goto end;
    }

    off_t fsz = s.st_size;

    catalog = malloc(fsz + 1);
    if (catalog == NULL) {
        err = ADDON_EINTERNAL;
        goto end;
    }
    if (fread(catalog, sizeof(*catalog), fsz, f) < 0) {
        err = ADDON_EBADJSON;
        goto end;
    }

    catalog[fsz] = '\0';

    result = cJSON_Parse(catalog);
    if (result == NULL) {
        err = ADDON_EBADJSON;
        goto end;
    }

end:
    if (f != NULL) {
        fclose(f);
    }

    if (catalog != NULL) {
        free(catalog);
    }

    if (out_err != NULL) {
        *out_err = err;
    }

    if (err != ADDON_OK && result != NULL) {
        cJSON_Delete(result);
        return NULL;
    }

    return result;
}

cJSON *addon_metadata_from_github(const char *url, int *out_err)
{
    int err = ADDON_OK;
    cJSON *resp = NULL;
    cJSON *result = NULL;
    Response res = { .data = NULL, .size = 0 };

    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        err = ADDON_EINTERNAL;
        goto end;
    }

    // curl_easy_setopt(curl, CURLOPT_VERBOSE);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_str_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&res);

    CURLcode status = curl_easy_perform(curl);
    if (status != CURLE_OK) {
        err = ADDON_EINTERNAL;
        goto end;
    }

    int http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200 || status == CURLE_ABORTED_BY_CALLBACK) {
        err = ADDON_EINTERNAL;
        goto end;
    }

    resp = cJSON_Parse(res.data);
    if (resp == NULL) {
        err = ADDON_EINTERNAL;
        goto end;
    }

    cJSON *tag_name = cJSON_GetObjectItemCaseSensitive(resp, "tag_name");
    if (!json_check_string(tag_name)) {
        err = ADDON_EBADJSON;
        goto end;
    }

    result = cJSON_CreateObject();
    if (result == NULL) {
        err = ADDON_EINTERNAL;
        goto end;
    }

    if (cJSON_AddStringToObject(result, "version", tag_name->valuestring) == NULL) {
        err = ADDON_EINTERNAL;
        goto end;
    }

    // a->version = _strdup(tag_name->valuestring);

    const char *zip_url = gh_resp_find_asset_zip(resp);
    if (zip_url == NULL) {
        err = ADDON_ENO_ZIP_ASSET;
        goto end;
    }

    if (cJSON_AddStringToObject(result, "url", zip_url) == NULL) {
        err = ADDON_EINTERNAL;
        goto end;
    }

end:
    if (curl != NULL) {
        curl_easy_cleanup(curl);
    }

    if (res.data) {
        free(res.data);
    }

    if (resp) {
        cJSON_Delete(resp);
    }

    if (out_err != NULL) {
        *out_err = err;
    }

    if (err != ADDON_OK && result != NULL) {
        cJSON_Delete(result);
        return NULL;
    }

    return result;
}

int addon_package(Addon *a)
{
    Response res = { .data = NULL, .size = 0 };
    FILE *fzip = NULL;
    char *tmpfile = NULL;

    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        return ADDON_EINTERNAL;
    }

    int err = ADDON_OK;

    curl_easy_setopt(curl, CURLOPT_URL, a->url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_str_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&res);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

    CURLcode status = curl_easy_perform(curl);
    if (status != CURLE_OK) {
        err = ADDON_EINTERNAL;
        goto end;
    }

    int http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200 || status == CURLE_ABORTED_BY_CALLBACK) {
        err = ADDON_EINTERNAL;
        goto end;
    }

    res.size--; // Remove terminating null since this should be raw data.

    char zipname[_MAX_FNAME];
    int nwrote = snprintf(zipname, _MAX_FNAME, "%s_%s_", a->name, a->version);
    if (nwrote >= _MAX_FNAME) {
        err = ADDON_ENAME_TOO_LONG;
        goto end;
    }

    tmpfile = _tempnam(NULL, zipname);
    if (tmpfile == NULL) {
        err = ADDON_EINTERNAL;
        goto end;
    }

    fzip = fopen(tmpfile, "wb");
    if (fzip == NULL) {
        err = ADDON_EINTERNAL;
        goto end;
    }

    if (fwrite(res.data, sizeof(*res.data), res.size, fzip) < res.size) {
        err = ADDON_EINTERNAL;
        goto end;
    }

    addon_set_str(&a->_zip_path, tmpfile);

end:
    curl_easy_cleanup(curl);
    if (res.data != NULL) {
        free(res.data);
    }

    if (fzip != NULL) {
        fclose(fzip);
    }

    return err;
}

int addon_extract(Addon *a)
{
    const char *addon_path = "../../../out/dump/";
    if (zipper_unzip(addon_path, a->_zip_path) != 0) {
        return ADDON_EUNZIP;
    }

    return 0;
}

void addon_print(const Addon *a, FILE *out)
{
    if (a->name != NULL) {
        fprintf(out, "name: %s\n", a->name);
    }

    if (a->desc != NULL) {
        fprintf(out, "desc: %s\n", a->desc);
    }

    if (a->version != NULL) {
        fprintf(out, "version: %s\n", a->version);
    }

    if (a->url != NULL) {
        fprintf(out, "url: %s\n", a->url);
    }

    if (a->handler != NULL) {
        fprintf(out, "handler: %s\n", a->handler);
    }
}
