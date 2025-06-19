namespace Wowpkg.Catalog;

public interface ICatalog
{
    /// <summary>
    /// Searches the catalog instance for the item that matches the name. Must
    /// be an exact match but is case insensitive.
    /// </summary>
    /// <param name="name"></param>
    /// <returns></returns>
    public CatalogItem? Find(string name);

    /// <summary>
    /// Searches the catalog and returns all items that contain the search term.
    /// It searches the "Name" and "Description" properties of the CatalogItem.
    /// </summary>
    /// <param name="searchTerm"></param>
    /// <returns></returns>
    public CatalogItem[] Search(string searchTerm);
}