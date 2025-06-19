namespace Wowpkg.Client;

public record Config
{
    public string? GitHubToken;
    public WowVersionConfig? Retail;
}

public record WowVersionConfig
{
    public required string AddonsPath;
}