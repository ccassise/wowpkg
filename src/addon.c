#include <errno.h>
#include <stdlib.h>

#include <cjson/cJSON.h>
#include <curl/curl.h>

#include "addon.h"
#include "appstate.h"
#include "catalog.h"
#include "config.h"
#include "context.h"
#include "ini.h"
#include "list.h"
#include "osapi.h"
#include "osstring.h"
#include "wowpkg.h"
#include "zipper.h"

typedef struct Response {
    size_t size;
    uint8_t *data;
} Response;

static AddonAsset *asset_create(void)
{
    return calloc(1, sizeof(AddonAsset));
}

static void asset_destroy(AddonAsset *asset)
{
    if (asset != NULL) {
        if (asset->size > 0) {
            free(asset->data);
            asset->size = 0;
        }
        free(asset->uri);
    }
    free(asset);
}

static size_t write_str_cb(void *restrict data, size_t size, size_t nmemb, void *restrict userdata)
{
    size_t realsize = size * nmemb;
    Response *res = userdata;

    uint8_t *ptr = realloc(res->data, res->size + realsize + 1);
    if (ptr == NULL) {
        return 0;
    }

    res->data = ptr;
    memcpy(&(res->data[res->size]), data, realsize);
    res->size += realsize;
    res->data[res->size] = '\0';

    return realsize;
}

static struct curl_slist *set_github_headers(struct curl_slist *list, const char *token)
{
    list = curl_slist_append(list, "Accept: application/vnd.github+json");
    list = curl_slist_append(list, "X-GitHub-Api-Version: 2022-11-28");
    if (token != NULL) {
        char auth_hdr[512];
        int n = snprintf(auth_hdr, ARRAY_SIZE(auth_hdr), "Authorization: Bearer %s", token);
        if (n >= 0 && n < (int)ARRAY_SIZE(auth_hdr)) {
            list = curl_slist_append(list, auth_hdr);
        }
    }

    return list;
}

static void addon_cleanup_files(Addon *a)
{
    if (a->_package_path != NULL) {
        os_remove_all(a->_package_path);
        free(a->_package_path);
        a->_package_path = NULL;
    }
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

    if (os_rename(src, dest) != 0) {
        return ADDON_EINTERNAL;
    }

    return ADDON_OK;
}

static int fetch_github_info(Addon *a, Context *ctx)
{
    int err = ADDON_OK;
    Response res = { .data = NULL, .size = 0 };
    cJSON *resp = NULL;
    cJSON *assets = NULL;
    cJSON *asset = NULL;
    struct curl_slist *headers = NULL;

    headers = set_github_headers(headers, ctx->config->github_token);

    // curl_easy_setopt(ctx->curl, CURLOPT_VERBOSE, true);
    curl_easy_setopt(ctx->curl, CURLOPT_URL, a->uri);
    curl_easy_setopt(ctx->curl, CURLOPT_USERAGENT, WOWPKG_USER_AGENT);
    curl_easy_setopt(ctx->curl, CURLOPT_WRITEFUNCTION, write_str_cb);
    curl_easy_setopt(ctx->curl, CURLOPT_WRITEDATA, (void *)&res);
    curl_easy_setopt(ctx->curl, CURLOPT_HTTPHEADER, headers);

    CURLcode status = curl_easy_perform(ctx->curl);
    if (status != CURLE_OK) {
        err = ADDON_EINTERNAL;
        goto cleanup;
    }

    long http_code;
    curl_easy_getinfo(ctx->curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        if (http_code == 403 || http_code == 429) {
            err = ADDON_ERATE_LIMIT;
        } else if (http_code == 401) {
            err = ADDON_EUNAUTHORIZED;
        } else {
            err = ADDON_EINTERNAL;
        }
        goto cleanup;
    }

    resp = cJSON_Parse((char *)res.data);
    if (resp == NULL) {
        err = ADDON_EBADJSON;
        goto cleanup;
    }

    /* Get addon version info. */
    cJSON *tag_name = cJSON_GetObjectItemCaseSensitive(resp, "tag_name");
    if (!cJSON_IsString(tag_name) || tag_name->valuestring == NULL) {
        err = ADDON_EBADJSON;
        goto cleanup;
    }
    ADDON_SET_VERSION(a, tag_name->valuestring);

    /* Reset addon's asset list. */
    while (a->assets->head != NULL) {
        list_remove(a->assets, a->assets->head);
    }
    /* Get addon ZIP URLs. Some addons (like DBM-Dungeons) may have mutliple
     * ZIPs. */
    assets = cJSON_GetObjectItemCaseSensitive(resp, "assets");
    cJSON_ArrayForEach(asset, assets)
    {
        cJSON *content_type = cJSON_GetObjectItemCaseSensitive(asset, "content_type");
        if (cJSON_IsString(content_type) && strcmp(content_type->valuestring, "application/zip") == 0) {
            cJSON *download_uri = cJSON_GetObjectItemCaseSensitive(asset, "browser_download_url");
            if (cJSON_IsString(download_uri) && download_uri->valuestring != NULL) {
                AddonAsset *zip = asset_create();
                if (zip == NULL) {
                    err = ADDON_EINTERNAL;
                    goto cleanup;
                }

                zip->uri = strdup(download_uri->valuestring);
                list_insert(a->assets, zip);
            }
        }
    }

cleanup:
    curl_slist_free_all(headers);
    curl_easy_reset(ctx->curl);
    free(res.data);
    cJSON_Delete(resp);

    return err;
}

