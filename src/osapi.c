#include <direct.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "osapi.h"

#ifdef _WIN32
static const char *OS_PATH_SEPS = "/\\";
#else
static const char *OS_PATH_SEPS = "/";
#endif

OsDir *os_opendir(const char *path)
{
    OsDir *result = malloc(sizeof(*result));
    if (result == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    char path_win[MAX_PATH];
    int n = snprintf(path_win, MAX_PATH, "%s%c*", path, OS_SEPARATOR);
    if (n >= MAX_PATH) {
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

    return result;

error:
    free(result);
    return NULL;
}

OsDirEnt *os_readdir(OsDir *dir)
{
    if (dir->_is_first == TRUE) {
        dir->_is_first = FALSE;
        return &dir->entry;
    }

    if (!FindNextFile(dir->dir, &dir->ffd)) {
        return NULL;
    }

    dir->entry.name = dir->ffd.cFileName;
    return &dir->entry;
}

void os_closedir(OsDir *dir)
{
    FindClose(dir->dir);
    free(dir);
}

int os_mkdir(const char *path, int perms)
{
#ifdef _WIN32
    (void)perms;
    return _mkdir(path);
#endif
}

int os_mkdir_all(char *path, int perms)
{
#ifdef _WIN32
    (void)perms;
#endif

    char *sep = path;
    while (*(sep += strcspn(sep, OS_PATH_SEPS)) != '\0') {
        char sep_ch = *sep;
        *sep = '\0';

        int err = os_mkdir(path, 0755);
        *sep = sep_ch; // Restore separator.

        if (err != 0 && errno != EEXIST) {
            return -1;
        }

        sep++;
    }

    return 0;
}

int os_remove_all(const char *path)
{
    int result = 0;
    OsDir *dir = os_opendir(path);
    if (dir == NULL) {
        return -1;
    }

    OsDirEnt *entry = NULL;
    while ((entry = os_readdir(dir)) != NULL && result == 0) {
        if (strcmp(".", entry->name) == 0 || strcmp("..", entry->name) == 0) {
            continue;
        }

        char p[MAX_PATH];
        int n = snprintf(p, MAX_PATH, "%s%c%s", path, OS_SEPARATOR, entry->name);
        if (n >= MAX_PATH) {
            errno = ENAMETOOLONG;
            result = -1;
            break;
        }

        struct _stat s;
        _stat(p, &s);

        if (s.st_mode & _S_IFDIR) {
            result = os_remove_all(p);
        } else {
            result = remove(p);
        }
    }

    os_closedir(dir);

    if (result == 0) {
        result = _rmdir(path);
    }

    return result;
}
