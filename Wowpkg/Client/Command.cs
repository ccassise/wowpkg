using System.Collections.ObjectModel;
using Wowpkg.Catalog;
using Wowpkg.Addon;
using System.Reflection;

namespace Wowpkg.Client;

public static class Command
{
    static private readonly string _catalogPath = "Catalog";
    static private readonly Version? _version = Assembly.GetEntryAssembly()?.GetName().Version;
    static private readonly string _userAgent = $"Wowpkg/{_version}";

    public static async Task Install(Database db, Config config, string[] args)
    {
        if (args.Length <= 1 || !string.Equals(args[0], "install", StringComparison.OrdinalIgnoreCase))
        {
            throw new ArgumentException("Invalid arguments");
        }
        if (config?.Retail?.AddonsPath == null)
        {
            throw new ArgumentException("AddonsPath is not set in the config file");
        }

        ReadOnlyCollection<InstalledAddon> installedAddons = db.Load();
        LocalCatalog catalog = new(_catalogPath);
        CatalogItem?[] items = args[1..]
            .Select(catalog.Find)
            .ToArray();
        for (var i = 0; i < items.Length; i++)
        {
            if (items[i] == null)
            {
                Console.Error.WriteLine("Could not find addon: {0}", args[i]);
                continue;
            }
            else if (installedAddons.Any(a => string.Equals(a.Name, items[i]?.Name, StringComparison.OrdinalIgnoreCase)))
            {
                Console.Error.WriteLine("Skipping already installed addon: {0}", items[i]?.Name);
                items[i] = null;
            }
        }

        using HttpClient githubHttpClient = GitHubAddon.CreateDefaultHttpClient(_userAgent, config.GitHubToken);
        IAddon[] addons = items
            .Where(i => i != null)
            .Select(i => new GitHubAddon(i!, githubHttpClient))
            .ToArray();

        foreach (var addon in addons)
        {
            Console.WriteLine("Info: {0} ({1})", addon.Name, addon.Uri);
            await addon.Info();
        }

        foreach (var addon in addons)
        {
            Console.WriteLine("Package: {0}", addon.Name);
            await addon.Package();
            Console.WriteLine("Unpack: {0} to {1}", addon.Name, config.Retail.AddonsPath);
            await addon.Unpack(config.Retail.AddonsPath);
            db.Add(addon);
            foreach (var dir in addon.Directories)
            {
                Console.WriteLine("Move: {0} -> {1}", dir, config.Retail.AddonsPath);
            }
        }

        await db.SaveAsync();
    }

    public static void List(Database db, string[] args)
    {
        if (args.Length == 0 || !string.Equals(args[0], "list", StringComparison.OrdinalIgnoreCase))
        {
            throw new Exception("Invalid arguments");
        }

        ReadOnlyCollection<InstalledAddon> installed = db.Load();
        foreach (var addon in installed)
        {
            Console.WriteLine("{0} ({1})", addon.Name, addon.Version);
        }
    }

    public static async Task Remove(Database db, Config config, string[] args)
    {
        if (args.Length <= 1 || !string.Equals(args[0], "remove", StringComparison.OrdinalIgnoreCase))
        {
            throw new Exception("Invalid arguments");
        }
        if (config?.Retail?.AddonsPath == null)
        {
            throw new ArgumentException("AddonsPath is not set in the config file");
        }

        ReadOnlyCollection<InstalledAddon> installed = db.Load();
        foreach (var arg in args[1..])
        {
            var addon = installed.FirstOrDefault(a => string.Equals(a?.Name, arg, StringComparison.OrdinalIgnoreCase), null);
            if (addon == null)
            {
                Console.Error.WriteLine("Addon not installed: {0}", arg);
                continue;
            }
            Console.WriteLine("Remove addon: {0}", addon.Name);
            db.Remove(addon);
            foreach (var dir in addon.Directories)
            {
                Console.WriteLine("Remove: {0}", Path.Join(config.Retail.AddonsPath, dir));
                Directory.Delete(Path.Join(config.Retail.AddonsPath, dir), true);
            }
        }
        await db.SaveAsync();
    }

