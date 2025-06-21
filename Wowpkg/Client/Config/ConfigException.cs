namespace Wowpkg.Client;

public class WowpkgConfigException : Exception
{
    public WowpkgConfigException(string message)
    : base(message)
    {
    }

    public WowpkgConfigException(string message, Exception innerException)
    : base(message, innerException)
    {
    }
}
