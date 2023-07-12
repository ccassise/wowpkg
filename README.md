# wowpkg
A CLI World of Warcraft addon manager. For those wanting a Linux package manager feel for managing addons.

Available for Windows and macOS.
![render1688854884163-min](https://github.com/ccassise/wowpkg/assets/58533624/8f9a8a1a-3bcf-49d2-ad8f-af6307c4e77a)

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
	- This name must be unique and contain no spaces.
2. See the [example addon](dev_only/example_addon.ini) for what should be in the new addon file.
4. Check that the addon is available by running `wowpkg info <addon_name>`

## Installing

### Windows
1. Run "wowpkg-VERSION-win64.exe" from latest release.
2. Create a wowpkg directory in %APPDATA% directory.
3. Copy [config.ini](dev_only/config.ini) to %APPDATA%\wowpkg and update the addons path to the path of your World of Warcraft AddOns directory.
4. Assuming wowpkg was installed to the default location. Add C:\Program Files\wowpkg\bin to user PATH environment variable. This makes is easy to run wowpkg from anywhere in the terminal.

### macOS
The below should work on Apple silicon. I have not been able to test on an Intel Mac.

1. Open "wowpkg-VERSION-Darwin.dmg" from latest release.
2. Drag the wowpkg directory to the Applications directory.
3. In the terminal:
	```
 	$ mkdir ~/.config/wowpkg
 	```
 4. Copy [config.ini](dev_only/config.ini) to ~/.config/wowpkg and update the addons path to the path of your World of Warcraft AddOns directory.
 5. Add wowpkg to your PATH by appending `export PATH="$PATH:/Applications/wowpkg/bin"` to `~/.zshrc` or equivalent config file for your terminal.

## Uninstalling

### Windows
1. Run the `Uninstall.exe` from the installed wowpkg directory.
2. Remove wowpkg\bin from your user PATH.
3. If you want to remove user config data, remove %APPDATA%\wowpkg directory.

### macOS
1. Remove /Applications/wowpkg directory.
2. Remove the export path from ~/.zshrc or terminal equivalent.
3. If you want to remove user config data, remove ~/.config/wowpkg directory.

## Running from source
The source should compile on Windows using MSVC, macOS using clang, and Linux using gcc. This repo uses [vcpkg](https://github.com/microsoft/vcpkg) for dependency management.

There are a couple of project specific cmake options to pass in that can change how the program is built.
| Option | Default | Description |
| --- | --- | --- |
| WOWPKG_ENABLE_SANITIZERS | OFF | Builds the program with or without sanitizers |
| WOWPKG_ENABLE_TESTS | OFF | Determines wether or not tests will be built |
| WOWPKG_USE_DEVELOPMENT_PATHS | OFF | When enabled the path to config.ini and location for saved.wowpkg will be set to [dev_only](dev_only) project directory. When disabled, the paths to config.ini and saved.wowpkg will be dependent on current OS. %APPDATA%/wowpkg for Windows and ~/.config/wowpkg for macOS/Linux. Generally, use development paths unless the project is being built for packaging/release. |

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
 