static int fetch_github_zip(AddonAsset *asset, Context *ctx)
{
    int err = ADDON_OK;
    Response res = { .data = NULL, .size = 0 };
    struct curl_slist *headers = NULL;

    headers = set_github_headers(headers, ctx->config->github_token);

    curl_easy_setopt(ctx->curl, CURLOPT_URL, asset->uri);
    curl_easy_setopt(ctx->curl, CURLOPT_USERAGENT, WOWPKG_USER_AGENT);
    curl_easy_setopt(ctx->curl, CURLOPT_WRITEFUNCTION, write_str_cb);
    curl_easy_setopt(ctx->curl, CURLOPT_WRITEDATA, (void *)&res);
    curl_easy_setopt(ctx->curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(ctx->curl, CURLOPT_HTTPHEADER, headers);

    CURLcode status = curl_easy_perform(ctx->curl);
    if (status != CURLE_OK) {
        err = ADDON_EINTERNAL;
        goto cleanup;
    }

    long http_code;
    curl_easy_getinfo(ctx->curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        if (http_code == 403 || http_code == 429) {
            err = ADDON_ERATE_LIMIT;
        } else if (http_code == 401) {
            err = ADDON_EUNAUTHORIZED;
        } else {
            err = ADDON_EINTERNAL;
        }
        goto cleanup;
    }

    if (res.size > 0) {
        /* write_str_cb adds a terminating null to the data. Remove it here since
         * this should be raw data. */
        res.size--;
    }

    asset->data = res.data; /* Transfer ownership to addon. */
    asset->size = res.size;
    res.data = NULL;
    res.size = 0;

cleanup:
    curl_slist_free_all(headers);
    curl_easy_reset(ctx->curl);
    free(res.data);

    return err;
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
        result->assets = list_create();
        if (result->assets == NULL) {
            list_destroy(result->dirs);
            free(result);
            return NULL;
        }

        list_set_free_fn(result->dirs, free);
        list_set_free_fn(result->assets, (ListFreeFn)asset_destroy);
    }

    return result;
}

void addon_destroy(Addon *a)
{
    if (a == NULL) {
        return;
    }

    free(a->name);
    free(a->desc);
    free(a->uri);
    free(a->version);
    list_destroy(a->dirs);
    list_destroy(a->assets);
    addon_cleanup_files(a);

    free(a);
}

