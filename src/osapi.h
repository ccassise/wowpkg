#pragma once

#include <stdio.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

#ifdef _WIN32

#define OS_SEPARATOR '\\'
#define OS_MAX_PATH MAX_PATH
#define OS_MAX_FILENAME _MAX_FNAME

#define os_stat _stat
#define os_rmdir _rmdir

typedef unsigned int OsMode;

#else

#define OS_SEPARATOR '/'
#define OS_MAX_PATH PATH_MAX
#define OS_MAX_FILENAME FILENAME_MAX

#define os_stat stat
#define os_rmdir rmdir

typedef mode_t OsMode;

#endif

typedef struct OsDirEnt {
    char *name;
} OsDirEnt;

#ifdef _WIN32

struct OsDirWin32 {
    OsDirEnt entry;
    HANDLE dir;
    WIN32_FIND_DATA ffd;
    BOOL _is_first;
};

typedef struct OsDirWin32 OsDir;

#else

struct OsDir {
    OsDirEnt entry;
    DIR *dir;
};

typedef struct OsDir OsDir;
#endif

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
int os_mkdir(const char *path, OsMode perms);
int os_mkdir_all(char *path, OsMode perms);

/**
 * Generates a unique temporary filename from template. Creates and opens the file, and returns the FILE stream.
 *
 * Modifies the template string. The last six characters of template shalle be
 * "XXXXXX". Since it will be modified template shall not be a string constant.
 *
 * On success returns an open FILE with "w+b" mode. On error, NULL is returned
 * and errno is set.
 */
FILE *os_mkstemp(char *template);

/**
 * Removes the file at path. If path is a directory then it recursively removes
 * all files and subdirectories.
 *
 * On success returns 0, otherwise returns -1 and sets errno on errors.
 */
int os_remove_all(const char *path);
