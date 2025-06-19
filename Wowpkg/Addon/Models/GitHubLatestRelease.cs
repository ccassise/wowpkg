using System.Text.Json.Serialization;

namespace Wowpkg.Addon.Models;

public record GitHubLatestRelease
{
    [JsonInclude]
    [JsonPropertyName("id")]
    public required int Id;

    [JsonInclude]
    [JsonPropertyName("tag_name")]
    public required string TagName;

    [JsonInclude]
    [JsonPropertyName("name")]
    public required string Name;

    [JsonInclude]
    [JsonPropertyName("assets")]
    public required GitHubAsset[] Assets;
}