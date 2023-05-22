#!/usr/bin/env sh

app_name=wowpkg

addon_path="_retail_/Interface/AddOns"

install_path="$HOME/.$app_name"

wow_dir="World of Warcraft"
catalog_dir=catalog
config_json=config.json
state_json=state.json

# Check that all necessary files are present for installation.
required_files=($app_name $catalog_dir)
for f in "${required_files[@]}"; do
    if ! test -f "$f" && ! test -d "$f"; then
        echo "Error: missing file needed for installation: $f"
        exit 1
    fi
done

if test -d "$install_path"; then
    echo "$install_path already exists"
    read -p "Reinstall? [y/N] " yesno
    if test "$yesno" != "y" && test "$yesno" != "Y"; then
        exit 1
    fi
fi

# If the config file already exists then the user has already gone through
# setup and should not have to enter their WoW path each time.
if ! test -f "$install_path/$config_json"; then
    search_paths=(
        "/Applications/$wow_dir"
    )

    for path in "${search_paths[@]}"; do
        if test -d "$path"; then
            wow_path=$path
            break
        fi
    done

    if test -z "${wow_path}"; then
        echo "$wow_dir directory not found"
        read -p "Enter path to \"$wow_dir\" directory: " wow_path
    fi

    if test -z "${wow_path}" || ! test -d "${wow_path}"; then
        echo "Error: could not find $wow_dir path"
        exit 1
    fi

    echo "Found $wow_path"
else
    echo "Found $config_json, skipping directory discovery"
fi

echo "$app_name will be installed to $install_path"
read -p "Continue [y/N] " yesno
if test "$yesno" != "y" && test "$yesno" != "Y"; then
    exit 1
fi

if ! test -d "$install_path"; then
    echo "Creating $install_path"
    if ! mkdir "$install_path"; then
        exit 1
    fi
fi

if ! test -d "$install_path/bin"; then
    echo "Creating $install_path/bin"
    if ! mkdir "$install_path/bin"; then
        exit 1
    fi
fi

echo "Copying files to $install_path"

if ! cp -r $catalog_dir "$install_path"; then
    exit 1
fi

if ! cp $app_name "$install_path/bin"; then
    exit 1
fi

if ! test -f "$install_path/$state_json"; then
    echo "Creating state.json"
    echo "{}" > $install_path/$state_json
fi

if ! test -f "$install_path/$config_json"; then
    echo "Creating config.json"
    echo "{\n\t\"addon_path\": \"$wow_path/$addon_path\"\n}" > $install_path/$config_json
fi

echo "Done"
