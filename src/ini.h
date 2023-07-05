/**
 * OVERVIEW
 * --------
 *
 * A VERY simple read only streaming .ini parser.
 *
 * From here on the following terms will be defined as referring to the following:
 *   1. name - the text to the left of the '=' symbol.
 *   2. value - the text to the right of the '=' symbol.
 *   3. section - the text enclosed in '[' and ']'.
 *   4. key - the name and value pair under a section, if any.
 *
 * EXAMPLE:
 *   [section]
 *   name = value
 *
 * Comments
 * --------
 * Comments are lines where the first non-whitespace character is ';' and
 * anything following will be considered a comment until end of line. Inline
 * comments are not supported. ';' that are not at the beginning of the line
 * will be parsed as a literal ';' and be part of a key property.
 *
 * For the below example "GOOD comment" will be skipped, but "BAD comment" will
 * be considered as part of the value. The below example would be parsed as
 * follows:
 *   key.section equals "section"
 *   key.name equals "name"
 *   key.value equals "value ; BAD comment"
 *
 * EXAMPLE:
 *   [section]
 *   ; GOOD comment
 *   name = value ; BAD comment
 *
 * Text
 * ----
 * Section, name, and value text will have whitespace at the beginning and end
 * trimmed. Whitespace within the text is preserved. Values are always parsed as
 * strings. It is up to the caller to decide if it should be a different type.
 *
 * The below example would be parsed as follows:
 *   key.section would equal "some section"
 *   key.name would equal "here is a name"
 *   key.value would equal "here is      a value"
 *
 * EXAMPLE:
 *   [   some section   ]
 *   here is a name      =       here is      a value
 *
 * Section
 * -------
 * Sections are lines where the first non-whitespace character is '[' and
 * includes all text up to the termination ']'. Name and value pairs shall not
 * be on the same line as a section.
 *
 * Name and Value
 * --------------
 * Name and value pairs are lines where the first non-whitespace character is
 * not '[' and is not ';'. It includes all text until a '=' is reached. A value
 * is all text to the right of the '='. A value may be empty, in such a case
 * value would be parsed as the empty string.
 *
 * Case sensitivity
 * ----------------
 * The current design does not care about case sensitivity at all. It is up to
 * the caller to determine how case sensitivity should be handled.
 *
 * Escape characters
 * -----------------
 * There is no support for escape characters. All characters will be read as
 * literals.
 */

#pragma once

#include <stdio.h>

/**
 * The size of buffers used to store section, name, and value strings.
 *
 * DEV: If this size is to change please also update tests with 'max' in name.
 */
#define INI_MAX_PROP 512

typedef struct INIKey {
    char section[INI_MAX_PROP];
    char name[INI_MAX_PROP];
    char value[INI_MAX_PROP];
} INIKey;

typedef struct INI {
    FILE *_inifile;
    int _pos; // last read character from stream.

    int _err;
    size_t _err_row;
    size_t _err_col;

    INIKey _key;
} INI;

enum {
    INI_OK = 0,

    INI_EEOF,
    INI_EPARSE,
    INI_ENAMETOOLONG,
};

#define ini_last_error(i) ((i)->_err)

/**
 * Opens a .ini file for reading.
 */
INI *ini_open(const char *path);

/**
 * Closes the .ini file and destroys passed in INI object.
 *
 * Passing a NULL pointer will make this function return immediately with no
 * action.
 */
void ini_close(INI *ini);

/**
 * Parses the open .ini file for the next key and returns it.
 *
 * This function is gauranteed to read the file from top to bottom. That is to
 * say, if the key name 'name1' appears in the file before key name 'name2',
 * then this function will always return 'name1' first.
 *
 * WARNING: This function writes to internal buffers. Future calls may
 * invalidate the contents of the returned key. The caller should copy the
 * contents into its own buffer if they need to persist.
 *
 * This function only returns when it has found a name value pair or when end of
 * file was reached. Sections wth no name value pairs are effectively skipped.
 *
 * If this does not return null then it is gauranteed INIKey.section,
 * INIKey.name, and INIKey.value are all null terminated strings. As such,
 * INIKey properties will contain at most INI_MAX_PROP - 1 characters.
 *
 * Returns the key if it was found, NULL on errors or if end of file was
 * reached.
 */
INIKey *ini_readkey(INI *ini);
