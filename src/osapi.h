#pragma once

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

#ifdef _WIN32
#define OS_SEPARATOR '\\'
#else
#define OS_SEPARATOR '/'
#endif

typedef struct OsDirEnt {
    char *name;
} OsDirEnt;

typedef struct OsDir {
    OsDirEnt entry;
#ifdef _WIN32
    HANDLE dir;
    WIN32_FIND_DATA ffd;
    BOOL _is_first;
#else
    DIR dir;
#endif
} OsDir;

OsDir *os_opendir(const char *path);
OsDirEnt *os_readdir(OsDir *dir);
void os_closedir(OsDir *dir);

/**
 * Creates a directory at the given path with the given permissions. On Windows,
 * permissions is ignored.
 *
 * mkdir_all will create all directories in path that do not exist. It may
 * modify the path string.
 *
 * On success returns 0, otherwise returns -1 and sets errno on errors.
 */
int os_mkdir(const char *path, int perms);
int os_mkdir_all(char *path, int perms);

/**
 * Removes the file at path. If path is a directory then it recursively removes
 * all files and subdirectories.
 *
 * On success returns 0, otherwise returns -1 and sets errno on errors.
 */
int os_remove_all(const char *path);
