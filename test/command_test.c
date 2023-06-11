#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_state.h"
#include "command.h"

static void test_cmd_list(void)
{
    const char *filename = "../../dev_only/test_cmd_list.txt";
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

    FILE *out = fopen(filename, "w+b");
    assert(out != NULL);

    const char *argv[] = { "list" };
    assert(cmd_list(&ctx, 1, argv, out) == 0);

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
    remove(filename);
}

static void test_cmd_search(void)
{
    const char *filename = "../../dev_only/test_cmd_search.txt";

    FILE *out = fopen(filename, "w+b");
    assert(out != NULL);

    const char *argv[] = { "search", "wigs" };
    assert(cmd_search(NULL, 2, argv, out) == 0);

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
    remove(filename);
}

int main(void)
{
    test_cmd_list();
    test_cmd_search();

    return 0;
}
