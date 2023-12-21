#pragma once

#include <stdint.h>

enum {
    ZIPPER_OK = 0,

    ZIPPER_ENAMETOOLONG,
    ZIPPER_ENOENT,
    ZIPPER_EREAD,
    ZIPPER_EWRITE,
};

/**
 * Extracts the filename src or the buffer that contains a compressed archive to
 * the destination path dest. Destination path should exist before calling
 * extract.
 *
 * RETURNS:
 *  ZIPPER_OK       On success.
 *
 * ERRORS:
 *  ENAMETOOLONG    Destination path is greater than OS_MAX_PATH.
 *  ENOENT          Either source archive does not exists or is not valid or
 *                    destination path does not exist.
 *  ZIPPER_EREAD    Error when trying to read the source archive.
 *  ZIPPER_EWRITE   Error when trying to extract archive to destination path.
 */
int zipper_extract(const char *src, const char *dest);
int zipper_extract_buf(const void *buf, size_t buf_size, const char *dest);
