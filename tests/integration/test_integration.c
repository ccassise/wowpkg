#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "addon.h"
#include "appstate.h"
#include "list.h"
#include "osapi.h"
#include "wowpkg.h"

static bool is_addons_dir_empty(void)
{
    OsDir *dir = os_opendir(WOWPKG_USER_FILE_DIR "/addons");
    assert(dir != NULL);
    OsDirEnt *entry = NULL;
    while ((entry = os_readdir(dir)) != NULL) {
        /* Ignore hidden files, '.', '..', and README.md */
        if (entry->name[0] == '.' || strcmp(entry->name, "README.md") == 0) {

            continue;
        }
        os_closedir(dir);
        return false;
    }
    os_closedir(dir);
    return true;
}

static void test_install_single(void)
{
    assert(system(WOWPKG_EXEC_PATH " install bigwigs") == EXIT_SUCCESS);
    assert(!is_addons_dir_empty());

    /* Check that installed addons show up in list as expected. */
    FILE *fpout = popen(WOWPKG_EXEC_PATH " list", "r");
    assert(fpout != NULL);
    char pout[32] = { '\0' };
    assert(fscanf(fpout, "BigWigs (%31[^)])\n", pout) == 1);
    assert(fgetc(fpout) == EOF);
    assert(pclose(fpout) == 0);

    /* Cleanup addons directory. */
    assert(system(WOWPKG_EXEC_PATH " remove bigwigs") == EXIT_SUCCESS);
    assert(is_addons_dir_empty());
}

static void test_install_multiple(void)
{
    assert(system(WOWPKG_EXEC_PATH " install bigwigs weakauras GATHERMATE2") == EXIT_SUCCESS);
    assert(!is_addons_dir_empty());

    /* Check that installed addons show up in list as expected. */
    FILE *fpout = popen(WOWPKG_EXEC_PATH " list", "r");
    assert(fpout != NULL);
    char pout[32] = { '\0' };
    assert(fscanf(fpout, "BigWigs (%31[^)])\n", pout) == 1);
    assert(fscanf(fpout, "GatherMate2 (%31[^)])\n", pout) == 1);
    assert(fscanf(fpout, "WeakAuras (%31[^)])\n", pout) == 1);
    assert(fgetc(fpout) == EOF);
    assert(pclose(fpout) == 0);

    /* Cleanup addons directory. */
    assert(system(WOWPKG_EXEC_PATH " remove bigwigs weakauras GATHERMATE2") == EXIT_SUCCESS);
    assert(is_addons_dir_empty());
}

/**
 * Installs all addons from catalog.
 */
static void test_stress(void)
{
    char cmd_args[1024] = { '\0' };
    OsDir *dir = os_opendir(WOWPKG_CATALOG_PATH);
    assert(dir != NULL);
    OsDirEnt *entry = NULL;
    while ((entry = os_readdir(dir)) != NULL) {
        if (strcmp(entry->name, ".") == 0 || strcmp(entry->name, "..") == 0) {
            continue;
        }
        /* Strip ending .ini extension. */
        char *ext = strstr(entry->name, ".ini");
        assert(ext != NULL);
        *ext = '\0';
        size_t cmd_len = strlen(cmd_args);
        assert(snprintf(cmd_args + cmd_len, ARRAY_SIZE(cmd_args) - cmd_len, " %s", entry->name) < (int)ARRAY_SIZE(cmd_args));
    }
    os_closedir(dir);

    /* Install addons. */
    char cmd_install[1024] = { '\0' };
    assert(snprintf(cmd_install, ARRAY_SIZE(cmd_install), "%s install %s", WOWPKG_EXEC_PATH, cmd_args) < (int)ARRAY_SIZE(cmd_install));
    assert(system(cmd_install) == 0);
    assert(!is_addons_dir_empty());

    /* Cleanup addons directory. */
    char cmd_remove[1024] = { '\0' };
    assert(snprintf(cmd_remove, ARRAY_SIZE(cmd_remove), "%s remove %s", WOWPKG_EXEC_PATH, cmd_args) < (int)ARRAY_SIZE(cmd_remove));
    assert(system(cmd_remove) == 0);
    assert(is_addons_dir_empty());
}