    public static async Task Search(string[] args)
    {
        if (args.Length != 2 || !string.Equals(args[0], "search", StringComparison.OrdinalIgnoreCase))
        {
            throw new Exception("Invalid arguments");
        }

        LocalCatalog catalog = new(_catalogPath);
        var found = catalog.Search(args[1]);
        foreach (var item in found)
        {
            Console.WriteLine("Name: {0}", item.Name);
            Console.WriteLine("Description: {0}", item.Description);
            Console.WriteLine("Uri: {0}", item.Uri);
            Console.WriteLine("");
        }
        await Task.CompletedTask;
    }

    public static async Task Show(Config config, string[] args)
    {
        if (args.Length != 2 || !string.Equals(args[0], "show", StringComparison.OrdinalIgnoreCase))
        {
            throw new Exception("Invalid arguments");
        }

        LocalCatalog catalog = new(_catalogPath);
        var found = catalog.Find(args[1]);
        if (found == null)
        {
            Console.Error.WriteLine("Could not find addon: {0}", args[1]);
            return;
        }


        using HttpClient githubHttpClient = GitHubAddon.CreateDefaultHttpClient(_userAgent, config.GitHubToken);
        var addon = new GitHubAddon(found, githubHttpClient);
        await addon.Info();

        Console.WriteLine("Name: {0}", addon.Name);
    }

    public static async Task Upgrade(Database db, Config config, string[] args)
    {
        if (args.Length < 1 || !string.Equals(args[0], "upgrade", StringComparison.OrdinalIgnoreCase))
        {
            throw new Exception("Invalid arguments");
        }
        if (config?.Retail?.AddonsPath == null)
        {
            throw new ArgumentException("AddonsPath is not set in the config file");
        }

        LocalCatalog catalog = new(_catalogPath);
        ReadOnlyCollection<InstalledAddon> toUpdate;
        if (args.Length == 1)
        {
            toUpdate = db.Load();
        }
        else
        {
            InstalledAddon[] installedAddons = [];
            foreach (var arg in args[1..])
            {
                var found = db.Load()
                    .FirstOrDefault(a => string.Equals(a?.Name, arg, StringComparison.OrdinalIgnoreCase), null)
                        ?? throw new ArgumentException($"Could not find installed addon: {arg}");
                installedAddons = [.. installedAddons.Append(found)];
            }
            toUpdate = installedAddons.AsReadOnly();
        }

        using HttpClient githubHttpClient = GitHubAddon.CreateDefaultHttpClient(_userAgent, config.GitHubToken);
        IAddon[] addons = toUpdate
            .Select(installed => (installed, catalog.Find(installed.Name) ?? throw new InvalidOperationException($"Could not find installed addon in catalog: {installed.Name})")))
            .Select(t =>
            {
                var installed = t.installed;
                var catalogItem = t.Item2;
                return new GitHubAddon(
                    name: catalogItem.Name,
                    description: catalogItem.Description,
                    version: installed.Version,
                    uri: catalogItem.Uri,
                    directories: installed.Directories,
                    httpClient: githubHttpClient
                );
            })
            .ToArray();

        if (addons.Length != toUpdate.Count)
        {
            throw new InvalidOperationException("Addons to update does not match addons found");
        }

        foreach (var addon in addons)
        {
            Console.WriteLine("Info: {0} ({1})", addon.Name, addon.Uri);
            await addon.Info();
            var installed = toUpdate.FirstOrDefault(a => a?.Name == addon.Name, null)
                ?? throw new InvalidOperationException("Did not find addon in list of installed addons");
            if (addon.Version != installed.Version)
            {
                Console.WriteLine("Upgrading addon: {0} ({1}) -> ({2})", installed.Name, installed.Version, addon.Version);
                Console.WriteLine("Packaging: {0}", addon.Name);
                await addon.Package();

                db.Remove(installed);
                foreach (var dir in addon.Directories)
                {
                    Directory.Delete(Path.Join(config.Retail.AddonsPath, dir), true);
                }

                Console.WriteLine("Unpacking: {0}", addon.Name);
                await addon.Unpack(config.Retail.AddonsPath);
                db.Add(addon);
                foreach (var dir in addon.Directories)
                {
                    Console.WriteLine("Move: {0} -> {1}", dir, config.Retail.AddonsPath);
                }
            }
        }
        await db.SaveAsync();
    }
}