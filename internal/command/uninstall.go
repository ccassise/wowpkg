package command

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"

	"github.com/ccassise/wowpkg/internal/config"
)

func Uninstall(cfg *config.Config, args []string) error {
	for _, arg := range args[1:] {
		var caseSensitiveArg string
		for k := range cfg.AppCfg.Installed {
			if strings.EqualFold(k, arg) {
				caseSensitiveArg = k
				break
			}
		}

		folders, ok := cfg.AppCfg.Installed[caseSensitiveArg]
		if !ok {
			fmt.Fprintf(os.Stderr, "%s: %s\n", arg, notFound)
			continue
		}

		for _, folder := range folders {
			if err := os.RemoveAll(filepath.Join(cfg.UserCfg.AddonDir, folder)); err != nil {
				return err
			}
		}

		delete(cfg.AppCfg.Installed, caseSensitiveArg)
	}

	return nil
}
