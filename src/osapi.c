#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "osapi.h"
#include "wowpkg.h"

OsDir *os_opendir(const char *restrict path)
{
    OsDir *result = malloc(sizeof(*result));
    if (result == NULL) {
        errno = ENOMEM;
        return NULL;
    }

#ifdef _WIN32
    char path_win[OS_MAX_PATH];
    int n = snprintf(path_win, ARRLEN(path_win), "%s%c*", path, OS_SEPARATOR);
    if (n < 0 || (size_t)n >= ARRLEN(path_win)) {
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

OsDirEnt *os_readdir(OsDir *restrict dir)
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

void os_closedir(OsDir *restrict dir)
{
#ifdef _WIN32
    FindClose(dir->dir);
    free(dir);
#else
    closedir(dir->dir);
    free(dir);
#endif
}

int os_mkdir(const char *restrict path, OsMode perms)
{
    if (*path == '\0') {
        return 0;
    }

#ifdef _WIN32
    UNUSED(perms);
    return _mkdir(path);
#else
    return mkdir(path, perms);
#endif
}

int os_mkdir_all(char *restrict path, OsMode perms)
{
#ifdef _WIN32
    UNUSED(perms);
#endif

    char *sep = path;
    while (*(sep += strcspn(sep, OS_VALID_SEPARATORS)) != '\0') {
        char sep_ch = *sep;
        *sep = '\0';

        int err = os_mkdir(path, perms);
        *sep = sep_ch; // Restore separator.

        if (err != 0 && errno != EEXIST) {
            return -1;
        }

        sep++;
    }

    return 0;
}

FILE *os_mkstemp(char *restrict template)
{
#ifdef _WIN32
    char *tmp = _mktemp(template);
    if (tmp == NULL) {
        return NULL;
    }

    return fopen(tmp, "w+b");
#else
    int fd = mkstemp(template);
    if (fd == -1) {
        return NULL;
    }

    return fdopen(fd, "w+b");
#endif
}

char *os_mkdtemp(char *restrict template)
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

int os_remove_all(const char *restrict path)
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
        int n = snprintf(p, ARRLEN(p), "%s%c%s", path, OS_SEPARATOR, entry->name);
        if (n < 0 || (size_t)n >= ARRLEN(p)) {
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

int os_rename(const char *restrict src, const char *restrict dest)
{
#ifdef _WIN32
    if (!MoveFileEx(src, dest, MOVEFILE_REPLACE_EXISTING)) {
        return -1;
    }
    return 0;
#else
    return rename(src, dest);
#endif
}
