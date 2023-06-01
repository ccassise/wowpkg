#include <assert.h>
#include <direct.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>

#include "osapi.h"

#define ARRLEN(a) (sizeof(a) / sizeof((a)[0]))

static void test_os_mkdir(void)
{
    int err = os_mkdir("../../test_osapi_tmp", 0755);
    assert(err == 0);

    struct _stat s;
    err = _stat("../../test_osapi_tmp", &s);
    assert(err == 0);

    assert(s.st_mode & S_IFDIR);

    err = _rmdir("../../test_osapi_tmp");
    assert(err == 0);
}

static void test_os_mkdir_all(void)
{
    int err = os_mkdir_all("../../test_osapi_tmp/test_subdir/test.txt", 0755);
    assert(err == 0);

    struct _stat s;
    err = _stat("../../test_osapi_tmp", &s);
    assert(err == 0);
    assert(s.st_mode & S_IFDIR);

    err = _stat("../../test_osapi_tmp/test_subdir", &s);
    assert(err == 0);
    assert(s.st_mode & S_IFDIR);

    err = _stat("../../test_osapi_tmp/test_subsir/test.txt", &s);
    assert(err != 0);

    err = _rmdir("../../test_osapi_tmp/test_subdir");
    assert(err == 0);

    err = _rmdir("../../test_osapi_tmp");
    assert(err == 0);
}

#ifdef _WIN32
static void test_os_mkdir_all_win32(void)
{
    int err = os_mkdir_all("..\\..\\test_osapi_tmp\\test_subdir\\test.txt", 0755);
    assert(err == 0);

    struct _stat s;
    err = _stat("..\\..\\test_osapi_tmp", &s);
    assert(err == 0);
    assert(s.st_mode & S_IFDIR);

    err = _stat("..\\..\\test_osapi_tmp\\test_subdir", &s);
    assert(err == 0);
    assert(s.st_mode & S_IFDIR);

    err = _stat("..\\..\\test_osapi_tmp\\test_subsir\\test.txt", &s);
    assert(err != 0);

    err = _rmdir("..\\..\\test_osapi_tmp\\test_subdir");
    assert(err == 0);

    err = _rmdir("..\\..\\test_osapi_tmp");
    assert(err == 0);
}
#endif

static void test_os_readdir(void)
{
    OsDir *dir = os_opendir("../../test/mocks/mock_dir");
    OsDirEnt *entry = NULL;

    const char *expect[] = {
        ".",
        "..",
        "test.txt",
        "test_a",
        "test_b",
        "test_c",
    };

    bool expect_was_found[ARRLEN(expect)] = { false };

    // Since order is not gauranteed from os_readdir we just look that every
    // entry in expect appeared exactly once.
    while ((entry = os_readdir(dir)) != NULL) {
        char *actual = entry->name;
        bool found = false;
        for (size_t i = 0; i < ARRLEN(expect); i++) {
            if (strcmp(actual, expect[i]) == 0) {
                assert(!expect_was_found[i]);
                expect_was_found[i] = true;
                found = true;
                break;
            }
        }
        assert(found);
    }

    for (size_t i = 0; i < ARRLEN(expect); i++) {
        assert(expect_was_found[i]);
    }

    os_closedir(dir);
}

static void test_os_remove_all(void)
{
    assert(os_mkdir_all("../../test/test_osapi_tmp/dir1/dir2/dir3/", 0755) == 0);

    FILE *dir3txt = fopen("../../test/test_osapi_tmp/dir1/dir2/dir3/dir3.txt", "wb");
    const char *dir3txtstr = "dir3.txt\n";
    assert(fwrite(dir3txtstr, sizeof(*dir3txtstr), strlen(dir3txtstr), dir3txt) > 0);
    fclose(dir3txt);

    FILE *dir2txt = fopen("../../test/test_osapi_tmp/dir1/dir2/dir2.txt", "wb");
    const char *dir2txtstr = "dir3.txt\n";
    assert(fwrite(dir2txtstr, sizeof(*dir2txtstr), strlen(dir2txtstr), dir2txt) > 0);
    fclose(dir2txt);

    assert(os_remove_all("../../test/test_osapi_tmp") == 0);

    struct _stat s;
    assert(_stat("../../test/test_osapi_tmp", &s) != 0);
}

int main(void)
{
    test_os_mkdir();
    test_os_mkdir_all();
    test_os_readdir();
    test_os_remove_all();

#ifdef _WIN32
    test_os_mkdir_all_win32();
#endif
}
