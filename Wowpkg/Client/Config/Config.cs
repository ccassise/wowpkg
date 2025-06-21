using System.Xml.Serialization;

namespace Wowpkg.Client;

public class Config
{
    public static Config Load(string path)
    {
        var xml = new XmlSerializer(typeof(Config));
        using var configFile = File.OpenRead(path);
        var config = xml.Deserialize(configFile) as Config
            ?? throw new WowpkgConfigException($"Invalid config file: {path}");
        return config;
    }

    public string? GitHubToken { get; init; }
    public WowVersionConfig? Retail { get; init; }
}
