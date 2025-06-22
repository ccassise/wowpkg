#include <assert.h>

#include "osstring.h"
#include "wowpkg.h"

#include "catalog.h"

static void test_catalog_find_all(void)
{
    /* This should ideally include all items in the catalog. Please add items to
     * the array as they are added to catalog. */
    const char *names[] = {
        "AllTheThings",
        "AstralKeys",
        "Bartender4",
        "BigDebuffs",
        "BigWigs",
        "BigWigs_Voice",
        "DBM-Dungeons",
        "DBM-PvP",
        "DBM-Retail",
        "DBM-SpellTimers",
        "DetailsDamageMeter",
        "GatherMate2",
        "Hekili",
        "LittleWigs",
        "MythicDungeonTools",
        "OmniBar",
        "OmniCC",
        "Plater",
        "PremadeGroupsFilter",
        "Quartz",
        "RaiderIO-AddOn",
        "RCLootCouncil2",
        "SavedInstances",
        "SexyMap",
        "SilverDragon",
        "Simulationcraft",
        "ThreatPlates",
        "Transcriptor",
        "WeakAuras",
    };

    assert(ARRAY_SIZE(names) > 0);
    for (size_t i = 0; i < ARRAY_SIZE(names); i++) {
        CatalogItem item;
        assert(catalog_find(&item, WOWPKG_CATALOG_PATH, names[i]) == CATALOG_OK);
        assert(strcmp(item.name, names[i]) == 0);
        assert(strlen(item.desc) > 0);
        const char *uri_start = "https://api.github.com/repos/";
        assert(strncmp(item.uri, uri_start, strlen(uri_start)) == 0);
    }
}

static void test_catalog_find_none(void)
{
    CatalogItem item;
    assert(catalog_find(&item, WOWPKG_CATALOG_PATH, "does_not_exist") != CATALOG_OK);
}

static void test_catalog_find_case_insensitive(void)
{
    CatalogItem item;
    assert(catalog_find(&item, WOWPKG_CATALOG_PATH, "LiTtLeWiGs") == CATALOG_OK);
    assert(strcmp(item.name, "LittleWigs") == 0);
    assert(strlen(item.desc) > 0);
    const char *uri_start = "https://api.github.com/repos/";
    assert(strncmp(item.uri, uri_start, strlen(uri_start)) == 0);
}

static void test_catalog_search_multiple(void)
{
    CatalogSearch *cs = catalog_search_begin(WOWPKG_CATALOG_PATH, "dungeon");
    int nfound = 0;
    const char *expected_names[] = {
        "DBM-Dungeons",
        "MythicDungeonTools",
        "LittleWigs",
    };
    int found_list[ARRAY_SIZE(expected_names)] = { 0 };
    CatalogItem *found = NULL;
    while ((found = catalog_search_inext(cs)) != NULL) {
        nfound++;
        for (size_t i = 0; i < ARRAY_SIZE(expected_names); i++) {
            if (strcmp(expected_names[i], found->name) == 0) {
                found_list[i] = 1;
            }
        }
    }
    assert(nfound == ARRAY_SIZE(expected_names));
    for (size_t i = 0; i < ARRAY_SIZE(expected_names); i++) {
        assert(found_list[i] == 1);
    }

    catalog_search_end(cs);
}

static void test_catalog_search_one(void)
{
    CatalogSearch *cs = catalog_search_begin(WOWPKG_CATALOG_PATH, "sexymap");
    int nfound = 0;
    const char *expected_names[] = {
        "SexyMap",
    };
    int found_list[ARRAY_SIZE(expected_names)] = { 0 };
    CatalogItem *found = NULL;
    while ((found = catalog_search_inext(cs)) != NULL) {
        nfound++;
        for (size_t i = 0; i < ARRAY_SIZE(expected_names); i++) {
            if (strcmp(expected_names[i], found->name) == 0) {
                found_list[i] = 1;
            }
        }
    }
    assert(nfound == ARRAY_SIZE(expected_names));
    for (size_t i = 0; i < ARRAY_SIZE(expected_names); i++) {
        assert(found_list[i] == 1);
    }

    catalog_search_end(cs);
}

static void test_catalog_search_none(void)
{
    CatalogSearch *cs = catalog_search_begin(WOWPKG_CATALOG_PATH, "should_not_cannot_exist");
    int nfound = 0;
    CatalogItem *found = NULL;
    while ((found = catalog_search_inext(cs)) != NULL) {
        nfound++;
    }
    assert(nfound == 0);

    catalog_search_end(cs);
}

int main(void)
{
    test_catalog_find_all();
    test_catalog_find_none();
    test_catalog_find_case_insensitive();
    test_catalog_search_multiple();
    test_catalog_search_one();
    test_catalog_search_none();

    return 0;
}
