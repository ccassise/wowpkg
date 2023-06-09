#include <assert.h>
#include <stdbool.h>

#include "osapi.h"
#include "osstring.h"
#include "wowpkg.h"
#include "zipper.h"

static void test_zipper_unzip(const char *outpath)
{
    assert(zipper_unzip(WOWPKG_TEST_DIR "/mocks/mock_zip.zip", outpath) == ZIPPER_ENOENT);

    assert(os_mkdir(outpath, 0755) == 0);

    assert(zipper_unzip(WOWPKG_TEST_DIR "/mocks/mock_zip.zip", outpath) == ZIPPER_OK);

    OsDir *dir = os_opendir(outpath);
    assert(dir != NULL);

    OsDirEnt *entry = NULL;

    const char *expect[] = {
        "mock_dir_a",
        "mock_dir_b",
        "mock_dir_c",
    };

    bool expect_found[ARRAY_SIZE(expect)] = { false };

    while ((entry = os_readdir(dir)) != NULL) {
        for (size_t i = 0; i < ARRAY_SIZE(expect); i++) {
            const char *actual = entry->name;
            if (strcmp(expect[i], actual) == 0) {
                assert(!expect_found[i]);
                expect_found[i] = true;
            }
        }
    }

    for (size_t i = 0; i < ARRAY_SIZE(expect); i++) {
        assert(expect_found[i]);
    }

    os_closedir(dir);

    char mock_dir[OS_MAX_PATH];

    snprintf(mock_dir, ARRAY_SIZE(mock_dir), "%s%c%s", outpath, OS_SEPARATOR, "mock_dir_a");
    dir = os_opendir(mock_dir);
    assert(dir != NULL);

    size_t filecount = 0;
    while ((entry = os_readdir(dir)) != NULL) {
        if (strcmp(".", entry->name) == 0 || strcmp("..", entry->name) == 0) {
            continue;
        }

        assert(strcmp(entry->name, "mock_dir_a.txt") == 0);
        filecount++;
    }
    assert(filecount == 1);

    os_closedir(dir);

    snprintf(mock_dir, ARRAY_SIZE(mock_dir), "%s%c%s", outpath, OS_SEPARATOR, "mock_dir_b");
    dir = os_opendir(mock_dir);
    assert(dir != NULL);

    filecount = 0;
    while ((entry = os_readdir(dir)) != NULL) {
        if (strcmp(".", entry->name) == 0 || strcmp("..", entry->name) == 0) {
            continue;
        }

        assert(strcmp(entry->name, "mock_dir_b.txt") == 0);
        filecount++;
    }
    assert(filecount == 1);

    os_closedir(dir);

    assert(os_remove_all(outpath) == 0);
}

int main(void)
{
    // Ensure previous runs don't affect this run.
    os_remove_all(WOWPKG_TEST_TMPDIR "test_tmp/");

    test_zipper_unzip(WOWPKG_TEST_TMPDIR "test_tmp");
    test_zipper_unzip(WOWPKG_TEST_TMPDIR "test_tmp/");

    return 0;
}
