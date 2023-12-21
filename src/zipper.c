#include <stdio.h>

#include <archive.h>
#include <archive_entry.h>

#include "osapi.h"
#include "wowpkg.h"
#include "zipper.h"

// /**
//  * Copies path to the given buffer removing '.', '..', and multiple sequential
//  * separators. Also, converts separators to the OS native separator.
//  *
//  * This function has similar semantics as snprintf(3). Writes no more than n - 1
//  * characters to buffer and buffer will always be null terminated as long as n > 0.
//  *
//  * Returns the number of characters that were written or would have been written
//  * had buffer contained enough space.
//  */
// static int snclean_path(char *restrict buf, size_t n, const char *restrict path)
// {
//     if (n > 0) {
//         buf[0] = '\0';
//     }

//     if (path[0] == '\0') {
//         return 0;
//     }

//     size_t result = 0;
//     char *writer = buf;
//     const char *filename = path;
//     while (1) {
//         size_t filename_len = strcspn(filename, OS_VALID_SEPARATORS);
//         if (filename == path && filename_len == 0) {
//             /* Handle root. */
//             if (result < n - 1) {
//                 writer[result] = OS_SEPARATOR;
//                 writer[result + 1] = '\0';
//             }
//             result++;
//         } else if (filename_len > 0) {
//             if (strncmp(filename, ".", filename_len - 1) != 0 || strncmp(filename, "..", filename_len - 1) != 0) {
//                 for (size_t i = 0; i < filename_len; i++) {
//                     if (n > 0 && result < n - 1) {
//                         writer[result] = filename[i];
//                         writer[result + 1] = '\0';
//                     }

//                     result++;
//                 }

//                 if (filename[filename_len] != '\0') {
//                     if (n > 0 && result < n - 1) {
//                         writer[result] = OS_SEPARATOR;
//                         writer[result + 1] = '\0';
//                     }

//                     result++;
//                 }
//             }
//         }

//         if (filename[filename_len] == '\0') {
//             break;
//         }

//         filename = &filename[filename_len + 1];
//     }

//     return (int)result;
// }

// static int zipper_unzip_file(unzFile uf, const char *dest)
// {
//     int err = ZIPPER_OK;
//     unz_file_info64 finfo;
//     FILE *out_file = NULL;
//     char raw_filename[OS_MAX_FILENAME];
//     char filename[OS_MAX_FILENAME];

//     err = unzGetCurrentFileInfo64(uf, &finfo, raw_filename, ARRAY_SIZE(raw_filename), NULL, 0, NULL, 0);
//     if (err != UNZ_OK) {
//         return ZIPPER_ENOENT;
//     }

//     int filename_len = snclean_path(filename, ARRAY_SIZE(filename), raw_filename);
//     if (filename_len >= (int)ARRAY_SIZE(filename)) {
//         err = ZIPPER_ENAMETOOLONG;
//         goto cleanup;
//     }

//     err = unzOpenCurrentFile(uf);
//     if (err != UNZ_OK) {
//         return ZIPPER_ENOENT;
//     }

//     char new_path[OS_MAX_PATH];
//     int n = snprintf(new_path, sizeof(new_path), "%s%c%s", dest, OS_SEPARATOR, filename);
//     if (n < 0 || (size_t)n >= sizeof(new_path)) {
//         err = ZIPPER_ENAMETOOLONG;
//         goto cleanup;
//     }

//     err = os_mkdir_all(new_path, 0755);
//     if (err != 0) {
//         err = ZIPPER_ENOENT;
//         goto cleanup;
//     }

//     if (finfo.compressed_size != 0) {
//         /* File is not a directory. */
//         out_file = fopen(new_path, "wb");
//         if (out_file == NULL) {
//             err = ZIPPER_ENOENT;
//             goto cleanup;
//         }

//         unsigned char buf[BUFSIZ];

//         int nread = 0;
//         while ((nread = unzReadCurrentFile(uf, buf, ARRAY_SIZE(buf))) > 0) {
//             if (fwrite(buf, sizeof(*buf), (size_t)nread, out_file) != (size_t)nread) {
//                 err = ZIPPER_EWRITE;
//                 goto cleanup;
//             }
//         }

//         if (nread < 0) {
//             err = ZIPPER_EREAD;
//             goto cleanup;
//         }
//     }

// cleanup:
//     unzCloseCurrentFile(uf);
//     if (out_file != NULL) {
//         fclose(out_file);
//     }

