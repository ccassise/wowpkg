package main

import (
	"fmt"
	"os"
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
		AppState: config.State{
			Installed: make(map[string]*addon.Addon),
			Latest:    make(map[string]*addon.Addon),
		},
		UserCfg: config.UserConfig{},
	}

	// TODO: Make sure this gives good error messages when JSON fails to decode.
	// TODO: If it doesn't find a config file just create it?
	// FIX: Handle when JSON fails to get parsed.
	if err := cfg.Load(); err != nil {
		fmt.Fprintf(os.Stderr, "%s\n", err)
		os.Exit(1)
	}

	var err error
	arg := os.Args[1]
	switch strings.ToLower(arg) {
	// case "create":
	// case "info":
	case "install":
		err = command.Install(&cfg, os.Args[1:])
	case "list":
		err = command.List(&cfg, os.Args[1:])
	case "outdated":
		err = command.Outdated(&cfg, os.Args[1:])
	case "remove":
		err = command.Remove(&cfg, os.Args[1:])
	case "search":
		err = command.Search(&cfg, os.Args[1:])
	case "update":
		err = command.Update(&cfg, os.Args[1:])
	case "upgrade":
		err = command.Upgrade(&cfg, os.Args[1:])
	default:
		fmt.Fprintf(os.Stderr, "this should be the help/usage message\n")
	}

	if err != nil {
		fmt.Fprintf(os.Stderr, "%s\n", err)
		os.Exit(1)
	}

	cfg.Save()
}
