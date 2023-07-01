#pragma once

#define WOWPKG_NAME "wowpkg"

#ifdef WOWPKG_VERSION
#define WOWPKG_USER_AGENT WOWPKG_NAME "/" WOWPKG_VERSION
#else
#define WOWPKG_VERSION
#define WOWPKG_USER_AGENT WOWPKG_NAME
#endif

#define ARRLEN(a) (sizeof(a) / sizeof(*(a)))
#define UNUSED(v) ((void)(v))
