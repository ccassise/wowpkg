namespace Wowpkg.Addon;

public interface IAddon
{
    /// <summary>
    /// The unique name of the addon.
    /// </summary>
    public string Name { get; }

    /// <summary>
    /// The addon description.
    /// </summary>
    public string Description { get; }

    /// <summary>
    /// The addon version.
    /// </summary>
    public string Version { get; }

    /// <summary>
    /// Use may be different per addon type. Generally may be used by Info() to
    /// get more complete information on the addon that is not or cannot be
    /// stored in the catalog.
    /// </summary>
    public string Uri { get; }

    /// <summary>
    /// The list of directories that the addon moved to the addon's install
    /// directory.
    /// </summary>
    public string[] Directories { get; }

    /// <summary>
    /// Gets more complete information about the addon. This may include things
    /// like version, location of zip files to extract, etc.
    /// </summary>
    /// <returns></returns>
    public Task Info();

    /// <summary>
    /// Performs any and all neccessary steps to prepare the addon for install.
    /// </summary>
    /// <returns></returns>
    public Task Package();

    /// <summary>
    /// Installs the addon by moving all of the relevant directories to the path
    /// given.
    /// </summary>
    /// <param name="path">The path to extract to.</param>
    /// <returns></returns>
    public Task Unpack(string path);
}
