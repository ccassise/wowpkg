# wowpkg
A CLI World of Warcraft addon manager. For those wanting a Linux package manager feel for managing addons.

wowpkg is an extremely fast and lightweight addon manager. You probably have an addon that takes more space than the entirety of wowpkg's install size (<10MB as of this writing).

This is currently a very early but pretty functional program.

## Usage
```
wowpkg COMMAND [ARGS... | OPTIONS]

wowpkg install ADDON...
wowpkg list
wowpkg outdated
wowpkg remove ADDON...
wowpkg search TEXT
wowpkg update [ADDON...]
wowpkg upgrade [ADDON...]
```

ADDON for the following commands is the name of an addon. The name will include no spaces and is case-insenstivie. It otherwise should match exactly the addon name found in catalog.


Installs one or more addons.
```
wowpkg install ADDON...
```


Lists all currently installed/managed addons.
```
wowpkg list
```


Lists all outdated addons.

Currently, this command does not automatically call `update`. 
```
wowpkg outdated
```

Remove/uninstall given addons.
```
wowpkg remove ADDON...
```

Searches catalog for any addons that matches TEXT. TEXT should not include any whitespace.

Currently, only the addon names are searched.
```
wowpkg search TEXT
```

Updates the metadata of addons. If no addon is provided then all currently installed addon's metadata is updated. If one or more addons are provided then only the metadata of those will be updated.
```
wowpkg update [ADDON...]
```

Upgrades addons. If no addon is provided then all currently installed addons that are outdated will be upgraded.

Currently, this command does not update an addon's metadata. Meaning `update` will almost always want to be ran before `upgrade`. A typical way to update and upgrade all addons at once would be something like `wowpkg update && wowpkg upgrade`.
```
wowpkg upgrade [ADDON...]
```

Print a concise summary of all commands.
```
wowpkg help
```

## Catalog / Adding addon
Currently only addons from Github that have releases are supported.

The addon catalog can be found in the `catalog` folder and is currently quite small. If there is an addon you want that is not in the catalog please create an issue or follow the steps below to add it to your wowpkg installation.

Assuming the installer and default location was used then the wowpkg installation is in:
- `%APPDATA%\wowpkg` for Windows.

Add the new addon to catalog by:
1. Creating a <addon_name>.json file in `wowpkg/catalog/`.
	- This name must be unique.
2. Fill the contents of the file with the following json object.
	- `"handler"` can be ignored for now.
	- `"name"` should be the same name as the file.
	- `"desc"` can be anything.
	- `"url"` should be the Github latest release api. Use the following with {owner} and {repo} replaced with the respective owner/repo of the addon you are trying to add.
		- `https://api.github.com/repos/{owner}/{repo}/releases/latest`
	```
	{
		"handler": "",
		"name": "<name of file>",
		"desc": "<anything>",
		"url": "https://api.github.com/repos/{owner}/{repo}/releases/latest"
	}
	```
3. Check that the addon is available by running `wowpkg search <addon_name>`

## Installing
The included installers make it easier to set up the below directory structure. The structure is important as the executable expects the files to be in these locations relative to itself. The location of this folder in the filesystem does not matter.

### Windows
Double click the included `installer.bat`. This will create `%APPDATA%\wowpkg` and set it up with the below structure, copying any necessary files. The included installer will then add `wowpkg/bin` to user PATH.

### macOS
From a terminal run the included `installer.sh`. This will create `$HOME/.wowpkg` and set it up with the below structure, copying any necessary files. `$HOME/.wowpkg/bin` will then need to be added to PATH. This can be done by appending `export PATH="$PATH:$HOME/.wowpkg/bin` to `$HOME/.zshrc`.

### Manually install
1. Create a directory with the below structure/files.
2. Create `config.json` with the following contents.
	```
	{
		"addon_path": "path/to/World of Warcraft"
	}
	```
3. Create `state.json` with the contents being an empty json object: `{}`.
4. Add the path to `wowpkg/bin` to PATH. This makes it easy to run in a terminal from any location.
### File structure
```
wowpkg/
|
+-- bin/
|   |
|   +-- wowpkg.exe
|
+-- catalog/
|   |
|   +-- *.json
|
+-- state.json
+-- config.json
```
## Uninstalling
### Windows
Assuming wowpkg was installed with the included installer:
1. Delete the `wowpkg` directory.
	- `%APPDATA%\wowpkg` on Windows.
2. Remove the wowpkg entry from user PATH.
### macOS
Assuming wowpkg was installed with the included installer:
1. `rm -r $HOME/.wowpkg`
2. Remove the wowpkg entry from PATH if it was added.

## Running from source
1. Install Go and clone the repo.
2. cd into the newly cloned repo.
3. `$ mkdir dev_test/addons`
4. `$ go run ./cmd/wowpkg/`

The repo is setup so that the above should work without issues. However, if you are wanting to run the executable in a different way there are a few things to be aware of. There are three paths that need to be set. The two configuration files' path (state.json and config.json) and the catalog path.

These are found in `addon.go`'s `CatalogPath()` and `config.go`'s `CfgPath()` and `StatePath()`.
