#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#endif

#include "osapi.h"
#include "wowpkg.h"

static void test_os_mkdir(void)
{
    int err = os_mkdir(WOWPKG_TEST_TMPDIR "test_osapi_tmp", 0755);
    assert(err == 0);

    struct os_stat s;
    err = os_stat(WOWPKG_TEST_TMPDIR "test_osapi_tmp", &s);
    assert(err == 0);

    assert(S_ISDIR(s.st_mode));

    err = os_rmdir(WOWPKG_TEST_TMPDIR "test_osapi_tmp");
    assert(err == 0);
}

static void test_os_mkdir_all(void)
{
    char path[OS_MAX_PATH] = WOWPKG_TEST_TMPDIR "test_osapi_tmp/test_subdir/test.txt";

    int err = os_mkdir_all(path, 0755);
    assert(err == 0);

    struct os_stat s;
    err = os_stat(WOWPKG_TEST_TMPDIR "test_osapi_tmp", &s);
    assert(err == 0);
    assert(S_ISDIR(s.st_mode));

    err = os_stat(WOWPKG_TEST_TMPDIR "test_osapi_tmp/test_subdir", &s);
    assert(err == 0);
    assert(S_ISDIR(s.st_mode));

    err = os_stat(WOWPKG_TEST_TMPDIR "test_osapi_tmp/test_subsir/test.txt", &s);
    assert(err != 0);

    err = os_rmdir(WOWPKG_TEST_TMPDIR "test_osapi_tmp/test_subdir");
    assert(err == 0);

    err = os_rmdir(WOWPKG_TEST_TMPDIR "test_osapi_tmp");
    assert(err == 0);
}

#ifdef _WIN32
static void test_os_mkdir_all_win32(void)
{
    char path[OS_MAX_PATH] = WOWPKG_TEST_TMPDIR "test_osapi_tmp\\test_subdir\\test.txt";

    int err = os_mkdir_all(path, 0755);
    assert(err == 0);

    struct os_stat s;
    err = os_stat(WOWPKG_TEST_TMPDIR "test_osapi_tmp", &s);
    assert(err == 0);
    assert(S_ISDIR(s.st_mode));

    err = os_stat(WOWPKG_TEST_TMPDIR "test_osapi_tmp\\test_subdir", &s);
    assert(err == 0);
    assert(S_ISDIR(s.st_mode));

    err = os_stat(WOWPKG_TEST_TMPDIR "test_osapi_tmp\\test_subsir\\test.txt", &s);
    assert(err != 0);

    err = os_rmdir(WOWPKG_TEST_TMPDIR "test_osapi_tmp\\test_subdir");
    assert(err == 0);

    err = os_rmdir(WOWPKG_TEST_TMPDIR "test_osapi_tmp");
    assert(err == 0);
}
#endif

static void test_os_readdir(void)
{
    OsDir *dir = os_opendir(WOWPKG_TEST_DIR "/mocks/mock_dir");
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

    assert(dir != NULL);

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
    char path[OS_MAX_PATH] = WOWPKG_TEST_TMPDIR "test_osapi_tmp/dir1/dir2/dir3/";

    assert(os_mkdir_all(path, 0755) == 0);

    FILE *dir3txt = fopen(WOWPKG_TEST_TMPDIR "test_osapi_tmp/dir1/dir2/dir3/dir3.txt", "wb");
    const char *dir3txtstr = "dir3.txt\n";
    assert(fwrite(dir3txtstr, sizeof(*dir3txtstr), strlen(dir3txtstr), dir3txt) == strlen(dir3txtstr));
    fclose(dir3txt);

    FILE *dir2txt = fopen(WOWPKG_TEST_TMPDIR "test_osapi_tmp/dir1/dir2/dir2.txt", "wb");
    const char *dir2txtstr = "dir3.txt\n";
    assert(fwrite(dir2txtstr, sizeof(*dir2txtstr), strlen(dir2txtstr), dir2txt) == strlen(dir2txtstr));
    fclose(dir2txt);

    assert(os_remove_all(WOWPKG_TEST_TMPDIR "test_osapi_tmp") == 0);

    struct os_stat s;
    assert(os_stat(WOWPKG_TEST_TMPDIR "test_osapi_tmp", &s) != 0);
}

static void test_os_mkstemp(void)
{
    char template[] = WOWPKG_TEST_TMPDIR "test_os_mkstemp_XXXXXX";
    const char *prefix = WOWPKG_TEST_TMPDIR "test_os_mkstemp_";

    FILE *ftemp = os_mkstemp(template);
    assert(ftemp != NULL);
    assert(strncmp(template, prefix, strlen(prefix)) == 0);
    assert(strlen(template) == strlen(prefix) + 6); // 6 is number of 'X' in template.

    struct os_stat s;
    assert(os_stat(template, &s) == 0);

    assert(S_ISREG(s.st_mode));

    fclose(ftemp);
    remove(template);
}

