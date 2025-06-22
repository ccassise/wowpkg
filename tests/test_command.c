#undef NDEBUG

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "addon.h"
#include "appstate.h"
#include "command.h"
#include "config.h"
#include "context.h"
#include "list.h"
#include "osapi.h"
#include "osstring.h"
#include "term.h"
#include "wowpkg.h"

static void test_cmd_list(void)
{
    const char *state_json = "{\n"
                             "    \"installed\": [\n"
                             "        {\n"
                             "            \"name\": \"c_test_name\",\n"
                             "            \"desc\": \"c_test_desc\",\n"
                             "            \"uri\": \"c_test_uri\",\n"
                             "            \"version\": \"v7.8.9\",\n"
                             "            \"dirs\": []\n"
                             "        },\n"
                             "        {\n"
                             "            \"name\": \"a_test_name\",\n"
                             "            \"desc\": \"a_test_desc\",\n"
                             "            \"uri\": \"a_test_uri\",\n"
                             "            \"version\": \"v1.2.3\",\n"
                             "            \"dirs\": []\n"
                             "        },\n"
                             "        {\n"
                             "            \"name\": \"b_test_name\",\n"
                             "            \"desc\": \"b_test_desc\",\n"
                             "            \"uri\": \"b_test_uri\",\n"
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

    FILE *stream = tmpfile();
    assert(stream != NULL);

    const char *argv[] = { "list" };
    assert(cmd_list(&ctx, ARRAY_SIZE(argv), argv, stream) == 0);

    long actual_len = ftell(stream);
    assert(actual_len > 0);
    fseek(stream, 0, SEEK_SET);

    char *actual = malloc(sizeof(*actual) * (size_t)actual_len + 1);
    assert(actual != NULL);

    assert(fread(actual, sizeof(*actual), (size_t)actual_len, stream) == (size_t)actual_len);
    actual[actual_len] = '\0';

    /* Should be sorted. */
    assert(strcmp(actual, "a_test_name (v1.2.3)\nb_test_name (v4.5.6)\nc_test_name (v7.8.9)\n") == 0);

    fclose(stream);
    appstate_destroy(ctx.state);
    free(actual);
}

static void test_cmd_search(void)
{
    Context ctx;
    ctx.state = appstate_create();

    FILE *stream = tmpfile();
    assert(stream != NULL);

    const char *argv[] = { "search", "wigs" };
    assert(cmd_search(&ctx, ARRAY_SIZE(argv), argv, stream) == 0);

    long actual_len = ftell(stream);
    assert(actual_len > 0);
    actual_len++; /* Make room for terminating null */
    fseek(stream, 0, SEEK_SET);

    char *actual = malloc(sizeof(*actual) * (size_t)actual_len);
    assert(actual != NULL);

    /* Output should be sorted. */
    assert(fgets(actual, (int)actual_len, stream) != NULL);
    assert(strstr(actual, "==>") != NULL);
    assert(strstr(actual, "BigWigs") != NULL);

    assert(fgets(actual, (int)actual_len, stream) != NULL);
    assert(strstr(actual, "Name:") != NULL);
    assert(strstr(actual, "BigWigs") != NULL);

    assert(fgets(actual, (int)actual_len, stream) != NULL);
    assert(strstr(actual, "URI:") != NULL);
    assert(strstr(actual, "https://api.github.com") != NULL);

    assert(fgets(actual, (int)actual_len, stream) != NULL);
    assert(strstr(actual, "Description:") != NULL);
    // assert(os_strcasestr(actual, "wigs") != NULL);

    assert(fgets(actual, (int)actual_len, stream) != NULL);
    assert(strstr(actual, "==>") != NULL);
    assert(strstr(actual, "BigWigs_Voice") != NULL);

    assert(fgets(actual, (int)actual_len, stream) != NULL);
    assert(strstr(actual, "Name:") != NULL);
    assert(strstr(actual, "BigWigs_Voice") != NULL);

    assert(fgets(actual, (int)actual_len, stream) != NULL);
    assert(strstr(actual, "URI:") != NULL);
    assert(strstr(actual, "https://api.github.com") != NULL);

    assert(fgets(actual, (int)actual_len, stream) != NULL);
    assert(strstr(actual, "Description:") != NULL);
    assert(os_strcasestr(actual, "wigs") != NULL);

    assert(fgets(actual, (int)actual_len, stream) != NULL);
    assert(strstr(actual, "==>") != NULL);
    assert(strstr(actual, "LittleWigs") != NULL);

    assert(fgets(actual, (int)actual_len, stream) != NULL);
    assert(strstr(actual, "Name:") != NULL);
    assert(strstr(actual, "LittleWigs") != NULL);

    assert(fgets(actual, (int)actual_len, stream) != NULL);
    assert(strstr(actual, "URI:") != NULL);
    assert(strstr(actual, "https://api.github.com") != NULL);

    assert(fgets(actual, (int)actual_len, stream) != NULL);
    assert(strstr(actual, "Description:") != NULL);
    // assert(os_strcasestr(actual, "wigs") != NULL);

    assert(fgets(actual, (int)actual_len, stream) == NULL);

    appstate_destroy(ctx.state);
    fclose(stream);
    free(actual);
}