static void test_upgrade_single(void)
{
    assert(system(WOWPKG_EXEC_PATH " install bigwigs weakauras") == EXIT_SUCCESS);
    assert(!is_addons_dir_empty());

    /* Change addon versions so that `upgrade` can be triggered. */
    AppState *state = appstate_create();
    assert(state != NULL);
    assert(appstate_load(state, WOWPKG_USER_FILE_DIR "/saved.wowpkg") == APPSTATE_OK);
    assert(list_len(state->installed) == 2);
    assert(list_len(state->latest) == 2);
    assert(!is_addons_dir_empty());
    ListNode *n = NULL;
    list_foreach(n, state->latest)
    {
        Addon *addon = n->value;
        addon_set_str(&addon->version, strdup("0"));
    }
    assert(appstate_save(state, WOWPKG_USER_FILE_DIR "/saved.wowpkg") == APPSTATE_OK);
    appstate_free(state);

    /* Check that `outdated` gives expected output. */
    FILE *fpout = popen(WOWPKG_EXEC_PATH " outdated", "r");
    assert(fpout != NULL);
    char pout[32] = { '\0' };
    assert(fscanf(fpout, "BigWigs (%31[^)]) < (0)\n", pout) == 1);
    assert(fscanf(fpout, "WeakAuras (%31[^)]) < (0)\n", pout) == 1);
    assert(fgetc(fpout) == EOF);
    assert(pclose(fpout) == 0);

    /* Only upgrade a single addon. */
    assert(system(WOWPKG_EXEC_PATH " upgrade weakauras") == EXIT_SUCCESS);

    /* Check that `outdated` gives expected output. */
    fpout = popen(WOWPKG_EXEC_PATH " outdated", "r");
    assert(fpout != NULL);
    assert(fscanf(fpout, "BigWigs (%31[^)]) < (0)\n", pout) == 1);
    assert(fgetc(fpout) == EOF);
    assert(pclose(fpout) == 0);

    /* Cleanup addons directory. */
    assert(system(WOWPKG_EXEC_PATH " remove weakauras bigwigs") == EXIT_SUCCESS);
    assert(is_addons_dir_empty());
}

static void test_upgrade_all(void)
{
    assert(system(WOWPKG_EXEC_PATH " install bigwigs weakauras") == EXIT_SUCCESS);
    assert(!is_addons_dir_empty());

    /* Change addon versions so that `upgrade` can be triggered. */
    AppState *state = appstate_create();
    assert(state != NULL);
    assert(appstate_load(state, WOWPKG_USER_FILE_DIR "/saved.wowpkg") == APPSTATE_OK);
    assert(list_len(state->installed) == 2);
    assert(list_len(state->latest) == 2);
    assert(!is_addons_dir_empty());
    ListNode *n = NULL;
    list_foreach(n, state->latest)
    {
        Addon *addon = n->value;
        addon_set_str(&addon->version, strdup("0"));
    }
    assert(appstate_save(state, WOWPKG_USER_FILE_DIR "/saved.wowpkg") == APPSTATE_OK);
    appstate_free(state);

    /* Check that `outdated` gives expected output. */
    FILE *fpout = popen(WOWPKG_EXEC_PATH " outdated", "r");
    assert(fpout != NULL);
    char pout[32] = { '\0' };
    assert(fscanf(fpout, "BigWigs (%31[^)]) < (0)\n", pout) == 1);
    assert(fscanf(fpout, "WeakAuras (%31[^)]) < (0)\n", pout) == 1);
    assert(fgetc(fpout) == EOF);
    assert(pclose(fpout) == 0);

    /* upgrade all outdated addons. */
    assert(system(WOWPKG_EXEC_PATH " upgrade") == EXIT_SUCCESS);

    /* Check that `outdated` gives expected output. */
    fpout = popen(WOWPKG_EXEC_PATH " outdated", "r");
    assert(fpout != NULL);
    assert(fgetc(fpout) == EOF);
    assert(pclose(fpout) == 0);

    /* Cleanup addons directory. */
    assert(system(WOWPKG_EXEC_PATH " remove weakauras bigwigs") == EXIT_SUCCESS);
    assert(is_addons_dir_empty());
}

