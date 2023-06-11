#include <assert.h>
#include <stdlib.h>

#include "config.h"
#include "osstring.h"

static void test_config_to_json(void)
{
    Config *cfg = config_create();
    cfg->addon_path = strdup("test/addon/path");

    char *actual = config_to_json(cfg);

    assert(strcmp(actual, "{\"addon_path\":\"test/addon/path\"}") == 0);

    free(actual);
    config_free(cfg);
}

static void test_config_from_json(void)
{
    Config *cfg = config_create();
    cfg->addon_path = strdup("test/addon/path");

    char *actual = config_to_json(cfg);

    config_free(cfg);

    cfg = config_create();
    assert(config_from_json(cfg, actual) == 0);

    assert(strcmp(cfg->addon_path, "test/addon/path") == 0);

    free(actual);
    config_free(cfg);
}

int main(void)
{
    test_config_to_json();
    test_config_from_json();

    return 0;
}
