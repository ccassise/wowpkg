#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "addon.h"
#include "command.h"
#include "context.h"
#include "osapi.h"
#include "osstring.h"
#include "wowpkg.h"

static void test_cmd_list(void)
{
    const char *state_json = "{\n"
                             "    \"installed\": [\n"
                             "        {\n"
                             "            \"name\": \"c_test_name\",\n"
                             "            \"desc\": \"c_test_desc\",\n"
                             "            \"url\": \"c_test_url\",\n"
                             "            \"version\": \"v7.8.9\",\n"
                             "            \"dirs\": []\n"
                             "        },\n"
                             "        {\n"
                             "            \"name\": \"a_test_name\",\n"
                             "            \"desc\": \"a_test_desc\",\n"
                             "            \"url\": \"a_test_url\",\n"
                             "            \"version\": \"v1.2.3\",\n"
                             "            \"dirs\": []\n"
                             "        },\n"
                             "        {\n"
                             "            \"name\": \"b_test_name\",\n"
                             "            \"desc\": \"b_test_desc\",\n"
                             "            \"url\": \"b_test_url\",\n"
                             "            \"version\": \"v4.5.6\",\n"
                             "            \"dirs\": []\n"
                             "        }\n"
                             "    ],\n"
                             "    \"latest\": []\n"
                             "}\n";

    Context ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.state = appstate_create();
    assert(appstate_from_json(ctx.state, state_json) == 0);

    FILE *out = tmpfile();
    assert(out != NULL);

    const char *argv[] = { "list" };
    assert(cmd_list(&ctx, ARRAY_SIZE(argv), argv, out) == 0);

    long out_len = ftell(out);
    assert(out_len > 0);
    fseek(out, 0, SEEK_SET);

    char *out_str = malloc(sizeof(*out_str) * (size_t)out_len + 1);
    assert(out_str != NULL);

    assert(fread(out_str, sizeof(*out_str), (size_t)out_len, out) == (size_t)out_len);
    out_str[out_len] = '\0';

    // Should be sorted.
    assert(strcmp(out_str, "a_test_name (v1.2.3)\nb_test_name (v4.5.6)\nc_test_name (v7.8.9)\n") == 0);

    fclose(out);
    appstate_free(ctx.state);
    free(out_str);
}

static void test_cmd_search(void)
{
    FILE *out = tmpfile();
    assert(out != NULL);

    const char *argv[] = { "search", "wigs" };
    assert(cmd_search(NULL, ARRAY_SIZE(argv), argv, out) == 0);

    long out_len = ftell(out);
    assert(out_len > 0);
    fseek(out, 0, SEEK_SET);

    char *out_str = malloc(sizeof(*out_str) * (size_t)out_len + 1);
    assert(out_str != NULL);

    assert(fread(out_str, sizeof(*out_str), (size_t)out_len, out) == (size_t)out_len);
    out_str[out_len] = '\0';

    // Should be sorted.
    assert(strcmp(out_str, "BigWigs\nBigWigs_Voice\nLittleWigs\n") == 0);

    fclose(out);
    free(out_str);
}

static void test_cmd_remove(void)
{
    Context ctx;
    memset(&ctx, 0, sizeof(ctx));

    ctx.state = appstate_create();
    ctx.config = config_create();

    const char outdir[] = WOWPKG_TEST_TMPDIR "test_cmd_remove/";
    const char outdir_test_a[] = WOWPKG_TEST_TMPDIR "test_cmd_remove/test_a";
    const char outdir_test_b[] = WOWPKG_TEST_TMPDIR "test_cmd_remove/test_b";
    const char outdir_test_c[] = WOWPKG_TEST_TMPDIR "test_cmd_remove/test_c";
    char outdir_test_a_txt[] = WOWPKG_TEST_TMPDIR "test_cmd_remove/test_a/test_a.txt";
    char outdir_test_b_txt[] = WOWPKG_TEST_TMPDIR "test_cmd_remove/test_b/test_b.txt";
    char outdir_test_c_txt[] = WOWPKG_TEST_TMPDIR "test_cmd_remove/test_c/test_c.txt";

    ctx.config->addons_path = strdup(outdir);

    assert(os_mkdir_all(outdir_test_a_txt, 0755) == 0);
    assert(os_mkdir_all(outdir_test_b_txt, 0755) == 0);
    assert(os_mkdir_all(outdir_test_c_txt, 0755) == 0);

    FILE *ftest_a = fopen(outdir_test_a_txt, "wb");
    assert(ftest_a != NULL);
    FILE *ftest_b = fopen(outdir_test_b_txt, "wb");
    assert(ftest_b != NULL);
    FILE *ftest_c = fopen(outdir_test_c_txt, "wb");
    assert(ftest_c != NULL);

    const char *test_a_txt_contents = "test_a.txt\n";
    const char *test_b_txt_contents = "test_b.txt\n";
    const char *test_c_txt_contents = "test_c.txt\n";

    assert(fwrite(test_a_txt_contents, sizeof(*test_a_txt_contents), strlen(test_a_txt_contents), ftest_a) == strlen(test_a_txt_contents));
    assert(fwrite(test_b_txt_contents, sizeof(*test_b_txt_contents), strlen(test_b_txt_contents), ftest_b) == strlen(test_b_txt_contents));
    assert(fwrite(test_c_txt_contents, sizeof(*test_c_txt_contents), strlen(test_c_txt_contents), ftest_c) == strlen(test_c_txt_contents));

    fclose(ftest_a);
    fclose(ftest_b);
    fclose(ftest_c);

    struct os_stat s;

    assert(os_stat(outdir_test_a_txt, &s) == 0);
    assert(os_stat(outdir_test_b_txt, &s) == 0);
    assert(os_stat(outdir_test_c_txt, &s) == 0);

    Addon *installed = addon_create();
    assert(installed != NULL);

    installed->name = strdup("MockAddon");
    list_insert(installed->dirs, strdup("test_a"));
    list_insert(installed->dirs, strdup("test_b"));
    list_insert(installed->dirs, strdup("test_c"));
    list_insert(ctx.state->installed, installed);

    Addon *latest = addon_dup(installed);
    assert(latest != NULL);

    list_insert(ctx.state->latest, latest);

    const char *argv[] = { "remove", "mockaddon" };
    assert(cmd_remove(&ctx, ARRAY_SIZE(argv), argv, stdout) == 0);

    assert(list_isempty(ctx.state->installed));
    assert(list_isempty(ctx.state->latest));

    assert(os_stat(outdir_test_a, &s) != 0);
    assert(os_stat(outdir_test_b, &s) != 0);
    assert(os_stat(outdir_test_c, &s) != 0);

    appstate_free(ctx.state);
    config_free(ctx.config);
    os_remove_all(outdir);
}