Addon *addon_dup(Addon *a)
{
    Addon *result = addon_create();
    if (result) {
        ADDON_SET_NAME(result, a->name);
        ADDON_SET_DESC(result, a->desc);
        ADDON_SET_VERSION(result, a->version);
        ADDON_SET_URI(result, a->uri);
        ListNode *node = NULL;
        list_foreach(node, a->assets)
        {
            AddonAsset *orig = node->value;
            AddonAsset *new = asset_create();
            if (new == NULL) {
                addon_destroy(result);
                return NULL;
            }
            new->uri = strdup(orig->uri);
            if (new->uri == NULL) {
                asset_destroy(new);
                addon_destroy(result);
                return NULL;
            }
            if (list_insert(result->assets, new) == NULL) {
                asset_destroy(new);
                addon_destroy(result);
                return NULL;
            }
        }
    }

    return result;
}

int addon_from_json(Addon *a, const cJSON *json)
{
    cJSON *addon_name = cJSON_GetObjectItemCaseSensitive(json, ADDON_KEY_NAME);
    if (cJSON_IsString(addon_name) && addon_name->valuestring != NULL) {
        ADDON_SET_NAME(a, addon_name->valuestring);
    }
    cJSON *addon_desc = cJSON_GetObjectItemCaseSensitive(json, ADDON_KEY_DESC);
    if (cJSON_IsString(addon_desc) && addon_desc->valuestring != NULL) {
        ADDON_SET_DESC(a, addon_desc->valuestring);
    }
    cJSON *addon_uri = cJSON_GetObjectItemCaseSensitive(json, ADDON_KEY_URI);
    if (cJSON_IsString(addon_uri) && addon_uri->valuestring != NULL) {
        ADDON_SET_URI(a, addon_uri->valuestring);
    }
    cJSON *addon_version = cJSON_GetObjectItemCaseSensitive(json, ADDON_KEY_VERSION);
    if (cJSON_IsString(addon_version) && addon_version->valuestring != NULL) {
        ADDON_SET_VERSION(a, addon_version->valuestring);
    }

    cJSON *dirs = cJSON_GetObjectItemCaseSensitive(json, ADDON_KEY_DIRS);
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

    cJSON *assets = cJSON_GetObjectItemCaseSensitive(json, ADDON_KEY_ASSETS);
    if (cJSON_IsArray(assets)) {
        cJSON *asset = NULL;
        cJSON_ArrayForEach(asset, assets)
        {
            if (!cJSON_IsString(asset) || asset->valuestring == NULL) {
                continue;
            }
            AddonAsset *zip = asset_create();
            if (zip == NULL) {
                return ADDON_EINTERNAL;
            }
            zip->uri = strdup(asset->valuestring);
            list_insert(a->assets, zip);
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

    if (cJSON_AddStringToObject(json, ADDON_KEY_NAME, a->name) == NULL) {
        err = ADDON_EINTERNAL;
        goto cleanup;
    }

    if (cJSON_AddStringToObject(json, ADDON_KEY_DESC, a->desc) == NULL) {
        err = ADDON_EINTERNAL;
        goto cleanup;
    }

    if (cJSON_AddStringToObject(json, ADDON_KEY_VERSION, a->version) == NULL) {
        err = ADDON_EINTERNAL;
        goto cleanup;
    }

    if (cJSON_AddStringToObject(json, ADDON_KEY_URI, a->uri) == NULL) {
        err = ADDON_EINTERNAL;
        goto cleanup;
    }

    cJSON *dirs = cJSON_AddArrayToObject(json, ADDON_KEY_DIRS);
    if (dirs == NULL) {
        err = ADDON_EINTERNAL;
        goto cleanup;
    }
    ListNode *node = NULL;
    list_foreach(node, a->dirs)
    {
        cJSON_AddItemToArray(dirs, cJSON_CreateString((char *)node->value));
    }

    cJSON *assets = cJSON_AddArrayToObject(json, ADDON_KEY_ASSETS);
    if (assets == NULL) {
        err = ADDON_EINTERNAL;
        goto cleanup;
    }
    node = NULL;
    list_foreach(node, a->assets)
    {
        AddonAsset *asset = node->value;
        cJSON_AddItemToArray(assets, cJSON_CreateString(asset->uri));
    }

cleanup:
    if (err == ADDON_OK) {
        result = cJSON_PrintUnformatted(json);
    }

    cJSON_Delete(json);

    return result;
}

const char *addon_strerror(int errcode)
{
    static const char *str_errors[] = {
        NULL,
        /* ADDON_EBADJSON */ "could not parse JSON",
        /* ADDON_ECATALOG */ "could not get item from catalog",
        /* ADDON_ECONFIG */ "could not parse config.ini",
        /* ADDON_EINTERNAL */ "addon internal",
        /* ADDON_ENAMETOOLONG */ "path or filename too long",
        /* ADDON_ENOENT */ "no such file or directory",
        /* ADDON_ENOTFOUND */ "could not find addon",
        /* ADDON_ENO_ZIP_ASSET */ "could not find ZIP URL",
        /* ADDON_ERATE_LIMIT */ "rate limit exceeded",
        /* ADDON_EUNAUTHORIZED */ "unauthorized request",
        /* ADDON_EUNZIP */ "could not extract ZIP",
    };

    if (errcode > 0 && errcode < (int)ARRAY_SIZE(str_errors)) {
        return str_errors[errcode];
    }
    return NULL;
}

int addon_info(Addon *a, Context *ctx, const char *name)
{
    int err = ADDON_OK;

    CatalogItem item;
    err = catalog_find(&item, WOWPKG_CATALOG_PATH, name);
    if (err == CATALOG_EINVALID) {
        return ADDON_EINTERNAL;
    } else if (err == CATALOG_ENAMETOOLONG) {
        return ADDON_ENAMETOOLONG;
    } else if (err == CATALOG_ENOENT) {
        return ADDON_ENOENT;
    }
    ADDON_SET_NAME(a, item.name);
    ADDON_SET_DESC(a, item.desc);
    ADDON_SET_URI(a, item.uri);
    err = fetch_github_info(a, ctx);
    if (err != ADDON_OK) {
        return err;
    }

    return err;
}

int addon_fetch(Addon *a, Context *ctx, AddonAsset *asset)
{
    UNUSED(a);
    UNUSED(ctx);
    int err = ADDON_OK;
    if ((err = fetch_github_zip(asset, ctx)) != ADDON_OK) {
        return err;
    }
    return err;
}

int addon_package(Addon *a, Context *ctx)
{
    UNUSED(ctx);
    if (list_isempty(a->assets)) {
        return ADDON_ENO_ZIP_ASSET;
    }

    char tmpdir[OS_MAX_PATH];

    /* Creates a string with a value
     * 'path/to/temp/wowpkg_<addon_name>_<addon_version>_XXXXXX'. */
    int n = snprintf(tmpdir, ARRAY_SIZE(tmpdir), "%s%c%s_%s_%s_XXXXXX", os_tempdir(), OS_SEPARATOR, WOWPKG_NAME, a->name, a->version);
    if (n < 0 || (size_t)n >= ARRAY_SIZE(tmpdir)) {
        return ADDON_ENAMETOOLONG;
    }

    if (os_mkdtemp(tmpdir) == NULL) {
        return ADDON_EINTERNAL;
    }

    ADDON_SET_STRING(a->_package_path, tmpdir);

    ListNode *asset_node = NULL;
    list_foreach(asset_node, a->assets)
    {
        AddonAsset *asset = asset_node->value;
        if (asset->size == 0) {
            return ADDON_ENO_ZIP_ASSET;
        }
        if (zipper_extract_buf(asset->data, asset->size, tmpdir) != ZIPPER_OK) {
            return ADDON_EUNZIP;
        }
        free(asset->data);
        asset->data = NULL;
        asset->size = 0;
    }

    OsDir *dir = os_opendir(a->_package_path);
    if (dir == NULL) {
        return ADDON_ENOENT;
    }

    OsDirEnt *entry = NULL;
    while ((entry = os_readdir(dir)) != NULL) {
        if (strcmp(entry->name, ".") == 0 || strcmp(entry->name, "..") == 0) {
            continue;
        }

        list_insert(a->dirs, strdup(entry->name));
    }

    return ADDON_OK;
}

int addon_extract(Addon *a, Context *ctx, const char *path)
{
    UNUSED(ctx);
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

        //     list_insert(a->dirs, strdup(entry->name));
    }

cleanup:
    os_closedir(dir);

    return err;
}