//     if (err == ZIPPER_OK) {
//         err = unzGoToNextFile(uf);
//         if (err == UNZ_END_OF_LIST_OF_FILE) {
//             err = ZIPPER_EEND_OF_LIST;
//         } else if (err != UNZ_OK) {
//             err = ZIPPER_ENOENT;
//         }
//     }

//     return err;
// }

// int zipper_unzip(const char *src, const char *dest)
// {
//     int err = ZIPPER_OK;

//     struct os_stat s;
//     if (os_stat(dest, &s) != 0 || !S_ISDIR(s.st_mode)) {
//         return ZIPPER_ENOENT;
//     }

//     unzFile uf = unzOpen64(src);
//     if (uf == NULL) {
//         return ZIPPER_ENOENT;
//     }

//     unz_global_info64 ufinfo;
//     err = unzGetGlobalInfo64(uf, &ufinfo);
//     if (err != UNZ_OK) {
//         err = ZIPPER_ENOENT;
//         goto cleanup;
//     }

//     for (size_t i = 0; i < ufinfo.number_entry; i++) {
//         err = zipper_unzip_file(uf, dest);
//         if (err == ZIPPER_EEND_OF_LIST) {
//             err = ZIPPER_OK;
//             break;
//         } else if (err != ZIPPER_OK) {
//             break;
//         }
//     }

// cleanup:
//     unzClose(uf);
//     return err;
// }

static int copy_data(struct archive *reader, struct archive *writer)
{
    int64_t err = ARCHIVE_OK;
    const void *buf;
    size_t size;
    la_int64_t offset;

    while (1) {
        err = archive_read_data_block(reader, &buf, &size, &offset);
        if (err == ARCHIVE_EOF) {
            return ZIPPER_OK;
        } else if (err != ARCHIVE_OK) {
            return ZIPPER_EREAD;
            ;
        }

        err = archive_write_data_block(writer, buf, size, offset);
        if (err != ARCHIVE_OK) {
            return ZIPPER_EWRITE;
        }
    }

    return err;
}

int zipper_unzip(const char *src, const char *dest)
{
    if (src == NULL || dest == NULL) {
        return ZIPPER_ENOENT;
    }

    struct os_stat s;
    if (os_stat(dest, &s) != 0) {
        return ZIPPER_ENOENT;
    }

    struct archive *a;
    struct archive *ext;
    struct archive_entry *entry;

    int err = ZIPPER_OK;
    int flags = 0;
    flags |= ARCHIVE_EXTRACT_ACL;
    flags |= ARCHIVE_EXTRACT_FFLAGS;
    flags |= ARCHIVE_EXTRACT_PERM;
    // flags |= ARCHIVE_EXTRACT_SECURE_NOABSOLUTEPATHS;
    flags |= ARCHIVE_EXTRACT_SECURE_NODOTDOT;
    flags |= ARCHIVE_EXTRACT_SECURE_SYMLINKS;
    flags |= ARCHIVE_EXTRACT_TIME;

    a = archive_read_new();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);
    ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, flags);
    archive_write_disk_set_standard_lookup(ext);
    if ((err = archive_read_open_filename(a, src, BUFSIZ)) != ARCHIVE_OK) {
        err = ZIPPER_ENOENT;
        goto cleanup;
    }

    while (1) {
        int a_err = archive_read_next_header(a, &entry);
        if (a_err == ARCHIVE_EOF) {
            err = ZIPPER_OK;
            break;
        } else if (a_err != ARCHIVE_OK) {
            err = ZIPPER_EREAD;
            goto cleanup;
        }

        char ext_path[OS_MAX_FILENAME];
        int n = snprintf(
            ext_path,
            ARRAY_SIZE(ext_path),
            "%s%c%s",
            dest,
            OS_SEPARATOR,
            archive_entry_pathname(entry));
        if (n >= (int)ARRAY_SIZE(ext_path)) {
            err = ZIPPER_ENAMETOOLONG;
            goto cleanup;
        }
        archive_entry_set_pathname(entry, ext_path);

        a_err = archive_write_header(ext, entry);
        if (a_err != ARCHIVE_OK) {
            err = ZIPPER_EWRITE;
            goto cleanup;
        }

        err = copy_data(a, ext);
        if (err != ZIPPER_OK) {
            goto cleanup;
        }

        a_err = archive_write_finish_entry(ext);
        if (a_err != ARCHIVE_OK) {
            err = ZIPPER_EWRITE;
            goto cleanup;
        }
    }

cleanup:
    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);
    return err;
}