static void test_os_mkstemps(void)
{
    char template[] = WOWPKG_TEST_TMPDIR "test_os_mkstemps_XXXXXXsuffix";
    const char *prefix = WOWPKG_TEST_TMPDIR "test_os_mkstemps_";
    const char *suffix = "suffix";

    FILE *ftemp = os_mkstemps(template, (int)strlen(suffix));
    assert(ftemp != NULL);
    assert(strncmp(template, prefix, strlen(prefix)) == 0);
    assert(strncmp(&template[strlen(prefix) + 6], suffix, strlen(suffix)) == 0);
    assert(strlen(template) == strlen(prefix) + 6 + strlen(suffix)); // 6 is number of 'X' in template.

    struct os_stat s;
    assert(os_stat(template, &s) == 0);

    assert(S_ISREG(s.st_mode));

    fclose(ftemp);
    remove(template);
}

static void test_os_mkstemps_suffixlen_too_large(void)
{
    char template[] = WOWPKG_TEST_TMPDIR "test_os_mkstemps_XXXXXXsuffix";
    const char *prefix = WOWPKG_TEST_TMPDIR "test_os_mkstemps_";
    const char *suffix = "suffix";

    FILE *ftemp = os_mkstemps(template, 4096);
    assert(ftemp == NULL);
    assert(errno == EINVAL);
    assert(strncmp(template, prefix, strlen(prefix)) == 0);
    assert(strncmp(&template[strlen(prefix)], "XXXXXX", 6) == 0); // 6 is number of 'X' in template.
    assert(strncmp(&template[strlen(prefix) + 6], suffix, strlen(suffix)) == 0);
    assert(strlen(template) == strlen(prefix) + 6 + strlen(suffix));

    ftemp = os_mkstemps(template, -1);
    assert(ftemp == NULL);
    assert(errno == EINVAL);

    struct os_stat s;
    assert(os_stat(template, &s) != 0);
}

static void test_os_mkdtemp(void)
{
    char template[] = WOWPKG_TEST_TMPDIR "test_os_mkdtemp_XXXXXX";

    assert(os_mkdtemp(template) != NULL);
    assert(strcmp(template, WOWPKG_TEST_TMPDIR "test_os_mkdtemp_XXXXXX") != 0);

    struct os_stat s;
    assert(os_stat(template, &s) == 0);

    assert(S_ISDIR(s.st_mode));

    os_rmdir(template);
}

static void test_os_rename_dir(void)
{
    char src[] = WOWPKG_TEST_TMPDIR "test_os_rename_XXXXXX";
    char dest[] = WOWPKG_TEST_TMPDIR "test_os_rename_XXXXXX";

    assert(os_mkdtemp(src) != NULL);
    assert(os_mkdtemp(dest) != NULL);

    assert(strcmp(src, dest) != 0);

    char actual[OS_MAX_PATH];
    int n = snprintf(actual, ARRLEN(actual), "%s%c%s", src, OS_SEPARATOR, &dest[strlen(WOWPKG_TEST_TMPDIR)]);
    assert(n > 0 && (size_t)n < ARRLEN(actual));

    assert(os_rename(dest, actual) == 0);

    struct os_stat s;
    assert(os_stat(actual, &s) == 0);
    assert(S_ISDIR(s.st_mode));

    os_remove_all(src);
}

static void test_os_rename_file(void)
{
    char src[] = WOWPKG_TEST_TMPDIR "test_os_rename_XXXXXX";
    char dest[] = WOWPKG_TEST_TMPDIR "test_os_rename_XXXXXX";

    FILE *srcf = os_mkstemp(src);

    assert(srcf != NULL);

    fclose(srcf);

    assert(strcmp(src, dest) != 0);

    assert(os_rename(src, dest) == 0);

    struct os_stat s;
    assert(os_stat(src, &s) != 0);

    assert(os_stat(dest, &s) == 0);
    assert(S_ISREG(s.st_mode));

    remove(dest);
}

static void test_os_rename_file_replace(void)
{
    char src[] = WOWPKG_TEST_TMPDIR "test_os_rename_XXXXXX";
    char dest[] = WOWPKG_TEST_TMPDIR "test_os_rename_XXXXXX";

    FILE *srcf = os_mkstemp(src);
    FILE *destf = os_mkstemp(dest);

    assert(srcf != NULL);
    assert(destf != NULL);

    fclose(srcf);
    fclose(destf);

    assert(strcmp(src, dest) != 0);

    assert(os_rename(src, dest) == 0);

    struct os_stat s;
    assert(os_stat(src, &s) != 0);

    assert(os_stat(dest, &s) == 0);
    assert(S_ISREG(s.st_mode));

    remove(dest);
}

int main(void)
{
    test_os_mkdir();
    test_os_mkdir_all();
    test_os_readdir();
    test_os_remove_all();
    test_os_mkstemp();
    test_os_mkstemps();
    test_os_mkstemps_suffixlen_too_large();
    test_os_mkdtemp();
    test_os_rename_dir();
    test_os_rename_file();
    test_os_rename_file_replace();

#ifdef _WIN32
    test_os_mkdir_all_win32();
#endif
}
