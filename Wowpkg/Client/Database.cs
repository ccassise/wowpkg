using System.Collections.ObjectModel;
using System.Text.Json;
using System.Xml.Serialization;
using Wowpkg.Addon;

namespace Wowpkg.Client;

public class Database
{
    private readonly List<InstalledAddon> _installedAddons = [];
    private readonly string _path;

    public Database(string path)
    {
        _path = path;
        using var reader = new StreamReader(_path);
        _installedAddons = JsonSerializer.Deserialize<List<InstalledAddon>>(reader.ReadToEnd()) ?? [];
    }

    public async Task SaveAsync()
    {
        var json = JsonSerializer.Serialize(_installedAddons);
        using var writer = new StreamWriter(_path);
        await writer.WriteLineAsync(json);
    }

    public ReadOnlyCollection<InstalledAddon> Load()
    {
        return _installedAddons.AsReadOnly();
    }

    public void Add(IAddon addon)
    {
        _installedAddons.Add(InstalledAddon.FromAddon(addon));
    }

    public bool Remove(InstalledAddon addon)
    {
        return _installedAddons.Remove(addon);
    }

    public bool Remove(IAddon addon)
    {
        return _installedAddons.Remove(InstalledAddon.FromAddon(addon));
    }
}