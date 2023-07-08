#pragma once

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
 * The path to files/directories the project needs in order to run. If this is
 * not set by CMake then it is assumed this is being compiled for release and
 * these paths are the location of the files relative to the executable.
 */
#ifndef WOWPKG_SAVED_FILE_PATH
#define WOWPKG_SAVED_FILE_PATH "../saved.wowpkg"
#endif

#ifndef WOWPKG_CONFIG_FILE_PATH
#define WOWPKG_CONFIG_FILE_PATH "../config.ini"
#endif

#ifndef WOWPKG_CATALOG_PATH
#define WOWPKG_CATALOG_PATH "../catalog"
#endif

/*****************************************************************************/

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define UNUSED(v) ((void)(v))
