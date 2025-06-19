namespace tests.Unit;

using Wowpkg.Catalog;

public class LocalCatalogTest
{
    [Trait("Category", "Unit")]
    [Theory]
    [InlineData("allthethings", "AllTheThings")]
    [InlineData("astralkeys", "AstralKeys")]
    [InlineData("bartender4", "Bartender4")]
    [InlineData("bigdebuffs", "BigDebuffs")]
    [InlineData("bigwigs", "BigWigs")]
    [InlineData("bigwigs_voice", "BigWigs_Voice")]
    [InlineData("dbm-dungeons", "DBM-Dungeons")]
    [InlineData("dbm-pvp", "DBM-PvP")]
    [InlineData("dbm-retail", "DBM-Retail")]
    [InlineData("dbm-spelltimers", "DBM-SpellTimers")]
    [InlineData("detailsdamagemeter", "DetailsDamageMeter")]
    [InlineData("gathermate2", "GatherMate2")]
    [InlineData("hekili", "Hekili")]
    [InlineData("littlewigs", "LittleWigs")]
    [InlineData("mythicdungeontools", "MythicDungeonTools")]
    [InlineData("omnibar", "OmniBar")]
    [InlineData("omnicc", "OmniCC")]
    [InlineData("plater", "Plater")]
    [InlineData("premadegroupsfilter", "PremadeGroupsFilter")]
    [InlineData("quartz", "Quartz")]
    [InlineData("raiderio-addon", "RaiderIO-AddOn")]
    [InlineData("rclootcouncil2", "RCLootCouncil2")]
    [InlineData("savedinstances", "SavedInstances")]
    [InlineData("sexymap", "SexyMap")]
    [InlineData("silverdragon", "SilverDragon")]
    [InlineData("simulationcraft", "Simulationcraft")]
    [InlineData("threatplates", "ThreatPlates")]
    [InlineData("transcriptor", "Transcriptor")]
    [InlineData("weakauras", "WeakAuras")]
    public void LocalCatalog_Find(string search, string addonName)
    {
        LocalCatalog catalog = new(".");

        CatalogItem? actual = catalog.Find(search);

        Assert.NotNull(actual);
        Assert.Equal(addonName, actual.Name);
        Assert.NotEmpty(actual.Description);
        Assert.NotEmpty(actual.Uri);
        var _ = new Uri(actual.Uri);
    }

    [Fact, Trait("Category", "Unit")]
    public void LocalCatalog_Find_Null()
    {
        LocalCatalog catalog = new(".");

        CatalogItem? actual = catalog.Find("does_not_exist");

        Assert.Null(actual);
    }
}