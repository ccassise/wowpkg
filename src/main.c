#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <curl/curl.h>

// #include <windows.h>

#include "addon.h"
#include "cjson/cJSON.h"

int main(int argc, const char *argv[])
{
    (void)argc;
    (void)argv;

    int result = 0;
    clock_t start = clock();

    cJSON *catalog_meta = NULL;
    cJSON *gh_meta = NULL;
    Addon *bigwigs = addon_create();
    Addon *weakauras = addon_create();

    int err = 0;
    catalog_meta = addon_metadata_from_catalog("wEaKauRas", &err);
    if (catalog_meta == NULL) {
        fprintf(stderr, "error: Failed to get addon metadata from catalog (%d)\n", err);
        result = 1;
        goto end;
    }
    addon_from_json(weakauras, catalog_meta);

    gh_meta = addon_metadata_from_github(weakauras->url, &err);
    if (gh_meta == NULL) {
        fprintf(stderr, "error: Failed to get addon metadata from Github (%d)\n", err);
        result = 1;
        goto end;
    }
    addon_from_json(weakauras, gh_meta);

    if ((err = addon_package(weakauras)) != ADDON_OK) {
        fprintf(stderr, "error: addon_package (%d)\n", err);
        result = 1;
        goto end;
    }

    if ((err = addon_extract(weakauras)) != ADDON_OK) {
        fprintf(stderr, "error: addon_extract (%d)\n", err);
        result = 1;
        goto end;
    }

    addon_package(weakauras);
    if (addon_metadata_from_catalog("not found", &err) != NULL) {
        fprintf(stderr, "error: SHOULD NOT BE FOUND BUT IT IS\n");
    }

    char *output = addon_to_json(weakauras);
    printf("%s\n", output);
    free(output);

    printf("Press enter to continue...\n");
    getchar();

end:
    if (catalog_meta != NULL) {
        cJSON_Delete(catalog_meta);
    }

    if (gh_meta != NULL) {
        cJSON_Delete(gh_meta);
    }

    addon_free(bigwigs);
    addon_free(weakauras);

    clock_t end = clock();
    printf("Complete in: %.2f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    return result;
}
