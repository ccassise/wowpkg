# $outputDirectory = "wowpkg-win-x64-1.0.0"
# dotnet publish .\Client\Client.csproj -r win-x64 -p:PublishSingleFile=true --self-contained true -o $outputDirectory

# Remove-Item "$outputDirectory\*.pdb"
# New-Item "$outputDirectory\bin" -ItemType Directory
# New-Item "$outputDirectory\Catalog" -ItemType Directory
# Move-Item -Path "$outputDirectory\*.exe" -Destination "$outputDirectory\bin"
# Copy-Item -Path ".\Data\Catalog\*.xml" -Destination "$outputDirectory\Catalog"
# Copy-Item -Path ".\WowpkgConfig.xml" -Destination "$outputDirectory"