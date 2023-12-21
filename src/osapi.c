#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#ifdef __linux__
#include <sys/sendfile.h>
#endif

#include "osapi.h"
#include "wowpkg.h"

static int os_copyfile(const char *oldpath, const char *newpath)
{
#if defined(_WIN32)
#elif defined(__APPLE__)
#else /* Linux */
    int err = 0;

    FILE *fold = fopen(oldpath, "rb");
    FILE *fnew = fopen(newpath, "wb");
    if (fold == NULL || fnew == NULL) {
        err = -1;
        goto cleanup;
    }

    int fdold = fileno(fold);
    int fdnew = fileno(fnew);
    if (fdold == -1 || fdnew == -1) {
        err = -1;
        goto cleanup;
    }

    struct stat sold;
    if (fstat(fdold, &sold) != 0 || sold.st_size < 0) {
        err = -1;
        goto cleanup;
    }

    ssize_t wrote = 0;
    while ((wrote += sendfile(fdnew, fdold, NULL, (size_t)sold.st_size)) < (ssize_t)sold.st_size) {
        if (wrote < 0) {
            err = -1;
            goto cleanup;
        }
    }

cleanup:
    if (fold != NULL) {
        fclose(fold);
    }
    if (fnew != NULL) {
        fclose(fnew);
    }
    return err;
#endif
}

/**
 * Copies all contents of directory at old path to new path.
 *
 * Old and new path shall be paths to directories that already exist. If new
 * path contains a file with the same path from old path, then the file will be
 * overwritten.
 *
 * Returns 0, -1 and sets errno on errors.
 */
static int os_copydir(const char *oldpath, const char *newpath)
{
    int err = 0;

    struct os_stat s_old;
    struct os_stat s_new;

    err = os_stat(oldpath, &s_old);
    if (err != 0 || !S_ISDIR(s_old.st_mode)) {
        return -1;
    }

    err = os_stat(newpath, &s_new);
    if (err != 0 || !S_ISDIR(s_new.st_mode)) {
        return -1;
    }

    OsDir *dir = os_opendir(oldpath);
    if (dir == NULL) {
        return -1;
    }

    OsDirEnt *entry = NULL;
    while ((entry = os_readdir(dir)) != NULL) {
        if (strcmp(entry->name, ".") == 0 || strcmp(entry->name, "..") == 0) {
            continue;
        }

        char oldname[OS_MAX_PATH];
        char newname[OS_MAX_PATH];

        int n = 0;

        n = snprintf(oldname, ARRAY_SIZE(oldname), "%s%c%s", oldpath, OS_SEPARATOR, entry->name);
        if (n < 0 || (size_t)n > ARRAY_SIZE(oldname)) {
            errno = ENAMETOOLONG;
            err = -1;
            break;
        }

        n = snprintf(newname, ARRAY_SIZE(newname), "%s%c%s", newpath, OS_SEPARATOR, entry->name);
        if (n < 0 || (size_t)n > ARRAY_SIZE(newname)) {
            errno = ENAMETOOLONG;
            err = -1;
            break;
        }

        struct os_stat s_oldname;
        if (os_stat(oldname, &s_oldname) != 0) {
            err = -1;
            break;
        }

        if (S_ISDIR(s_oldname.st_mode)) {
#ifdef _WIN32
            mode_t permissions = 0755;
#else
            mode_t permissions = s_oldname.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
#endif
            err = os_mkdir(newname, permissions);
            if (err != 0 && errno != EEXIST) {
                return -1;
            }

            err = os_copydir(oldname, newname);
        } else {
            err = os_copyfile(oldname, newname);
        }

        if (err != 0) {
            break;
        }
    }

    os_closedir(dir);

    return err;
}

OsDir *os_opendir(const char *path)
{
    OsDir *result = malloc(sizeof(*result));
    if (result == NULL) {
        errno = ENOMEM;
        return NULL;
    }

#ifdef _WIN32
    /* Windows requires a '*' at the end of a path in order to grab all files in
     * a directory, and this function should never be called like that -- so add
     * it now. */
    char path_win[OS_MAX_PATH];
    int n = snprintf(path_win, ARRAY_SIZE(path_win), "%s%c*", path, OS_SEPARATOR);
    if (n < 0 || (size_t)n >= ARRAY_SIZE(path_win)) {
        errno = ENAMETOOLONG;
        goto error;
    }

    result->dir = FindFirstFile(path_win, &result->ffd);
    if (result->dir == INVALID_HANDLE_VALUE) {
        errno = ENOENT;
        goto error;
    }

    result->entry.name = result->ffd.cFileName;
    result->_is_first = TRUE;

#else
    result->dir = opendir(path);
    if (result->dir == NULL) {
        goto error;
    }
#endif

    return result;

error:
    free(result);
    return NULL;
}

OsDirEnt *os_readdir(OsDir *dir)
{
#ifdef _WIN32
    if (dir->_is_first == TRUE) {
        dir->_is_first = FALSE;
        return &dir->entry;
    }

    if (!FindNextFile(dir->dir, &dir->ffd)) {
        return NULL;
    }

    dir->entry.name = dir->ffd.cFileName;
    return &dir->entry;
#else
    struct dirent *entry = NULL;

    entry = readdir(dir->dir);
    if (entry == NULL) {
        return NULL;
    }

    dir->entry.name = entry->d_name;
    return &dir->entry;
#endif
}

void os_closedir(OsDir *dir)
{
    if (dir == NULL) {
        return;
    }

#ifdef _WIN32
    FindClose(dir->dir);
    free(dir);
#else
    closedir(dir->dir);
    free(dir);
#endif
}

