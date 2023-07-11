#pragma once

#include "term.h"

#define WOWPKG_NAME "wowpkg"

/**
 * This should be set by CMake at compile time to the same version as the CMake
 * project.
 */
#ifdef WOWPKG_VERSION
#define WOWPKG_USER_AGENT WOWPKG_NAME "/" WOWPKG_VERSION
#else
#define WOWPKG_VERSION ""
#define WOWPKG_USER_AGENT WOWPKG_NAME
#endif

/**
 * Path to catalog. If this is not set then it is assumed that it is building
 * for release and the path will be relative to the executable.
 */
#ifndef WOWPKG_CATALOG_PATH
#define WOWPKG_CATALOG_PATH "../catalog"
#endif

/*****************************************************************************/

#define PRINT_ERROR(...) fprintf(stderr, TERM_WRAP(TERM_BOLD_RED, "Error: ") __VA_ARGS__)
#define PRINT_WARNING(...) fprintf(stderr, TERM_WRAP(TERM_BOLD_YELLOW, "Warning: ") __VA_ARGS__)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define UNUSED(v) ((void)(v))
