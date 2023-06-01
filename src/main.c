#include <stdio.h>
#include <time.h>

#include <cjson/cJSON.h>
#include <curl/curl.h>

// #include <windows.h>

#include "addon.h"

int main(int argc, const char *argv[])
{
    (void)argc;
    (void)argv;
    clock_t start = clock();

    // char *tmp = _tempnam(NULL, "myfile");
    // printf("%s.zip\n", tmp);
    // free(tmp);
    Addon bigwigs;
    // Addon plater;
    Addon weakauras;
    Addon notfound;
    memset(&bigwigs, 0, sizeof(bigwigs));
    // memset(&plater, 0, sizeof(plater));
    memset(&weakauras, 0, sizeof(weakauras));
    memset(&notfound, 0, sizeof(notfound));

    // addon_fetch_metadata(&weakauras, "weakauras");
    int err = 0;
    cJSON *meta = addon_metadata_from_catalog("weakauras", &err);
    if (meta == NULL) {
        fprintf(stderr, "error: Failed to get addon metadata from catalog (%d)\n", err);
    }
    addon_from_json(&weakauras, meta);

    meta = addon_metadata_from_github(weakauras.url, &err);
    if (meta == NULL) {
        fprintf(stderr, "error: Failed to get addon metadata from Github (%d)\n", err);
    }
    addon_from_json(&weakauras, meta);

    if ((err = addon_package(&weakauras)) != ADDON_OK) {
        fprintf(stderr, "error: addon_package (%d)\n", err);
        exit(1);
    }

    if ((err = addon_extract(&weakauras)) != ADDON_OK) {
        fprintf(stderr, "error: addon_extract (%d)\n", err);
        exit(1);
    }

    // addon_fetch_metadata(&plater, "PLATER");
    // addon_fetch_metadata(&weakauras, "WeAkAuRaS");
    // addon_package(&weakauras);
    // if (addon_fetch_metadata(&notfound, "not_found") != ADDON_ENOTFOUND) {
    //     fprintf(stderr, "error: SHOULD NOT BE FOUND BUT IT IS\n");
    // }

    // addon_print(&bigwigs, stdout);
    // // addon_print(&plater, stdout);
    addon_print(&weakauras, stdout);

    addon_free(&bigwigs);
    // addon_free(&plater);
    addon_free(&weakauras);
    addon_free(&notfound);

    clock_t end = clock();
    printf("Complete in: %.2f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    return 0;
}
