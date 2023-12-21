#undef NDEBUG

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

    bool expect_was_found[ARRAY_SIZE(expect)] = { false };

    assert(dir != NULL);

    /* Since order is not gauranteed from os_readdir we just look that every
     * entry in expect appeared exactly once. */
    while ((entry = os_readdir(dir)) != NULL) {
        char *actual = entry->name;
        bool found = false;
        for (size_t i = 0; i < ARRAY_SIZE(expect); i++) {
            if (strcmp(actual, expect[i]) == 0) {
                assert(!expect_was_found[i]);
                expect_was_found[i] = true;
                found = true;
                break;
            }
        }
        assert(found);
    }

    for (size_t i = 0; i < ARRAY_SIZE(expect); i++) {
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
    assert(strlen(template) == strlen(prefix) + 6); /* 6 is number of 'X' in template. */

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
    assert(strlen(template) == strlen(prefix) + 6 + strlen(suffix)); /* 6 is number of 'X' in template. */

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
    assert(strncmp(&template[strlen(prefix)], "XXXXXX", 6) == 0); /* 6 is number of 'X' in template. */
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
    char oldpath[] = WOWPKG_TEST_TMPDIR "test_os_rename_dir_XXXXXX";
    char newpath[] = WOWPKG_TEST_TMPDIR "test_os_rename_dir_XXXXXX";
    char txt_data[] = "test text data";

    assert(os_mkdtemp(oldpath) != NULL);

    assert(strcmp(oldpath, newpath) != 0);

    /* Create a path to a text file in the old directory. */
    char old_txt_path[OS_MAX_PATH];
    int n = snprintf(old_txt_path, ARRAY_SIZE(old_txt_path), "%s%c%s", oldpath, OS_SEPARATOR, "test.txt");
    assert(n > 0 && (size_t)n < ARRAY_SIZE(old_txt_path));

    /* Where we expect the text file to end up. */
    char new_txt_path[OS_MAX_PATH];
    n = snprintf(new_txt_path, ARRAY_SIZE(new_txt_path), "%s%c%s", newpath, OS_SEPARATOR, "test.txt");
    assert(n > 0 && (size_t)n < ARRAY_SIZE(new_txt_path));

    /* Create a text file in the old directory with some data. */
    FILE *ftxt = fopen(old_txt_path, "wb");
    assert(ftxt != NULL);
    assert(fwrite(txt_data, sizeof(*txt_data), ARRAY_SIZE(txt_data), ftxt) == ARRAY_SIZE(txt_data));
    fclose(ftxt);

    assert(os_rename(oldpath, newpath) == 0);

    /* Check that everything from the old path got moved to the new path. */
    struct os_stat s;
    assert(os_stat(newpath, &s) == 0);
    assert(S_ISDIR(s.st_mode));
    assert(os_stat(new_txt_path, &s) == 0);
    assert(S_ISREG(s.st_mode));

    ftxt = fopen(new_txt_path, "rb");
    assert(ftxt != NULL);

    char buf[BUFSIZ];
    assert(fread(buf, sizeof(*buf), ARRAY_SIZE(buf), ftxt) == ARRAY_SIZE(txt_data));
    assert(strcmp(buf, txt_data) == 0); /* Terminating NULL is included in ARRAY_SIZE(txt_data). */

    /* Make sure old path no longer exists. */
    assert(os_stat(oldpath, &s) != 0);

    fclose(ftxt);
    os_remove_all(newpath);
}

static void test_os_rename_file(void)
{
    char oldpath[] = WOWPKG_TEST_TMPDIR "test_os_rename_file_XXXXXX";
    char newpath[] = WOWPKG_TEST_TMPDIR "test_os_rename_file_XXXXXX";

    FILE *fold = os_mkstemp(oldpath);

    assert(fold != NULL);

    fclose(fold);

    assert(strcmp(oldpath, newpath) != 0);

    assert(os_rename(oldpath, newpath) == 0);

    struct os_stat s;
    assert(os_stat(oldpath, &s) != 0);

    assert(os_stat(newpath, &s) == 0);
    assert(S_ISREG(s.st_mode));

    remove(newpath);
}

static void test_os_rename_file_replace(void)
{
    char oldpath[] = WOWPKG_TEST_TMPDIR "test_os_rename_file_replace_XXXXXX";
    char newpath[] = WOWPKG_TEST_TMPDIR "test_os_rename_file_replace_XXXXXX";
    char test_data[] = "some test data\n";

    FILE *fold = os_mkstemp(oldpath);
    FILE *fnew = os_mkstemp(newpath);

    assert(fold != NULL);
    assert(fnew != NULL);

    assert(fwrite(test_data, sizeof(*test_data), ARRAY_SIZE(test_data), fold) == ARRAY_SIZE(test_data));

    fclose(fold);
    fclose(fnew);

    assert(strcmp(oldpath, newpath) != 0);

    assert(os_rename(oldpath, newpath) == 0);

    struct os_stat s;
    assert(os_stat(oldpath, &s) != 0);

    assert(os_stat(newpath, &s) == 0);
    assert(S_ISREG(s.st_mode));

    char buf[BUFSIZ];
    fnew = fopen(newpath, "rb");
    assert(fread(buf, sizeof(*buf), ARRAY_SIZE(buf), fnew) == ARRAY_SIZE(test_data));
    assert(strcmp(buf, test_data) == 0); /* Terminating NULL is included in ARRAY_SIZE(test_data). */

    fclose(fnew);
    remove(newpath);
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
