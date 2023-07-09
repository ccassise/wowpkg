# wowpkg
A CLI World of Warcraft addon manager. For those wanting a Linux package manager feel for managing addons.

Available for Windows and macOS.

## Usage
```
wowpkg COMMAND [ARGS... | OPTIONS]

wowpkg info ADDON...
wowpkg install ADDON...
wowpkg list
wowpkg outdated
wowpkg remove ADDON...
wowpkg search TEXT
wowpkg update [ADDON...]
wowpkg upgrade [ADDON...]
```

ADDON for the following commands is the name of an addon. The name will include no spaces and is case-insenstivie. It otherwise should match exactly the addon name found in catalog.

Gets info for one or more addons. Things like name, description, installed status, and url used.
```
wowpkg info ADDON...
```

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
Currently only addons from GitHub that have releases are supported.

The addon catalog can be found in the [catalog](catalog) directory and is currently quite small. If there is an addon you want that is not in the catalog please create an issue or follow the steps below to add it to your wowpkg installation.

Addons can be added to the installed wowpkg/catalog directory.

Add the new addon to catalog by:
1. Creating a <addon_name>.ini file in `wowpkg/catalog/`.
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
The included installers make it easier to set up the below directory structure. The structure is important as the executable expects the files to be in these locations relative to itself. The location of this directory in the filesystem does not matter.

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
|   +-- *.ini
|
+-- saved.wowpkg
+-- config.ini
```

### Windows


### macOS


### Manually install
1. Create a directory with the below structure/files.
2. Create `config.ini` and paste the contents from [the example config.ini](dev_only/config.ini).
3. Add the path to `wowpkg/bin` to PATH. This makes it easy to run in a terminal from any location.

## Uninstalling
1. Delete the wowpkg directory from where it was installed. Using the default this would be `C:\Program Files\wowpkg` on Windows and `/Applications/wowpkg` on macOS.
2. Remove `path/to/wowpkg/bin` from PATH variable.

## Running from source
The source should compile on Windows using MSVC, macOS using clang, and Linux using gcc. This repo uses [vcpkg](https://github.com/microsoft/vcpkg) for dependency management.

There are a couple of project specific cmake options to pass in that can change how the program is built.
| Option | Default | Description |
| --- | --- | --- |
| WOWPKG_ENABLE_SANITIZERS | OFF | Builds the program with or without sanitizers |
| WOWPKG_ENABLE_TESTS | OFF | Determines wether or not tests will be built |
| WOWPKG_USE_DEVELOPMENT_PATHS | OFF | When enabled the path to config.ini and location for saved.wowpkg will be set to [dev_only](dev_only) project directory. When disabled, the paths to config.ini and saved.wowpkg will be relative from the executable location. Generally, use development paths unless the project is being built for packaging/release. |

1. Clone the repo.
2. Change to project directoy and get submodules.
	```
 	$ cd wowpkg
	$ git submodule update --init
	```
 3. Run vcpkg bootstrap with `./vcpkg/bootstrap-vcpkg.sh` or `./vcpkg/bootstrap-vcpkg.bat` depending on your system.
 4. Install dependencies with vcpkg.

	Windows
	```
 	$ ./vcpkg/vcpkg.exe install cjson:x64-windows curl:x64-windows minizip:x64-windows
 	```
 	macOS/Linux
	```
 	$ ./vcpkg/vcpkg install cjson curl minizip
 	```
6. Create a build directory and compile.
	```
	$ mkdir build
 	$ cd build
 	$ cmake .. -DWOWPKG_USE_DEVELOPMENT_PATHS:option=on
 	$ cmake --build .
	```
 7. Change the path in [config.ini](dev_only/config.ini) to where you want the addons to be extracted to. Something like `/path/to/wowpkg/dev_only/addons`.
 8. Run the compiled program.
 
