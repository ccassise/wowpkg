using System.Reflection;
using System.Xml.Serialization;
using Wowpkg.Client;

if (args.Length == 0)
{
    Console.Error.WriteLine("TODO: PRINT HELP");
    Environment.Exit(1);
}

var exePath = Environment.GetCommandLineArgs()[0];
var exeDir = Directory.GetParent(exePath)?.FullName ??
    throw new InvalidOperationException("Could not get executable directory");
Directory.SetCurrentDirectory(exeDir);

string configPath = "WowpkgConfig.xml";
Config config = new();
try
{
    using (var configFile = File.OpenRead(configPath))
    {
        var xml = new XmlSerializer(typeof(Config));
        config = xml.Deserialize(configFile) as Config
            ?? throw new Exception("Could not get config data");
        if (config?.Retail?.AddonsPath == null)
        {
            throw new ArgumentException("AddonsPath is not set in the config file");
        }
    }
}
catch (Exception e)
{
    Console.Error.WriteLine(e.Message);
    Environment.Exit(1);
}

Database db = new(Path.Join(config.Retail.AddonsPath, "wowpkg.saved"));
try
{
    string command = args[0];
    switch (command.ToLower())
    {
        case "install":
            await Command.Install(db, config, args);
            break;
        case "list":
            Command.List(db, args);
            break;
        case "remove":
            await Command.Remove(db, config, args);
            break;
        case "search":
            await Command.Search(args);
            break;
        case "show":
            await Command.Show(config, args);
            break;
        case "upgrade":
            await Command.Upgrade(db, config, args);
            break;
        default:
            Console.Error.WriteLine("Unrecognized command: {0}", command);
            break;
    }
}
catch (Exception e)
{
    Console.Error.WriteLine(e.Message);
}
