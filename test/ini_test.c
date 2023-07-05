#include <assert.h>

#include "ini.h"
#include "osstring.h"
#include "wowpkg.h"

void test_ini_ok_basic(void)
{
    INI *ini = ini_open(WOWPKG_TEST_DIR "/ini_test_inputs/ok_basic.ini");
    assert(ini != NULL);

    INIKey *key = ini_readkey(ini);
    assert(key != NULL);

    assert(strcmp(key->section, "section") == 0);
    assert(strcmp(key->name, "name") == 0);
    assert(strcmp(key->value, "value") == 0);

    assert(ini_readkey(ini) == NULL);
    assert(ini_last_error(ini) == INI_EEOF);

    ini_close(ini);
}

void test_ini_ok_basic_multiple(void)
{
    INI *ini = ini_open(WOWPKG_TEST_DIR "/ini_test_inputs/ok_basic_multiple.ini");
    assert(ini != NULL);

    INIKey *key = ini_readkey(ini);
    assert(key != NULL);
    assert(strcmp(key->section, "test section 1") == 0);
    assert(strcmp(key->name, "test1") == 0);
    assert(strcmp(key->value, "TEST1") == 0);

    key = ini_readkey(ini);
    assert(key != NULL);
    assert(strcmp(key->section, "test section 1") == 0);
    assert(strcmp(key->name, "test2") == 0);
    assert(strcmp(key->value, "TEST2") == 0);

    key = ini_readkey(ini);
    assert(key != NULL);
    assert(strcmp(key->section, "test section 2") == 0);
    assert(strcmp(key->name, "test3") == 0);
    assert(strcmp(key->value, "TEST3") == 0);

    key = ini_readkey(ini);
    assert(key != NULL);
    assert(strcmp(key->section, "test section 2") == 0);
    assert(strcmp(key->name, "test4") == 0);
    assert(strcmp(key->value, "TEST4") == 0);

    key = ini_readkey(ini);
    assert(key != NULL);
    assert(strcmp(key->section, "test section 2") == 0);
    assert(strcmp(key->name, "test5") == 0);
    assert(strcmp(key->value, "TEST5") == 0);

    assert(ini_readkey(ini) == NULL);
    assert(ini_last_error(ini) == INI_EEOF);

    ini_close(ini);
}

void test_ini_ok_empty(void)
{
    INI *ini = ini_open(WOWPKG_TEST_DIR "/ini_test_inputs/ok_empty.ini");
    assert(ini != NULL);

    INIKey *key = ini_readkey(ini);
    assert(key == NULL);
    assert(ini_last_error(ini) == INI_EEOF);

    ini_close(ini);
}

void test_ini_ok_max(void)
{
    INI *ini = ini_open(WOWPKG_TEST_DIR "/ini_test_inputs/ok_max.ini");
    assert(ini != NULL);

    INIKey *key = ini_readkey(ini);
    assert(key != NULL);

    assert(strcmp(key->section, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa") == 0);
    assert(strcmp(key->name, "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb") == 0);
    assert(strcmp(key->value, "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc") == 0);

    assert(ini_readkey(ini) == NULL);
    assert(ini_last_error(ini) == INI_EEOF);

    ini_close(ini);
}

void test_ini_ok_no_inline_comment(void)
{
    INI *ini = ini_open(WOWPKG_TEST_DIR "/ini_test_inputs/ok_no_inline_comment.ini");
    assert(ini != NULL);

    INIKey *key = ini_readkey(ini);
    assert(key != NULL);

    assert(strcmp(key->section, "") == 0);
    assert(strcmp(key->name, "name") == 0);
    assert(strcmp(key->value, "value ; parsed as part of value") == 0);

    assert(ini_readkey(ini) == NULL);
    assert(ini_last_error(ini) == INI_EEOF);

    ini_close(ini);
}

void test_ini_ok_no_name(void)
{
    INI *ini = ini_open(WOWPKG_TEST_DIR "/ini_test_inputs/ok_no_name.ini");
    assert(ini != NULL);

    INIKey *key = ini_readkey(ini);
    assert(key != NULL);

    assert(strcmp(key->section, "") == 0);
    assert(strcmp(key->name, "") == 0);
    assert(strcmp(key->value, "value") == 0);

    assert(ini_readkey(ini) == NULL);
    assert(ini_last_error(ini) == INI_EEOF);

    ini_close(ini);
}

void test_ini_ok_no_value(void)
{
    INI *ini = ini_open(WOWPKG_TEST_DIR "/ini_test_inputs/ok_no_value.ini");
    assert(ini != NULL);

    INIKey *key = ini_readkey(ini);
    assert(key != NULL);

    assert(strcmp(key->section, "") == 0);
    assert(strcmp(key->name, "name") == 0);
    assert(strcmp(key->value, "") == 0);

    assert(ini_readkey(ini) == NULL);
    assert(ini_last_error(ini) == INI_EEOF);

    ini_close(ini);
}

void test_ini_ok_section_only(void)
{
    INI *ini = ini_open(WOWPKG_TEST_DIR "/ini_test_inputs/ok_section_only.ini");
    assert(ini != NULL);

    INIKey *key = ini_readkey(ini);
    assert(key == NULL);
    assert(ini_last_error(ini) == INI_EEOF);

    ini_close(ini);
}

void test_ini_error_bad_name(void)
{
    INI *ini = ini_open(WOWPKG_TEST_DIR "/ini_test_inputs/error_bad_name.ini");
    assert(ini != NULL);

    INIKey *key = ini_readkey(ini);
    assert(key == NULL);

    assert(ini_last_error(ini) == INI_EPARSE);

    ini_close(ini);
}

void test_ini_error_bad_section(void)
{
    INI *ini = ini_open(WOWPKG_TEST_DIR "/ini_test_inputs/error_bad_section.ini");
    assert(ini != NULL);

    INIKey *key = ini_readkey(ini);
    assert(key == NULL);

    assert(ini_last_error(ini) == INI_EPARSE);

    ini_close(ini);
}

int main(void)
{
    test_ini_ok_basic();
    test_ini_ok_basic_multiple();
    test_ini_ok_empty();
    test_ini_ok_max();
    test_ini_ok_no_inline_comment();
    test_ini_ok_no_name();
    test_ini_ok_no_value();
    test_ini_ok_section_only();

    test_ini_error_bad_name();
    test_ini_error_bad_section();
}
