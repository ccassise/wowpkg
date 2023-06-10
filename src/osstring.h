#pragma once

#include <string.h>

#ifdef _WIN32
#define strdup _strdup
#define strcasecmp _stricmp
#endif
