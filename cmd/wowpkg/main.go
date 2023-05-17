package main

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"

	"github.com/ccassise/wowpkg/internal/command"
	"github.com/ccassise/wowpkg/internal/config"
	"github.com/ccassise/wowpkg/pkg/addon"
)

func main() {
	if len(os.Args) <= 1 {
		fmt.Fprintf(os.Stderr, "this should be the help/usage message\n")
		os.Exit(1)
	}

	cfg := config.Config{
		AppCfg: config.AppConfig{
			Installed: make(map[string]*addon.Addon),
			Latest:    make(map[string]*addon.Addon),
			// Installed: map[string][]string{
			// 	"BigWigs":    {"BigWigs", "BigWigs_Aberrus", "BigWigs_Core", "BigWigs_DragonIsles", "BigWigs_Options", "BigWigs_Plugins", "BigWigs_VaultOfTheIncarnates"},
			// 	"LittleWigs": {"LittleWigs", "LittleWigs_BattleForAzeroth", "LittleWigs_BurningCrusade", "LittleWigs_Cataclysm", "LittleWigs_Classic", "LittleWigs_Legion", "LittleWigs_MistsOfPandaria", "LittleWigs_Shadowlands", "LittleWigs_WarlordsOfDraenor", "LittleWigs_WrathOfTheLichKing"},
			// },
		},
		UserCfg: config.UserConfig{
			AddonDir: filepath.Join("dump", "out"),
		},
	}

	// TODO: Make sure this gives good error messages when JSON fails to decode.
	// TODO: If it doesn't find a config file just create it?
	// FIX: When the config file is empty/doesn't exist json returns a EOF character. This error should be ignored.
	if err := cfg.Load(); err != nil {
		fmt.Fprintf(os.Stderr, "%s\n", err)
		os.Exit(1)
	}

	arg := os.Args[1]
	switch strings.ToLower(arg) {
	case "install":
		err := command.Install(&cfg, os.Args[1:])
		if err != nil {
			fmt.Fprintf(os.Stderr, "%s\n", err)
		}
	case "list":
		err := command.List(&cfg, os.Args[1:])
		if err != nil {
			fmt.Fprintf(os.Stderr, "%s\n", err)
		}
	case "outdated":
		err := command.Outdated(&cfg, os.Args[1:])
		if err != nil {
			fmt.Fprintf(os.Stderr, "%s\n", err)
		}
	case "search":
		err := command.Search(&cfg, os.Args[1:])
		if err != nil {
			fmt.Fprintf(os.Stderr, "%s\n", err)
		}
	case "uninstall":
		err := command.Uninstall(&cfg, os.Args[1:])
		if err != nil {
			fmt.Fprintf(os.Stderr, "%s\n", err)
		}
	case "update":
		err := command.Update(&cfg, os.Args[1:])
		if err != nil {
			fmt.Fprintf(os.Stderr, "%s\n", err)
		}
	case "upgrade":
		err := command.Upgrade(&cfg, os.Args[1:])
		if err != nil {
			fmt.Fprintf(os.Stderr, "%s\n", err)
		}
	// case "create"
	default:
		fmt.Fprintf(os.Stderr, "this should be the help/usage message\n")
	}
}
