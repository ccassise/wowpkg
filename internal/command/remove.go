package command

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"

	"github.com/ccassise/wowpkg/internal/config"
	"github.com/ccassise/wowpkg/pkg/addon"
)

func Remove(cfg *config.Config, args []string) error {
	for _, arg := range args[1:] {
		key := strings.ToLower(arg)

		installed, ok := cfg.AppState.Installed[key]
		if !ok {
			fmt.Fprintf(os.Stderr, "Error: [%s] %s\n", arg, &addon.NotFound{})
			continue
		}

		fmt.Printf("==> Remove %s\n", installed.Name)
		for _, dir := range installed.Dirs {
			toRemove := filepath.Join(cfg.UserCfg.AddonPath, dir)
			if err := os.RemoveAll(toRemove); err != nil {
				fmt.Fprintf(os.Stderr, "Error: [%s] failed to remove %s\n", installed.Name, toRemove)
				continue
			}
			fmt.Printf("removing %s\n", toRemove)
		}

		delete(cfg.AppState.Installed, key)
		delete(cfg.AppState.Latest, key)
	}

	return nil
}
