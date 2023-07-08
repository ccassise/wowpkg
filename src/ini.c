#include <ctype.h>
#include <stdlib.h>

#include "ini.h"

/**
 * Skips all whitespace except the newline character '\n'. When this function
 * terminates the next character in stream will either be a non-whitespace
 * character or '\n'.
 */
static void skip_space(INI *ini)
{
    while ((ini->_pos = getc(ini->_inifile)) != EOF) {
        if (!isspace(ini->_pos) || ini->_pos == '\n') {
            ini->_pos = ungetc(ini->_pos, ini->_inifile);
            return;
        }

        ini->_err_col++;
    }
}

/**
 * Parses all text up to the terminating character and places up to n - 1
 * characters into buffer. Buffer will always be NULL terminated.
 *
 * Returns the number of characters placed into buffer or number of characters
 * that would have been placed into buffer were it to have enough space.
 *
 * If this function returns a positive value then the next character in stream
 * will be one past the terminating character.
 *
 * Returns -1 when the terminating character is not the end of line character
 * and end of line or end of file is reached before the terminating character.
 */
static int parse_text(INI *ini, char *buf, size_t n, char terminating_ch)
{
    if (n > 0) {
        buf[0] = '\0';
    }

    int result = 0;
    while ((ini->_pos = getc(ini->_inifile)) != EOF && ini->_pos != terminating_ch) {
        // Reached end of line before reaching terminating character. Since
        // multiline anything is not supported this is surely an error.
        if (ini->_pos == '\n') {
            return -1;
        } else if (ini->_pos == '\r') {
            continue;
        }

        if (n > 0 && (size_t)result < n - 1) {
            buf[result] = (char)ini->_pos;
            buf[result + 1] = '\0';
        }

        result++;
        ini->_err_col++;
    }

    if (n > 0 && (size_t)result < n - 1) {
        // Trim whitespace from end of buffer.
        while (result > 0 && isspace(buf[result - 1])) {
            buf[result - 1] = '\0';
            result--;
        }
    }

    return result;
}

INI *ini_open(const char *path)
{
    INI *result = calloc(1, sizeof(*result));
    if (result) {

        result->_inifile = fopen(path, "rb");
        if (result->_inifile == NULL) {
            free(result);
            return NULL;
        }

        result->_err_row = 0; // This will be incremented during the first read.
        result->_err_col = 0;

        result->_key.section[0] = '\0';
        result->_key.name[0] = '\0';
        result->_key.value[0] = '\0';
    }

    return result;
}

void ini_close(INI *ini)
{
    if (ini == NULL) {
        return;
    }

    fclose(ini->_inifile);
    free(ini);
}

INIKey *ini_readkey(INI *ini)
{
    // Finding new key so reset old.
    ini->_key.name[0] = '\0';
    ini->_key.value[0] = '\0';

    int n;
    while (1) {
        // Each loop of this should read and parse an entire line.

        ini->_err_row++;
        ini->_err_col = 1;

        skip_space(ini);

        ini->_pos = getc(ini->_inifile);
        switch (ini->_pos) {
        case '\r':
        case '\n':
            break;
        case ';':
            parse_text(ini, NULL, 0, '\n');
            ini->_pos = ungetc(ini->_pos, ini->_inifile);
            break;
        case '[':
            skip_space(ini);
            n = parse_text(ini, ini->_key.section, INI_MAX_PROP, ']');
            if (n < 0) {
                ini->_err = INI_EPARSE;
                return NULL;
            } else if (n >= INI_MAX_PROP) {
                ini->_err = INI_ENAMETOOLONG;
                return NULL;
            }

            skip_space(ini);

            break;
        default:
            // parse_text expects the start of the name but the first character
            // was already read.
            ini->_pos = ungetc(ini->_pos, ini->_inifile);

            n = parse_text(ini, ini->_key.name, INI_MAX_PROP, '=');
            if (n < 0) {
                ini->_err = INI_EPARSE;
                return NULL;
            } else if (n >= INI_MAX_PROP) {
                ini->_err = INI_ENAMETOOLONG;
                return NULL;
            }

            skip_space(ini);

            int ch = getc(ini->_inifile);
            ungetc(ch, ini->_inifile);

            n = parse_text(ini, ini->_key.value, INI_MAX_PROP, '\n');
            if (n < 0) {
                ini->_err = INI_EPARSE;
                return NULL;
            } else if (n >= INI_MAX_PROP) {
                ini->_err = INI_ENAMETOOLONG;
                return NULL;
            }
        }

        // Check this before EOF because the file may not have a newline at end
        // of file.
        if (ini->_key.name[0] != '\0' || ini->_key.value[0] != '\0') {
            break;
        }

        if (ini->_pos == EOF) {
            ini->_err = INI_EEOF;
            return NULL;
        }
    }

    return &ini->_key;
}
