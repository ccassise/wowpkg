package command

import (
	"fmt"

	"github.com/ccassise/wowpkg/internal/config"
)

func List(cfg *config.Config, args []string) error {
	fmt.Println("==> List")
	for _, addon := range cfg.AppCfg.Installed {
		fmt.Println(addon.Name)
	}

	return nil
}