static void test_cmd_outdated(void)
{
    Addon *addon1 = addon_create();
    Addon *addon2 = addon_create();
    Addon *addon3 = addon_create();

    addon_set_str(&addon1->name, strdup("AddonOne"));
    addon_set_str(&addon1->version, strdup("v1.2.3"));
    addon_set_str(&addon2->name, strdup("AddonTwo"));
    addon_set_str(&addon2->version, strdup("v4.5.6"));
    addon_set_str(&addon3->name, strdup("AddonThree"));
    addon_set_str(&addon3->version, strdup("19700101.1"));

    Addon *addon1_latest = addon_dup(addon1);
    Addon *addon2_latest = addon_dup(addon2);
    Addon *addon3_latest = addon_dup(addon3);

    addon_set_str(&addon1_latest->version, strdup("v1.2.5"));
    addon_set_str(&addon2_latest->version, strdup("v5.6.7"));
    addon_set_str(&addon3_latest->version, strdup("20200809.5"));

    Context ctx;
    memset(&ctx, 0, sizeof(ctx));

    ctx.state = appstate_create();
    list_insert(ctx.state->installed, addon1);
    list_insert(ctx.state->installed, addon2);
    list_insert(ctx.state->installed, addon3);

    list_insert(ctx.state->latest, addon1_latest);
    list_insert(ctx.state->latest, addon2_latest);
    list_insert(ctx.state->latest, addon3_latest);

    FILE *out = tmpfile();

    const char *argv[] = { "outdated" };
    assert(cmd_outdated(&ctx, ARRAY_SIZE(argv), argv, out) == 0);

    long out_len = ftell(out);
    assert(out_len > 0);
    fseek(out, 0, SEEK_SET);

    char *out_str = malloc(sizeof(*out_str) * (size_t)out_len + 1);
    assert(out_str != NULL);

    assert(fread(out_str, sizeof(*out_str), (size_t)out_len, out) == (size_t)out_len);
    out_str[out_len] = '\0';

    // Should be sorted.
    assert(strcmp(out_str, "AddonOne (v1.2.3) < (v1.2.5)\nAddonThree (19700101.1) < (20200809.5)\nAddonTwo (v4.5.6) < (v5.6.7)\n") == 0);

    fclose(out);
    free(out_str);
    appstate_free(ctx.state);
}

static void test_cmd_info(void)
{
    Context ctx;
    ctx.state = appstate_create();

    Addon *addon = addon_create();
    addon->name = strdup("Simulationcraft");
    addon->url = strdup("zip_url");
    addon->version = strdup("v1.2.3");

    list_insert(ctx.state->installed, addon); // Transfer ownership of addon to ctx.state.

    FILE *out = tmpfile();
    assert(out != NULL);

    const char *argv[] = { "info", "plater", "___not_found___", "SIMULATIONCRAFT" };
    assert(cmd_info(&ctx, ARRAY_SIZE(argv), argv, out) == 0);

    long out_len = ftell(out);
    assert(out_len > 0);
    fseek(out, 0, SEEK_SET);

    char *actual = malloc(sizeof(*actual) * (size_t)out_len + 1);
    assert(actual != NULL);

    assert(fread(actual, sizeof(*actual), (size_t)out_len, out) == (size_t)out_len);
    actual[out_len] = '\0';

    const char *expect = ("==> Plater\n"
                          "Description:     Nameplate addon designed for advanced users.\n"
                          "From:            https://api.github.com/repos/Tercioo/Plater-Nameplates/releases/latest\n"
                          "Installed:       No\n"
                          "==> Simulationcraft\n"
                          "Description:     Constructs SimC export strings\n"
                          "From:            https://api.github.com/repos/simulationcraft/simc-addon/releases/latest\n"
                          "Installed:       Yes\n"
                          "Version:         v1.2.3\n"
                          "ZIP:             zip_url\n");

    assert(strcmp(expect, actual) == 0);

    appstate_free(ctx.state);
    fclose(out);
    free(actual);
}

int main(void)
{
    test_cmd_list();
    test_cmd_search();
    test_cmd_remove();
    test_cmd_outdated();
    test_cmd_info();

    return 0;
}
