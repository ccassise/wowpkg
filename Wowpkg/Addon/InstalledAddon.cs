using System.Text.Json.Serialization;

namespace Wowpkg.Addon;

public record InstalledAddon
{
    [JsonInclude]
    public required string Name;

    [JsonInclude]
    public required string Version;

    [JsonInclude]
    public required string[] Directories;

    public static InstalledAddon FromAddon(IAddon addon)
    {
        return new InstalledAddon
        {
            Name = addon.Name,
            Version = addon.Version,
            Directories = addon.Directories,
        };
    }
}