int os_mkdir(const char *path, mode_t perms)
{
    if (path[0] == '\0') {
        return 0;
    }

#ifdef _WIN32
    UNUSED(perms);
    return _mkdir(path);
#else
    return mkdir(path, perms);
#endif
}

int os_mkdir_all(char *path, mode_t perms)
{
#ifdef _WIN32
    UNUSED(perms);
#endif

    char *sep = path;
    while (*(sep += strcspn(sep, OS_VALID_SEPARATORS)) != '\0') {
        char sep_ch = *sep;
        *sep = '\0';

        int err = os_mkdir(path, perms);
        *sep = sep_ch; /* Restore separator. */

        if (err != 0 && errno != EEXIST) {
            return -1;
        }

        sep++;
    }

    return 0;
}

FILE *os_mkstemp(char *template)
{
    return os_mkstemps(template, 0);
}

FILE *os_mkstemps(char *template, int suffixlen)
{
#ifdef _WIN32
    int len = (int)strlen(template);

    if (suffixlen < 0 || suffixlen > len) {
        errno = EINVAL;
        return NULL;
    }

    int suffix_start = len - suffixlen;

    char ch = template[suffix_start];
    template[suffix_start] = '\0';

    char *tmp = _mktemp(template);
    template[suffix_start] = ch;

    if (tmp == NULL) {
        return NULL;
    }

    return fopen(tmp, "w+b");
#else
    int fd = mkstemps(template, suffixlen);
    if (fd == -1) {
        return NULL;
    }

    return fdopen(fd, "w+b");
#endif
}

char *os_mkdtemp(char *template)
{
#ifdef _WIN32
    char *result = _mktemp(template);
    if (result == NULL) {
        return NULL;
    }

    if (os_mkdir(result, 0700) != 0) {
        return NULL;
    }

    return result;
#else
    return mkdtemp(template);
#endif
}

const char *os_tempdir(void)
{
#ifdef _WIN32
    const char *tmp_paths[] = {
        getenv("TMP"),
        getenv("TEMP"),
        getenv("USERPROFILE"),
    };
#else
    const char *tmp_paths[] = {
        getenv("TMPDIR"),
        "/tmp",
    };
#endif

    for (size_t i = 0; i < ARRAY_SIZE(tmp_paths); i++) {
        if (tmp_paths[i] != NULL) {
            struct os_stat s;
            if (os_stat(tmp_paths[i], &s) == 0 && S_ISDIR(s.st_mode)) {
                return tmp_paths[i];
            }
        }
    }

    return ".";
}

int os_remove_all(const char *path)
{
    int err = 0;
    OsDir *dir = os_opendir(path);
    if (dir == NULL) {
        return -1;
    }

    OsDirEnt *entry = NULL;
    while ((entry = os_readdir(dir)) != NULL && err == 0) {
        if (strcmp(".", entry->name) == 0 || strcmp("..", entry->name) == 0) {
            continue;
        }

        char p[OS_MAX_PATH];
        int n = snprintf(p, ARRAY_SIZE(p), "%s%c%s", path, OS_SEPARATOR, entry->name);
        if (n < 0 || (size_t)n >= ARRAY_SIZE(p)) {
            errno = ENAMETOOLONG;
            err = -1;
            break;
        }

        struct os_stat s;
        os_stat(p, &s);

        if (S_ISDIR(s.st_mode)) {
            err = os_remove_all(p);
        } else {
            err = remove(p);
        }
    }

    os_closedir(dir);

    if (err == 0) {
        err = os_rmdir(path);
    }

    return err;
}

int os_rename(const char *oldpath, const char *newpath)
{
#ifdef _WIN32
    if (!MoveFileExA(oldpath, newpath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
        return -1;
    }
    return 0;
#else
    /* Try using rename(2) but if errno is EXDEV then we will fall back to copy
     * and delete. */
    int err = rename(oldpath, newpath);
    if (err == 0) {
        return 0;
    } else if (err != 0 && errno != EXDEV) {
        return -1;
    }

    struct os_stat s_old;
    if (os_stat(oldpath, &s_old) != 0) {
        return -1;
    }

    if (S_ISREG(s_old.st_mode)) {
        if (os_copyfile(oldpath, newpath) != 0) {
            return -1;
        }
        remove(oldpath);
    } else if (S_ISDIR(s_old.st_mode)) {
        struct os_stat s_new;

        err = os_stat(newpath, &s_new);
        if (err != 0) {
            /* Old path is a directory and new path does not exist -- create a
             * directory at the new path. */
            err = os_mkdir(newpath, 0755);
            if (err != 0) {
                return -1;
            }
        } else {
            /* Old path is a directory and new path exists -- ensure that new
             * path is an empty directory. */
            if (!S_ISDIR(s_new.st_mode)) {
                return -1;
            }

            OsDir *dir = os_opendir(newpath);
            if (dir == NULL) {
                return -1;
            }

            OsDirEnt *entry = NULL;
            while ((entry = os_readdir(dir)) != NULL) {
                /* These should be the only entries in an empty directory. */
                if (strcmp(entry->name, ".") != 0 && strcmp(entry->name, "..") != 0) {
                    err = -1;
                    break;
                }
            }

            os_closedir(dir);

            if (err != 0) {
                return -1;
            }
        }

        if (os_copydir(oldpath, newpath) != 0) {
            return -1;
        }

        os_remove_all(oldpath);
    }

    return 0;
#endif
}
