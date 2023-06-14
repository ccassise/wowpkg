#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>

#include "osapi.h"
#include "zipper.h"

// #ifdef _WIN32
// static const char *PATH_SEPS = "/\\";
// #else
// static const char *PATH_SEPS = "/";
// #endif

// static bool is_sep(char sep)
// {
//     for (const char *s = PATH_SEPS; *s; s++) {
//         if (*s == sep) {
//             return true;
//         }
//     }

//     return false;
// }

// // https://github.com/zlib-ng/minizip-ng/commit/dd808239ddd952079d6061dfd3132933a856a980
// static int snclean(char *s, size_t n, const char *path)
// {
//     const char *p = path;
//     char *wp = s;
//     int wrote = 0;
//     while (1) {
//         size_t path_end = strcspn(p, PATH_SEPS);
//         // printf("%lld\n", path_end);
//         if (path_end == 0 || strncmp(p, "..", path_end) != 0) {
//             for (size_t i = 0; i < path_end; i++) {
//                 *wp++ = p[i];
//                 *wp = '\0';
//                 wrote++;
//                 // putchar(p[i]);
//             }
//             // putchar('\n');
//             // printf("here?\n");
//             *wp++ = OS_SEPARATOR;
//             *wp = '\0';
//             wrote++;
//         }

//         if (p[path_end] == '\0') {
//             break;
//         }

//         p += path_end + 1;
//     }

//     return wrote;
// }

static int zipper_unzip_file(unzFile uf, const char *dest)
{
    int err = ZIPPER_OK;
    unz_file_info64 finfo;
    char filename[OS_MAX_FILENAME];
    FILE *out_file = NULL;

    err = unzGetCurrentFileInfo64(uf, &finfo, filename, sizeof(filename), NULL, 0, NULL, 0);
    if (err != UNZ_OK) {
        return ZIPPER_ENOENT;
    }

    err = unzOpenCurrentFile(uf);
    if (err != UNZ_OK) {
        return ZIPPER_ENOENT;
    }

    char new_path[OS_MAX_PATH];
    int nwrote = snprintf(new_path, sizeof(new_path), "%s%c%s", dest, OS_SEPARATOR, filename);
    if (nwrote >= (int)sizeof(new_path) || nwrote < 0) {
        err = ZIPPER_ENAMETOOLONG;
        goto end;
    }

    err = os_mkdir_all(new_path, 0755);
    if (err != 0) {
        err = ZIPPER_ENOENT;
        goto end;
    }

    if (filename[finfo.size_filename - 1] != '/') {
        out_file = fopen(new_path, "wb");
        if (out_file == NULL) {
            err = ZIPPER_ENOENT;
            goto end;
        }

        unsigned char buf[BUFSIZ];

        int nread = 0;
        while ((nread = unzReadCurrentFile(uf, buf, sizeof(buf) / sizeof(buf[0]))) > 0) {
            if (fwrite(buf, sizeof(*buf), (size_t)nread, out_file) != (size_t)nread) {
                err = ZIPPER_EWRITE;
                goto end;
            }
        }

        if (nread < 0) {
            err = ZIPPER_EREAD;
            goto end;
        }
    }

end:
    unzCloseCurrentFile(uf);
    if (out_file != NULL) {
        fclose(out_file);
    }

    if (err == ZIPPER_OK) {
        err = unzGoToNextFile(uf);
        if (err == UNZ_END_OF_LIST_OF_FILE) {
            err = ZIPPER_EEND_OF_LIST;
        } else if (err != UNZ_OK) {
            err = ZIPPER_ENOENT;
        }
    }

    return err;
}

int zipper_unzip(const char *src, const char *dest)
{
    // char tmp[OS_MAX_PATH];

    // int nww = snclean(tmp, OS_MAX_PATH, "/../hello/../world../..hmm../../");
    // printf("%d: %s\n", nww, tmp);

    // nww = snclean(tmp, OS_MAX_PATH, "/");
    // printf("%d: %s\n", nww, tmp);

    // return nww;
    int err = ZIPPER_OK;

    struct os_stat s;
    if (os_stat(dest, &s) != 0 || !(s.st_mode & S_IFDIR)) {
        return ZIPPER_ENOENT;
    }

    unzFile uf = unzOpen64(src);
    if (uf == NULL) {
        return ZIPPER_ENOENT;
    }

    unz_global_info64 ufinfo;
    err = unzGetGlobalInfo64(uf, &ufinfo);
    if (err != UNZ_OK) {
        err = ZIPPER_ENOENT;
        goto end;
    }

    for (size_t i = 0; i < ufinfo.number_entry; i++) {
        err = zipper_unzip_file(uf, dest);
        if (err == ZIPPER_EEND_OF_LIST) {
            err = ZIPPER_OK;
            break;
        } else if (err != ZIPPER_OK) {
            break;
        }
    }

end:
    unzClose(uf);
    return err;
}
