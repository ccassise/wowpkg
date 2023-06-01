#include <assert.h>
#include <stdbool.h>

#include "osapi.h"
#include "zipper.h"

#define ARRLEN(a) (sizeof(a) / sizeof((a)[0]))

static void test_zipper_unzip(const char *outpath)
{
    assert(zipper_unzip(outpath, "../../test/mocks/mock_zip.zip") == ZIPPER_ENOENT);

    assert(os_mkdir(outpath, 0755) == 0);

    int err = zipper_unzip(outpath, "../../test/mocks/mock_zip.zip");
    assert(err == ZIPPER_OK);

    OsDir *dir = os_opendir(outpath);
    assert(dir != NULL);

    OsDirEnt *entry = NULL;

    const char *expect[] = {
        "mock_dir_a",
        "mock_dir_b",
        "mock_dir_c",
    };

    bool expect_found[ARRLEN(expect)] = { false };

    while ((entry = os_readdir(dir)) != NULL) {
        for (size_t i = 0; i < ARRLEN(expect); i++) {
            const char *actual = entry->name;
            if (strcmp(expect[i], actual) == 0) {
                assert(!expect_found[i]);
                expect_found[i] = true;
            }
        }
    }

    for (size_t i = 0; i < ARRLEN(expect); i++) {
        assert(expect_found[i]);
    }

    os_closedir(dir);

    char mock_dir[MAX_PATH];

    snprintf(mock_dir, MAX_PATH, "%s%c%s", outpath, OS_SEPARATOR, "mock_dir_a");
    dir = os_opendir(mock_dir);
    assert(dir != NULL);

    while ((entry = os_readdir(dir)) != NULL) {
        if (strcmp(".", entry->name) == 0 || strcmp("..", entry->name) == 0) {
            continue;
        }

        assert(strcmp(entry->name, "mock_dir_a.txt") == 0);
        assert(os_readdir(dir) == NULL);
    }

    os_closedir(dir);

    snprintf(mock_dir, MAX_PATH, "%s%c%s", outpath, OS_SEPARATOR, "mock_dir_b");
    dir = os_opendir(mock_dir);
    assert(dir != NULL);

    while ((entry = os_readdir(dir)) != NULL) {
        if (strcmp(".", entry->name) == 0 || strcmp("..", entry->name) == 0) {
            continue;
        }

        assert(strcmp(entry->name, "mock_dir_b.txt") == 0);
        assert(os_readdir(dir) == NULL);
    }

    os_closedir(dir);

    assert(os_remove_all(outpath) == 0);
}

int main(void)
{
    test_zipper_unzip("../../test/test_tmp");
    test_zipper_unzip("../../test/test_tmp/");

    return 0;
}
