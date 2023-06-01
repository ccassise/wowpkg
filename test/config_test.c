#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

static void test_config_to_json(void)
{
    Config cfg;
    cfg.addon_path = "test/addon/path";

    char *actual = config_to_json(&cfg);

    assert(strcmp(actual, "{\"addon_path\":\"test/addon/path\"}") == 0);

    free(actual);
}

static void test_config_from_json(void)
{
    Config cfg;
    cfg.addon_path = "test/addon/path";

    char *actual = config_to_json(&cfg);

    cfg.addon_path = NULL;
    assert(config_from_json(&cfg, actual) == 0);

    assert(strcmp(cfg.addon_path, "test/addon/path") == 0);

    free(actual);
    config_free(&cfg);
}

int main(void)
{
    test_config_to_json();
    test_config_from_json();

    return 0;
}
