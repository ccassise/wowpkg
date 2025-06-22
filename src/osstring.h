#pragma once

#include <string.h>

#ifdef _WIN32
#define strdup _strdup
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

/**
 * Searches haystack to see if needle appears in the string, ignoring case.
 *
 * RETURNS:
 *  The beginning of the first occurrence of needle in haystack or NULL if
 *  needle was not found.
 */
const char *os_strcasestr(const char *haystack, const char *needle);
