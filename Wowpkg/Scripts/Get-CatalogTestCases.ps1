param (
    [Parameter(Mandatory = $true)][string]$CatalogPath
)

$files = Get-Item -Path "$CatalogPath\*.xml"
foreach ($file in $files) {
    $xmlString = Get-Content -Path $file
    $xml = [xml]$xmlString
    $searchName = $file.BaseName.ToLower()
    $name = $xml.CatalogItem.Name
    Write-Host "[InlineData(`"${searchName}`", `"${name}`")]"
}
