#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "addon.h"
#include "command.h"
#include "context.h"
#include "osapi.h"
#include "osstring.h"

#define ARRLEN(a) (sizeof(a) / sizeof(*(a)))

static void test_cmd_list(void)
{
    const char *state_json = "{\n"
                             "    \"installed\": [\n"
                             "        {\n"
                             "            \"handler\": \"c_test_handler\",\n"
                             "            \"name\": \"c_test_name\",\n"
                             "            \"desc\": \"c_test_desc\",\n"
                             "            \"url\": \"c_test_url\",\n"
                             "            \"version\": \"v7.8.9\",\n"
                             "            \"dirs\": []\n"
                             "        },\n"
                             "        {\n"
                             "            \"handler\": \"a_test_handler\",\n"
                             "            \"name\": \"a_test_name\",\n"
                             "            \"desc\": \"a_test_desc\",\n"
                             "            \"url\": \"a_test_url\",\n"
                             "            \"version\": \"v1.2.3\",\n"
                             "            \"dirs\": []\n"
                             "        },\n"
                             "        {\n"
                             "            \"handler\": \"b_test_handler\",\n"
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
    assert(cmd_list(&ctx, ARRLEN(argv), argv, out) == 0);

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
    assert(cmd_search(NULL, ARRLEN(argv), argv, out) == 0);

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

static int cmp_str_to_addon(const void *a, const void *b)
{
    const char *str = a;
    const Addon *addon = b;

    return strcasecmp(str, addon->name);
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

    ctx.config->addon_path = strdup(outdir);

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

    char *argv[] = { "remove", "mockaddon" };
    assert(cmd_remove(&ctx, ARRLEN(argv), argv, stderr) == 0);

    assert(list_isempty(ctx.state->installed));
    assert(list_isempty(ctx.state->latest));

    assert(os_stat(outdir_test_a, &s) != 0);
    assert(os_stat(outdir_test_b, &s) != 0);
    assert(os_stat(outdir_test_c, &s) != 0);

    appstate_free(ctx.state);
    config_free(ctx.config);
    os_remove_all(outdir);
}

int main(void)
{
    test_cmd_list();
    test_cmd_search();
    test_cmd_remove();

    return 0;
}
