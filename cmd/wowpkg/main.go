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
		fmt.Fprintf(os.Stderr, "Usage: wowpkg COMMAND [ARGS... | OPTIONS]\n")
		os.Exit(1)
	}

	cfg := config.Config{
		AppState: config.State{
			Installed: make(map[string]*addon.Addon),
			Latest:    make(map[string]*addon.Addon),
		},
		UserCfg: config.UserConfig{},
	}

	if err := cfg.AppState.Load(config.StatePath()); err != nil {
		fmt.Fprintf(os.Stderr, "%s\n", err)
		os.Exit(1)
	}

	if err := cfg.UserCfg.Load(config.CfgPath()); err != nil {
		fmt.Fprintf(os.Stderr, "%s\n", err)
		os.Exit(1)
	}

	var err error
	arg := os.Args[1]
	switch strings.ToLower(arg) {
	// case "create":
	// case "info":
	case "help":
		err = command.Help(&cfg, os.Args[1:])
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
		fmt.Fprintf(os.Stderr, "Error: unknown command '%s'\n", arg)
	}

	if err != nil {
		fmt.Fprintf(os.Stderr, "%s\n", err)
		os.Exit(1)
	}

	cfg.AppState.Save(config.StatePath())
}
