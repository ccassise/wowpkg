using System.IO.Compression;
using System.Net.Http.Json;
using Wowpkg.Addon.Models;
using Wowpkg.Catalog;

namespace Wowpkg.Addon;

public class GitHubAddon : IAddon, IDisposable
{
    public string Name { get => _name; }
    public string Description { get => _description; }
    public string Version { get => _version; }
    public string Uri { get => _uri; }
    public string[] Directories { get => _directories; }

    private readonly string _name;
    private readonly string _description;
    private string _version;
    private readonly string _uri;

    private string[] _zipUrls = [];
    private string[] _directories = [];

    // The directory that the zip files will be extracted to. Should always get
    // removed when the program exits, if not sooner.
    private DirectoryInfo? _tempDirectory;

    private readonly HttpClient _httpClient;

    public GitHubAddon(string name, string description, string version, string uri, string[] directories, HttpClient httpClient)
    {
        _name = name;
        _description = description;
        _version = version;
        _uri = uri;
        _directories = directories;
        _httpClient = httpClient;
    }

    ~GitHubAddon()
    {
        _tempDirectory?.Delete(recursive: true);
    }

    public void Dispose()
    {
        _tempDirectory?.Delete(recursive: true);
        GC.SuppressFinalize(this);
    }

    public GitHubAddon(CatalogItem item, HttpClient httpClient)
    {
        _name = item.Name;
        _description = item.Description;
        _version = "";
        _uri = item.Uri;
        _httpClient = httpClient;
    }

    /// <summary>
    /// Creates a default HTTP client that can be shared accross multiple
    /// instances of GitHubAddon.
    /// </summary>
    /// <param name="githubToken">
    /// If provided will forward to GitHub via the Autorization header.
    /// </param>
    /// <returns></returns>
    public static HttpClient CreateDefaultHttpClient(string userAgent, string? githubToken = null)
    {
        HttpClient httpClient = new();
        httpClient.DefaultRequestHeaders.Add("User-Agent", userAgent);
        httpClient.DefaultRequestHeaders.Add("X-GitHub-Api-Version", "2022-11-28");
        if (githubToken != null)
        {
            httpClient.DefaultRequestHeaders.Add("Authorization:", $"Bearer {githubToken}");
        }
        return httpClient;
    }

    /// <summary>
    /// Gets addon release information from GitHub. If this call is successful
    /// then the addon instance is ready to be Package()'d.
    /// </summary>
    /// <returns></returns>
    /// <exception cref="GitHubApiException">Thrown when a GitHub API request fails.</exception>
    public async Task Info()
    {
        var resp = await _httpClient.GetAsync(Uri);
        IEnumerable<string>? rateLimitRemaining = [];
        resp.Headers.TryGetValues("x-ratelimit-remaining", out rateLimitRemaining);
        var isAuth = _httpClient.DefaultRequestHeaders.Contains("Authorization");
        if ((resp.StatusCode == System.Net.HttpStatusCode.Forbidden || resp.StatusCode == System.Net.HttpStatusCode.Unauthorized) && isAuth)
        {
            throw new GitHubApiException($"GitHub response did not indicate success: {(int)resp.StatusCode}. Check that the GitHub token in config is valid and not expired.");
        }
        else if (resp.StatusCode == System.Net.HttpStatusCode.TooManyRequests)
        {
            throw new GitHubApiException("GitHub rate limit exceeded");
        }
        else if (resp.StatusCode == System.Net.HttpStatusCode.Forbidden && (rateLimitRemaining?.FirstOrDefault("0") ?? "0") == "0")
        {
            throw new GitHubApiException("GitHub rate limit exceeded");
        }
        else if (!resp.IsSuccessStatusCode)
        {
            throw new GitHubApiException($"GitHub response did not indicate success: {(int)resp.StatusCode}");
        }
        GitHubLatestRelease latestRelease = await resp.Content.ReadFromJsonAsync<GitHubLatestRelease>()
            ?? throw new GitHubApiException("GitHub response deserialized to null");

        _version = latestRelease.TagName;
        _zipUrls = latestRelease.Assets
            .Where(a => a.ContentType == "application/zip")
            .Select(a => a.BrowserDownloadUrl)
            .ToArray();
    }

    /// <summary>
    /// Package the addon by downloading and extracting the zip files associated
    /// to the addon. Stores them in a temporary directory.
    /// </summary>
    /// <returns></returns>
    /// <exception cref="InvalidOperationException">
    /// Generally would mean Package() was called before Info()
    /// </exception>
    public async Task Package()
    {
        if (_zipUrls.Length == 0)
        {
            throw new InvalidOperationException("There are no zip files to package");
        }
        _tempDirectory = Directory.CreateTempSubdirectory("wowpkg-");
        foreach (var zipUrl in _zipUrls)
        {
            var resp = await _httpClient.GetStreamAsync(zipUrl);
            ZipFile.ExtractToDirectory(resp, _tempDirectory.FullName);
        }
        _directories = Directory.GetDirectories(_tempDirectory.FullName)
            .Select(d => Path.GetFileName(d) ?? string.Empty)
            .ToArray();
    }

    /// <summary>
    /// Extracts directories to the target path. Path should be a directory.
    /// </summary>
    /// <param name="path"></param>
    /// <returns></returns>
    /// <exception cref="InvalidOperationException">
    /// Generally would mean Unpack() was called before Package().
    /// </exception>
    public async Task Unpack(string path)
    {
        if (_tempDirectory == null)
        {
            throw new InvalidOperationException("There is nothing to unpack");
        }
        foreach (var dir in _directories)
        {
            var from = Path.Join(_tempDirectory.FullName, dir);
            var to = Path.Join(path, dir);
            Directory.Move(from, to);
        }
        await Task.CompletedTask;
    }
}

public class GitHubApiException : Exception
{
    public GitHubApiException()
    : base("Request to GitHub failed")
    {
    }


    public GitHubApiException(string message)
    : base(message)
    {
    }

    public GitHubApiException(string message, Exception innerException)
    : base(message, innerException)
    {
    }
}
