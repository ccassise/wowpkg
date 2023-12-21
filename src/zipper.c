#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include <archive.h>

#include "osapi.h"
#include "wowpkg.h"
#include "zipper.h"

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
        }

        err = archive_write_data_block(writer, buf, size, offset);
        if (err != ARCHIVE_OK) {
            return ZIPPER_EWRITE;
        }
    }

    return (int)err;
}

int zipper_unzip(const char *src, const char *dest)
{
    if (src == NULL || dest == NULL) {
        return ZIPPER_ENOENT;
    }

    char pwd[OS_MAX_PATH];
    if (os_getcwd(pwd, ARRAY_SIZE(pwd)) == NULL) {
        if (errno == ENAMETOOLONG) {
            return ZIPPER_ENAMETOOLONG;
        }
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
    flags |= ARCHIVE_EXTRACT_SECURE_NOABSOLUTEPATHS;
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

    /* Change directory to dest so that libarchive will extract to it. */
    if (os_chdir(dest) != 0) {
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
    os_chdir(pwd);
    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);
    return err;
}
