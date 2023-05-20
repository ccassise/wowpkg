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
		lowerArg := strings.ToLower(arg)

		installed, ok := cfg.AppState.Installed[lowerArg]
		if !ok {
			fmt.Fprintf(os.Stderr, "Error: [%s] %s\n", arg, &addon.NotFound{})
			continue
		}

		fmt.Printf("==> Remove %s\n", installed.Name)
		dirs := cfg.AppState.Installed[lowerArg].Dirs
		for _, dir := range dirs {
			toRemove := filepath.Join(cfg.UserCfg.AddonPath, dir)
			if err := os.RemoveAll(toRemove); err != nil {
				fmt.Fprintf(os.Stderr, "Error: [%s] failed to remove %s\n", installed.Name, toRemove)
				continue
			}
			fmt.Printf("removing %s\n", toRemove)
		}

		delete(cfg.AppState.Installed, lowerArg)
		delete(cfg.AppState.Latest, lowerArg)
	}

	return nil
}
