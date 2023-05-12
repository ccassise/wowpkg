package main

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"

	"github.com/ccassise/wowpkg/internal/command"
	"github.com/ccassise/wowpkg/internal/config"
)

func main() {
	if len(os.Args) <= 1 {
		fmt.Fprintf(os.Stderr, "this should be the help/usage message\n")
		os.Exit(1)
	}

	cfg := config.Config{
		AppCfg: config.AppConfig{
			Installed: make(map[string][]string),
			// Installed: map[string][]string{
			// 	"BigWigs":    {"BigWigs", "BigWigs_Aberrus", "BigWigs_Core", "BigWigs_DragonIsles", "BigWigs_Options", "BigWigs_Plugins", "BigWigs_VaultOfTheIncarnates"},
			// 	"LittleWigs": {"LittleWigs", "LittleWigs_BattleForAzeroth", "LittleWigs_BurningCrusade", "LittleWigs_Cataclysm", "LittleWigs_Classic", "LittleWigs_Legion", "LittleWigs_MistsOfPandaria", "LittleWigs_Shadowlands", "LittleWigs_WarlordsOfDraenor", "LittleWigs_WrathOfTheLichKing"},
			// },
		},
		UserCfg: config.UserConfig{
			AddonDir:   filepath.Join("dump", "out"),
			CatalogDir: "catalog",
		},
	}

	// TODO: Make sure this gives good error messages when JSON fails to decode.
	// TODO: If it doesn't find a config file just create it?
	if err := cfg.Load(); err != nil {
		fmt.Fprintf(os.Stderr, "%s\n", err)
		os.Exit(1)
	}

	arg := os.Args[1]
	switch strings.ToLower(arg) {
	case "install":
		fmt.Println(os.Args[1:])
		err := command.Install(&cfg, os.Args[1:])
		if err != nil {
			fmt.Fprintf(os.Stderr, "%s\n", err)
		}
		cfg.Save()
	case "uninstall":
		fmt.Println(cfg.AppCfg.Installed)
		err := command.Uninstall(&cfg, os.Args[1:])
		if err != nil {
			fmt.Fprintf(os.Stderr, "%s\n", err)
		}
		cfg.Save()
	// case "update"
	// case "upgrade"
	// case "outdated"
	default:
		fmt.Fprintf(os.Stderr, "this should be the help/usage message\n")
	}

	// f, err := os.Open(filepath.Join("catalog", "bigwigs.json"))
	// if err != nil {
	// 	fmt.Fprintln(os.Stderr, err)
	// 	os.Exit(1)
	// }

	// var pkgInfo addon.GHPkgInfo
	// bufReader := bufio.NewReader(f)
	// if err := json.NewDecoder(bufReader).Decode(&pkgInfo); err != nil {
	// 	fmt.Fprintln(os.Stderr, err)
	// 	os.Exit(1)
	// }

	// // fmt.Println(pkgInfo.Owner, pkgInfo.Repo, pkgInfo.Handler)

	// // zipDir := filepath.Join("dump", "zips")
	// pkg, err := addon.New(&pkgInfo)
	// if err != nil {
	// 	fmt.Println(err)
	// 	os.Exit(1)
	// }
	// defer pkg.Clean()
	// // defer pkgInfo.Clean()

	// // addonDir := "C:\\Program Files (x86)\\Battle.net\\World of Warcraft\\_retail_\\Interface\\AddOns"
	// addonDir := filepath.Join("dump", "out")

	// // if err = pkgInfo.Process(filepath.Join("dump", "out")); err != nil {
	// if err = pkg.Package(addonDir); err != nil {
	// 	fmt.Println(err)
	// 	os.Exit(1)
	// }
}
