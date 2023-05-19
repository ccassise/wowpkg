package command

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"

	"github.com/ccassise/wowpkg/internal/config"
	"github.com/ccassise/wowpkg/pkg/addon"
)

func Install(cfg *config.Config, args []string) error {
	if len(args) <= 1 {
		return &InvalidArgs{}
	}

	for _, arg := range args[1:] {
		addon, err := addon.New(arg)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error: [%s] %s\n", arg, err)
			continue
		}
		defer addon.Clean()

		fmt.Printf("==> [%s] downloading %s\n", addon.Name, addon.Url)
		if err = addon.Fetch(); err != nil {
			fmt.Fprintf(os.Stderr, "Error: [%s] %s\n", addon.Name, err)
			continue
		}

		fmt.Printf("==> [%s] packaging...\n", addon.Name)
		if err = addon.Package(); err != nil {
			fmt.Fprintf(os.Stderr, "Error: [%s] %s\n", addon.Name, err)
			continue
		}

		fmt.Printf("==> [%s] extracting files to %s\n", addon.Name, cfg.UserCfg.AddonPath)

		for _, dir := range addon.Dirs {
			newPath := filepath.Join(cfg.UserCfg.AddonPath, dir)
			if _, err := os.Stat(newPath); err == nil {
				fmt.Fprintf(os.Stderr, "Warning: replacing %s\n", newPath)
			}
		}

		if err = addon.Unpack(cfg.UserCfg.AddonPath); err != nil {
			fmt.Fprintf(os.Stderr, "Error: [%s] %s\n", addon.Name, err)
			continue
		}

		for _, dir := range addon.Dirs {
			fmt.Printf("moving %s to %s\n", dir, cfg.UserCfg.AddonPath)
		}

		cfg.AppState.Installed[strings.ToLower(arg)] = addon
		cfg.AppState.Latest[strings.ToLower(arg)] = addon
	}

	return nil
}
