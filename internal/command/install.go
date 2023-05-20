package command

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"sync"

	"github.com/ccassise/wowpkg/internal/config"
	"github.com/ccassise/wowpkg/pkg/addon"
)

func Install(cfg *config.Config, args []string) error {
	if len(args) <= 1 {
		return &InvalidArgs{}
	}

	var wg sync.WaitGroup
	wg.Add(len(args[1:]))

	addons := make(chan *addon.Addon)
	for _, arg := range args[1:] {
		go func(arg string) {
			defer wg.Done()

			addon, err := addon.New(arg)
			if err != nil {
				fmt.Fprintf(os.Stderr, "Error: [%s] %s\n", arg, err)
				return
			}

			fmt.Printf("==> [%s] downloading %s\n", addon.Name, addon.Url)
			if err = addon.Fetch(); err != nil {
				fmt.Fprintf(os.Stderr, "Error: [%s] %s\n", addon.Name, err)
				return
			}

			addons <- addon
		}(arg)
	}

	go func() {
		wg.Wait()
		close(addons)
	}()

	for addon := range addons {
		func() {
			defer func() {
				fmt.Printf("==> [%s] cleaning up any temp files\n", addon.Name)
				addon.Clean()
			}()

			fmt.Printf("==> [%s] packaging...\n", addon.Name)
			if err := addon.Package(); err != nil {
				fmt.Fprintf(os.Stderr, "Error: [%s] %s\n", addon.Name, err)
				return
			}

			fmt.Printf("==> [%s] extracting files to %s\n", addon.Name, cfg.UserCfg.AddonPath)

			for _, dir := range addon.Dirs {
				newPath := filepath.Join(cfg.UserCfg.AddonPath, dir)
				if _, err := os.Stat(newPath); err == nil {
					fmt.Fprintf(os.Stderr, "Warning: replacing %s\n", newPath)
				}
			}

			if err := addon.Unpack(cfg.UserCfg.AddonPath); err != nil {
				fmt.Fprintf(os.Stderr, "Error: [%s] %s\n", addon.Name, err)
				return
			}

			for _, dir := range addon.Dirs {
				fmt.Printf("moving %s to %s\n", dir, cfg.UserCfg.AddonPath)
			}

			cfg.AppState.Installed[strings.ToLower(addon.Name)] = addon
			cfg.AppState.Latest[strings.ToLower(addon.Name)] = addon
		}()
	}

	return nil
}
