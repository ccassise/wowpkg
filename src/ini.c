#include <ctype.h>
#include <stdlib.h>

#include "ini.h"

/**
 * Sets error and the row and column that the error occurred.
 */
#define INI_SET_ERR(ini, err)          \
    do {                               \
        (ini)->_err = err;             \
        (ini)->_err_row = (ini)->_row; \
        (ini)->_err_col = (ini)->_col; \
    } while (0)

static int ini_getc(INI *ini)
{
    if ((ini->_ch = getc(ini->_f)) != EOF) {
        ini->_col++;
    }
    return ini->_ch;
}

static int ini_ungetc(INI *ini)
{
    if (ini->_col > 0) {
        ini->_col--;
    }

    ini->_ch = ungetc(ini->_ch, ini->_f);

    return ini->_ch;
}

/**
 * Skips all whitespace except the newline character '\n'. When this function
 * terminates the next character in stream will either be a non-whitespace
 * character or '\n'.
 */
static void skip_space(INI *ini)
{
    while (ini_getc(ini) != EOF) {
        if (!isspace(ini->_ch) || ini->_ch == '\n') {
            ini_ungetc(ini);
            return;
        }
    }
}

/**
 * Parses all text up to the terminating character or newline character,
 * whichever comes first, and places up to n - 1 characters into buffer. Buffer
 * will always be NULL terminated.
 *
 * Returns the number of characters placed into buffer or number of characters
 * that would have been placed into buffer were it to have enough space.
 *
 * If this function returns a positive value then the next character in stream
 * will be one past the terminating character.
 *
 * Returns -1 when the terminating character is not the newline character
 * and newline or end of file is reached before the terminating character.
 */
static int parse_text(INI *ini, char *buf, size_t n, char terminating_ch)
{
    if (n > 0) {
        buf[0] = '\0';
    }

    size_t nwrote = 0;
    size_t text_end = 0; /* one past the last non-whitespace character. */
    while (ini_getc(ini) != EOF && ini->_ch != terminating_ch) {
        /* Reached end of line before reaching terminating character. Since
         * multiline anything is not supported this is surely an error. */
        if (ini->_ch == '\n') {
            return -1;
        } else if (ini->_ch == '\r') {
            continue;
        }

        if (n > 0 && nwrote < n - 1) {
            buf[nwrote] = (char)ini->_ch;
            buf[nwrote + 1] = '\0';
        }

        nwrote++;

        if (!isspace(ini->_ch)) {
            text_end = nwrote;
        }
    }

    /* Trim ending whitespace and subtract it from result. */
    if (text_end < n) {
        buf[text_end] = '\0';
        nwrote = text_end;
    }

    return (int)nwrote;
}

INI *ini_open(const char *path)
{
    INI *result = calloc(1, sizeof(*result));
    if (result) {

        result->_f = fopen(path, "rb");
        if (result->_f == NULL) {
            free(result);
            return NULL;
        }

        result->_row = 1;
        result->_col = 0;

        result->_key.section[0] = '\0';
        result->_key.name[0] = '\0';
        result->_key.value[0] = '\0';

        result->_err = INI_OK;
        result->_err_row = 0;
        result->_err_col = 0;
    }

    return result;
}

void ini_close(INI *ini)
{
    if (ini == NULL) {
        return;
    }

    fclose(ini->_f);
    free(ini);
}

INIKey *ini_readkey(INI *ini)
{
    /* Finding new key so reset old. */
    ini->_key.name[0] = '\0';
    ini->_key.value[0] = '\0';

    while (1) {
        /* Each iteration of this loop should parse an entire line. At the end of
         * the loop iteration the next read character in stream will either be
         * the beginning of the line or EOF. */

        ini->_row++;
        ini->_col = 0;

        int n = 0;

        skip_space(ini);

        ini_getc(ini);
        switch (ini->_ch) {
        case '\n':
            break;
        case ';':
            parse_text(ini, NULL, 0, '\n');
            break;
        case '[':
            skip_space(ini);
            n = parse_text(ini, ini->_key.section, INI_MAX_PROP, ']');
            if (n < 0) {
                INI_SET_ERR(ini, INI_EPARSE);
                return NULL;
            } else if (n >= INI_MAX_PROP) {
                INI_SET_ERR(ini, INI_ENAMETOOLONG);
                return NULL;
            }

            skip_space(ini);
            ini_getc(ini); /* Discard newline. */

            break;
        default:
            /* parse_text expects the start of the name but the first character
             * was already read for the switch statement. */
            ini_ungetc(ini);

            n = parse_text(ini, ini->_key.name, INI_MAX_PROP, '=');
            if (n < 0) {
                INI_SET_ERR(ini, INI_EPARSE);
                return NULL;
            } else if (n >= INI_MAX_PROP) {
                INI_SET_ERR(ini, INI_ENAMETOOLONG);
                return NULL;
            }

            skip_space(ini);

            n = parse_text(ini, ini->_key.value, INI_MAX_PROP, '\n');
            if (n < 0) {
                INI_SET_ERR(ini, INI_EPARSE);
                return NULL;
            } else if (n >= INI_MAX_PROP) {
                INI_SET_ERR(ini, INI_ENAMETOOLONG);
                return NULL;
            }
        }

        /* Check this before EOF because the file may not have a newline at end
         * of file. */
        if (ini->_key.name[0] != '\0' || ini->_key.value[0] != '\0') {
            break;
        }

        if (ini->_ch == EOF) {
            ini->_err = INI_OK;
            return NULL;
        }
    }

    return &ini->_key;
}
