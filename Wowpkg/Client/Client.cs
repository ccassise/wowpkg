using System.Reflection;
using System.Text;
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

// try
// {
//     using (var configFile = File.OpenRead(configPath))
//     {
//         var xml = new XmlSerializer(typeof(Config));
//         config = xml.Deserialize(configFile) as Config;
//             // ?? throw new Exception("Could not get config data");
//         if (config?.Retail?.AddonsPath == null)
//         {
//             throw new ArgumentException("AddonsPath is not set in the config file");
//         }
//     }
// }
// catch (Exception e)
// {
//     Console.Error.WriteLine(e.Message);
//     Environment.Exit(1);
// }

try
{
    string configPath = "Config.xml";
    Config config = Config.Load(configPath);
    if (config?.Retail?.AddonsPath == null)
    {
        throw new ArgumentException("AddonsPath is not set in the config file");
    }

    string savedPath = Path.Join(config.Retail.AddonsPath, "wowpkg.saved");
    if (!File.Exists(savedPath))
    {
        Console.Error.Write("Warning: wowpkg.saved was not found. Managed ");
        Console.Error.Write("addon information may have been lost. If this is ");
        Console.Error.Write("the frist time running this program, this message ");
        Console.Error.Write("can be ignored.\n");
        Console.Error.WriteLine("Creating {0}", savedPath);
        FileStream f = File.Create(savedPath);
        f.Write(Encoding.UTF8.GetBytes("[]"));
        f.Close();
    }
    Database db = new(savedPath);
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
    Environment.Exit(1);
}
