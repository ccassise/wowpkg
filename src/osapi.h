#pragma once

#include <stdio.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

#ifdef __APPLE__
#include <sys/syslimits.h>
#endif

#ifdef _WIN32

#define OS_SEPARATOR '\\'
#define OS_VALID_SEPARATORS "\\/"
#define OS_PATH_ENV_DELIMITER ";"
#define OS_MAX_PATH MAX_PATH
#define OS_MAX_FILENAME _MAX_FNAME

#define OS_IS_SEP(c) ((c) == OS_VALID_SEPARATORS[0] || (c) == OS_VALID_SEPARATORS[1])

#if !defined(S_ISDIR)
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif

#if !defined(S_ISREG)
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#endif

#define os_stat _stat
#define os_rmdir _rmdir
#define os_getcwd _getcwd
#define os_chdir _chdir

typedef unsigned short mode_t;

#else

#define OS_SEPARATOR '/'
#define OS_VALID_SEPARATORS "/"
#define OS_PATH_ENV_DELIMITER ":"
#define OS_MAX_PATH PATH_MAX
#define OS_MAX_FILENAME FILENAME_MAX

#define OS_IS_SEP(c) ((c) == OS_VALID_SEPARATORS[0])

#define os_stat stat
#define os_rmdir rmdir
#define os_getcwd getcwd
#define os_chdir chdir

#endif

typedef struct OsDirEnt {
    char *name;
} OsDirEnt;

#ifdef _WIN32

struct OsDir_Win32 {
    OsDirEnt entry;
    HANDLE dir;
    WIN32_FIND_DATA ffd;
    BOOL _is_first;
};

typedef struct OsDir_Win32 OsDir;

#else

struct OsDir {
    OsDirEnt entry;
    DIR *dir;
};

typedef struct OsDir OsDir;
#endif

/**
 * These functions behave like the POSIX functions with similar names. See
 * opendir(3), readdir(3), and closedir(3).
 */
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
int os_mkdir(const char *path, mode_t perms);
int os_mkdir_all(char *path, mode_t perms);

/**
 * Generates a unique temporary filename from template. Creates and opens the
 * file, and returns the FILE stream.
 *
 * Modifies the template string. The last six characters of template shall be
 * "XXXXXX". Since it will be modified template shall not be a string constant.
 *
 * os_mkstemps includes a suffix after the six characters of template. So,
 * template would be in the form of prefixXXXXXXsuffix.
 *
 * On success returns an open FILE with "w+b" mode set. On error, NULL is returned
 * and errno is set.
 */
FILE *os_mkstemp(char *template);
FILE *os_mkstemps(char *template, int suffixlen);

/**
 * Generates a unique temporary filename from template. Creates a directory with
 * permissions 0700 (on non-Windows).
 *
 * Modifies the template string. The last six characters of template shall be
 * "XXXXXX". Since it will be modified template shall not be a string constant.
 *
 * On success returns the modified template string. Otherwise returns NULL and
 * sets errno on errors.
 */
char *os_mkdtemp(char *template);

/**
 * Attempts to get the temp directory for the current OS. If no value was found
 * then it returns the current working directory ".".
 *
 * WARNING: The returned string may be statically allocated and as such shall
 * not be modified. Future calls of this function may change the contents of
 * previously returned strings.
 *
 * On Windows, it returns the first non-empty value from %TMP%, %TEMP%, or
 * %USERPROFILE%.
 *
 * On Unix systems, it returns the first non-empty value from $TMPDIR or /tmp.
 */
const char *os_tempdir(void);

/**
 * Removes the file at path. If path is a directory then it recursively removes
 * all files and subdirectories.
 *
 * On success returns 0, otherwise returns -1 and sets errno on errors.
 */
int os_remove_all(const char *path);

/**
 * See rename(2) for *nix and MoveFileEx with MOVEFILE_REPLACE_EXISTING and
 * MOVEFILE_COPY_ALLOWED for Windows.
 *
 * If new path is on a different file system and rename fails, then a copy and
 * delete will instead be performed.
 *
 * On success returns 0, otherwise returns -1 and sets errno on errors.
 */
int os_rename(const char *oldpath, const char *newpath);
