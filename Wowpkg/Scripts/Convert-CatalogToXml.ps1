param(
    [Parameter(Mandatory = $true)][string]$InputCatalogPath,
    [Parameter(Mandatory = $true)][string]$OutputCatalogPath
)

Get-ChildItem -Path "$InputCatalogPath\*.ini" |
ForEach-Object {
    $lines = Get-Content -Path $_.FullName |
    Select-String -Pattern " = " |
    ForEach-Object { $_.Line.Replace(" = ", "=") }

    $name = $_.Name.Replace(".ini", ".xml")
    $file = "${OutputCatalogPath}${name}"

    New-Item -Path $file -Force
    Add-Content -Path $file -Value '<?xml version="1.0" encoding="UTF-8"?>'
    Add-Content -Path $file -Value "<CatalogItem>"
    foreach ($line in $lines) {
        $key = $line.Split("=")[0]
        $value = $line.Split("=")[1]

        if ($key -eq "name") {
            Add-Content -Path $file -Value "`t<Name>${value}</Name>"
        }
        elseif ($key -eq "desc") {
            Add-Content -Path $file -Value "`t<Description>${value}</Description>"
        }
        elseif ($key -eq "url") {
            Add-Content -Path $file -Value "`t<Uri>${value}</Uri>"
        }
    }
    Add-Content -Path $file -Value "</CatalogItem>"
}
