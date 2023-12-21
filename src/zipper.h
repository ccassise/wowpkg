#pragma once

enum {
    ZIPPER_OK = 0,

    ZIPPER_ENOENT,
    ZIPPER_ENAMETOOLONG,
    ZIPPER_EWRITE,
    ZIPPER_EREAD,
};

/**
 * Unzips a .zip archive at src to dest. It is expected that dest exists and is
 * a directory.
 *
 * On success returns ZIPPER_OK. On error returns one of the ZIPPER_E values.
 */
int zipper_unzip(const char *src, const char *dest);
