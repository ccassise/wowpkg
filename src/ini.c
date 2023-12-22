#include <ctype.h>
#include <stdlib.h>

#include "ini.h"

typedef struct INI {
    FILE *file;
    int ch; /* last read character from stream. */

    size_t row; /* Current row in file. */
    size_t col; /* Position in line of last read character. */

    int err; /* Last error encountered, if any. */
    size_t err_row; /* The row that the last error was found. */
    size_t err_col; /* The col that the last error was found. */

    INIKey key;
} INI;

/**
 * Sets error and the row and column that the error occurred.
 */
#define INI_SET_ERR(ini, ini_err)    \
    do {                             \
        (ini)->err = ini_err;        \
        (ini)->err_row = (ini)->row; \
        (ini)->err_col = (ini)->col; \
    } while (0)

static int ini_getc(INI *ini)
{
    if ((ini->ch = getc(ini->file)) != EOF) {
        ini->col++;
    }
    return ini->ch;
}

static int ini_ungetc(INI *ini)
{
    if (ini->col > 0) {
        ini->col--;
    }

    ini->ch = ungetc(ini->ch, ini->file);

    return ini->ch;
}

/**
 * Skips all whitespace except the newline character '\n'. When this function
 * terminates the next character in stream will either be a non-whitespace
 * character or '\n'.
 */
static void skip_space(INI *ini)
{
    while (ini_getc(ini) != EOF) {
        if (!isspace(ini->ch) || ini->ch == '\n') {
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
    while (ini_getc(ini) != EOF && ini->ch != terminating_ch) {
        /* Reached end of line before reaching terminating character. Since
         * multiline anything is not supported this is surely an error. */
        if (ini->ch == '\n') {
            return -1;
        } else if (ini->ch == '\r') {
            continue;
        }

        if (n > 0 && nwrote < n - 1) {
            buf[nwrote] = (char)ini->ch;
            buf[nwrote + 1] = '\0';
        }

        nwrote++;

        if (!isspace(ini->ch)) {
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

int ini_last_error(INI *ini)
{
    return ini->err;
}

size_t ini_last_error_row(INI *ini)
{
    return ini->err_row;
}

size_t ini_last_error_col(INI *ini)
{
    return ini->err_col;
}

INI *ini_open(const char *path)
{
    INI *result = calloc(1, sizeof(*result));
    if (result) {

        result->file = fopen(path, "rb");
        if (result->file == NULL) {
            free(result);
            return NULL;
        }

        result->row = 1;
        result->col = 0;

        result->key.section[0] = '\0';
        result->key.name[0] = '\0';
        result->key.value[0] = '\0';

        result->err = INI_OK;
        result->err_row = 0;
        result->err_col = 0;
    }

    return result;
}

void ini_close(INI *ini)
{
    if (ini == NULL) {
        return;
    }

    fclose(ini->file);
    free(ini);
}

INIKey *ini_readkey(INI *ini)
{
    /* Finding new key so reset old. */
    ini->key.name[0] = '\0';
    ini->key.value[0] = '\0';

    while (1) {
        /* Each iteration of this loop should parse an entire line. At the end of
         * the loop iteration the next read character in stream will either be
         * the beginning of the line or EOF. */

        ini->row++;
        ini->col = 0;

        int n = 0;

        skip_space(ini);

        ini_getc(ini);
        switch (ini->ch) {
        case '\n':
            break;
        case ';':
            parse_text(ini, NULL, 0, '\n');
            break;
        case '[':
            skip_space(ini);
            n = parse_text(ini, ini->key.section, INI_MAX_PROP, ']');
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

            n = parse_text(ini, ini->key.name, INI_MAX_PROP, '=');
            if (n < 0) {
                INI_SET_ERR(ini, INI_EPARSE);
                return NULL;
            } else if (n >= INI_MAX_PROP) {
                INI_SET_ERR(ini, INI_ENAMETOOLONG);
                return NULL;
            }

            skip_space(ini);

            n = parse_text(ini, ini->key.value, INI_MAX_PROP, '\n');
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
        if (ini->key.name[0] != '\0' || ini->key.value[0] != '\0') {
            break;
        }

        if (ini->ch == EOF) {
            ini->err = INI_OK;
            return NULL;
        }
    }

    return &ini->key;
}
