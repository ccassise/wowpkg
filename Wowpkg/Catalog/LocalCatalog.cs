using System.Xml.Serialization;

namespace Wowpkg.Catalog;

/// <summary>
/// Represents a catalog that exists on disk.
/// </summary>
/// <param name="path">The directory that holds the catalog files.</param>
public class LocalCatalog(string path) : ICatalog
{
    public CatalogItem? Find(string name)
    {
        foreach (var file in Directory.EnumerateFiles(path))
        {
            var fileName = Path.GetFileNameWithoutExtension(file);
            if (string.Equals(fileName, name, StringComparison.OrdinalIgnoreCase))
            {
                using var fs = File.OpenRead(file);
                var xml = new XmlSerializer(typeof(CatalogItem));
                return xml.Deserialize(fs) as CatalogItem;
            }
        }
        return null;
    }

    public CatalogItem[] Search(string searchTerm)
    {
        CatalogItem[] result = [];
        var xml = new XmlSerializer(typeof(CatalogItem));
        foreach (var file in Directory.EnumerateFiles(path))
        {
            using var fs = File.OpenRead(file);
            CatalogItem catalogItem = xml.Deserialize(fs) as CatalogItem
                ?? throw new ArgumentException($"Failed to deserialize catalog item: {file}");
            if (
                catalogItem.Name.Contains(searchTerm, StringComparison.OrdinalIgnoreCase)
                || catalogItem.Description.Contains(searchTerm, StringComparison.OrdinalIgnoreCase)
            )
            {
                result = [.. result.Append(catalogItem)];
            }
        }
        return result;
    }
}