void test_update_single(void)
{
    assert(system(WOWPKG_EXEC_PATH " install bigwigs weakauras") == EXIT_SUCCESS);
    assert(!is_addons_dir_empty());

    /* Change addon versions so that `update` can be triggered. */
    AppState *state = appstate_create();
    assert(state != NULL);
    assert(appstate_load(state, WOWPKG_USER_FILE_DIR "/saved.wowpkg") == APPSTATE_OK);
    assert(list_len(state->installed) == 2);
    assert(list_len(state->latest) == 2);
    assert(!is_addons_dir_empty());
    ListNode *n = NULL;
    list_foreach(n, state->installed)
    {
        Addon *addon = n->value;
        if (strcmp(addon->name, "WeakAuras") == 0) {
            addon_set_str(&addon->version, strdup("0"));
        }
    }
    assert(appstate_save(state, WOWPKG_USER_FILE_DIR "/saved.wowpkg") == APPSTATE_OK);
    appstate_free(state);

    /* Only update a single addon. */
    assert(system(WOWPKG_EXEC_PATH " update weakauras") == EXIT_SUCCESS);

    /* Check that `outdated` gives expected output. */
    FILE *fpout = popen(WOWPKG_EXEC_PATH " outdated", "r");
    assert(fpout != NULL);
    char pout[32] = { '\0' };
    assert(fscanf(fpout, "WeakAuras (0) < (%31[^)])\n", pout) == 1);
    assert(fgetc(fpout) == EOF);
    assert(pclose(fpout) == 0);

    /* Cleanup addons directory. */
    assert(system(WOWPKG_EXEC_PATH " remove weakauras bigwigs") == EXIT_SUCCESS);
    assert(is_addons_dir_empty());
}

void test_update_all(void)
{
    assert(system(WOWPKG_EXEC_PATH " install bigwigs weakauras") == EXIT_SUCCESS);
    assert(!is_addons_dir_empty());

    /* Change addon versions so that `update` can be triggered. */
    AppState *state = appstate_create();
    assert(state != NULL);
    assert(appstate_load(state, WOWPKG_USER_FILE_DIR "/saved.wowpkg") == APPSTATE_OK);
    assert(list_len(state->installed) == 2);
    assert(list_len(state->latest) == 2);
    assert(!is_addons_dir_empty());
    ListNode *n = NULL;
    list_foreach(n, state->installed)
    {
        Addon *addon = n->value;
        addon_set_str(&addon->version, strdup("0"));
    }
    assert(appstate_save(state, WOWPKG_USER_FILE_DIR "/saved.wowpkg") == APPSTATE_OK);
    appstate_free(state);

    /* Only update a single addon. */
    assert(system(WOWPKG_EXEC_PATH " update") == EXIT_SUCCESS);

    /* Check that `outdated` gives expected output. */
    FILE *fpout = popen(WOWPKG_EXEC_PATH " outdated", "r");
    assert(fpout != NULL);
    char pout[32] = { '\0' };
    assert(fscanf(fpout, "BigWigs (0) < (%31[^)])\n", pout) == 1);
    assert(fscanf(fpout, "WeakAuras (0) < (%31[^)])\n", pout) == 1);
    assert(fgetc(fpout) == EOF);
    assert(pclose(fpout) == 0);

    /* Cleanup addons directory. */
    assert(system(WOWPKG_EXEC_PATH " remove weakauras bigwigs") == EXIT_SUCCESS);
    assert(is_addons_dir_empty());
}
int main(void)
{
    test_install_single();
    test_install_multiple();
    // test_stress();
    test_upgrade_single();
    test_upgrade_all();
    test_update_single();
    test_update_all();

    return EXIT_SUCCESS;
}
