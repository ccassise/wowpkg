package command

import (
	"fmt"

	"github.com/ccassise/wowpkg/internal/config"
)

func Outdated(cfg *config.Config, args []string) error {
	for name, installed := range cfg.AppCfg.Installed {
		latest := cfg.AppCfg.Latest[name]
		if latest.Version != installed.Version {
			fmt.Printf("\t%s(%s) < %s\n", installed.Name, installed.Version, latest.Version)
		}
	}

	return nil
}