static void test_cmd_search_none(void)
{
    Context ctx;
    ctx.state = appstate_create();

    FILE *stream = tmpfile();
    assert(stream != NULL);

    const char *argv[] = { "search", "___not_found___" };
    assert(cmd_search(&ctx, ARRAY_SIZE(argv), argv, stream) == 0);

    long actual_len = ftell(stream);
    assert(actual_len == 0);

    appstate_destroy(ctx.state);
    fclose(stream);
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

    ADDON_SET_NAME(installed, (const char *)"MockAddon");
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

    appstate_destroy(ctx.state);
    config_destroy(ctx.config);
    os_remove_all(outdir);
}

static void test_cmd_outdated(void)
{
    Addon *addon1 = addon_create();
    Addon *addon2 = addon_create();
    Addon *addon3 = addon_create();

    ADDON_SET_NAME(addon1, (const char *)"AddonOne");
    ADDON_SET_VERSION(addon1, (const char *)"v1.2.3");
    ADDON_SET_NAME(addon2, (const char *)"AddonTwo");
    ADDON_SET_VERSION(addon2, (const char *)"v4.5.6");
    ADDON_SET_NAME(addon3, (const char *)"AddonThree");
    ADDON_SET_VERSION(addon3, (const char *)"19700101.1");

    Addon *addon1_latest = addon_dup(addon1);
    Addon *addon2_latest = addon_dup(addon2);
    Addon *addon3_latest = addon_dup(addon3);

    ADDON_SET_VERSION(addon1_latest, (const char *)"v1.2.5");
    ADDON_SET_VERSION(addon2_latest, (const char *)"v5.6.7");
    ADDON_SET_VERSION(addon3_latest, (const char *)"20200809.5");

    Context ctx;
    memset(&ctx, 0, sizeof(ctx));

    ctx.state = appstate_create();
    list_insert(ctx.state->installed, addon1);
    list_insert(ctx.state->installed, addon2);
    list_insert(ctx.state->installed, addon3);

    list_insert(ctx.state->latest, addon1_latest);
    list_insert(ctx.state->latest, addon2_latest);
    list_insert(ctx.state->latest, addon3_latest);

    FILE *stream = tmpfile();
    const char *argv[] = { "outdated" };

    assert(cmd_outdated(&ctx, ARRAY_SIZE(argv), argv, stream) == 0);

    long actual_len = ftell(stream);
    assert(actual_len > 0);
    fseek(stream, 0, SEEK_SET);

    char *actual = malloc(sizeof(*actual) * (size_t)actual_len + 1);
    assert(actual != NULL);

    assert(fread(actual, sizeof(*actual), (size_t)actual_len, stream) == (size_t)actual_len);
    actual[actual_len] = '\0';

    /* Should be sorted. */
    assert(strcmp(actual, "AddonOne (v1.2.3) < (v1.2.5)\nAddonThree (19700101.1) < (20200809.5)\nAddonTwo (v4.5.6) < (v5.6.7)\n") == 0);

    fclose(stream);
    free(actual);
    appstate_destroy(ctx.state);
}

int main(void)
{
    test_cmd_list();
    test_cmd_search();
    test_cmd_search_none();
    test_cmd_remove();
    test_cmd_outdated();

    return 0;
}
