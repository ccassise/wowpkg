package command

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"

	"github.com/ccassise/wowpkg/internal/config"
	"github.com/ccassise/wowpkg/pkg/addon"
)

func Uninstall(cfg *config.Config, args []string) error {
	for _, arg := range args[1:] {
		lowerArg := strings.ToLower(arg)

		_, ok := cfg.AppCfg.Installed[lowerArg]
		if !ok {
			fmt.Fprintf(os.Stderr, "%s\n", &addon.NotFound{Name: arg})
			continue
		}

		folders := cfg.AppCfg.Installed[lowerArg].Folders
		for _, folder := range folders {
			if err := os.RemoveAll(filepath.Join(cfg.UserCfg.AddonDir, folder)); err != nil {
				return err
			}
		}

		delete(cfg.AppCfg.Installed, lowerArg)
		delete(cfg.AppCfg.Latest, lowerArg)
	}

	cfg.Save()

	return nil
}
