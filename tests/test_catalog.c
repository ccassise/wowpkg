#include <assert.h>

#include "catalog.h"
#include "osstring.h"
#include "wowpkg.h"

#include <stdio.h>

static void test_catalog_find_all()
{
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
    for (int i = 0; i < ARRAY_SIZE(names); i++) {
        CatalogItem item;
        assert(catalog_find(&item, WOWPKG_CATALOG_PATH, names[i]) == CATALOG_OK);
        assert(strcmp(item.name, names[i]) == 0);
        assert(strlen(item.desc) > 0);
        const char *uri_start = "https://api.github.com/repos/";
        assert(strncmp(item.uri, uri_start, strlen(uri_start)) == 0);
    }
}

static void test_catalog_find_none()
{
    CatalogItem item;
    assert(catalog_find(&item, WOWPKG_CATALOG_PATH, "does_not_exist") != CATALOG_OK);
}

static void test_catalog_find_case_insensitive()
{
    CatalogItem item;
    assert(catalog_find(&item, WOWPKG_CATALOG_PATH, "LiTtLeWiGs") == CATALOG_OK);
    assert(strcmp(item.name, "LittleWigs") == 0);
    assert(strlen(item.desc) > 0);
    const char *uri_start = "https://api.github.com/repos/";
    assert(strncmp(item.uri, uri_start, strlen(uri_start)) == 0);
}

int main()
{
    test_catalog_find_all();
    test_catalog_find_none();
    test_catalog_find_case_insensitive();

    return 0;
}