using System.Text.Json.Serialization;

namespace Wowpkg.Addon.Models;

public record GitHubAsset
{
    [JsonInclude]
    [JsonPropertyName("id")]
    public required int Id;

    [JsonInclude]
    [JsonPropertyName("name")]
    public required string Name;

    [JsonInclude]
    [JsonPropertyName("content_type")]
    public required string ContentType;

    [JsonInclude]
    [JsonPropertyName("size")]
    public required int Size;

    [JsonInclude]
    [JsonPropertyName("created_at")]
    public required DateTime CreatedAt;

    [JsonInclude]
    [JsonPropertyName("updated_at")]
    public required DateTime UpdatedAt;

    [JsonInclude]
    [JsonPropertyName("browser_download_url")]
    public required string BrowserDownloadUrl;
}