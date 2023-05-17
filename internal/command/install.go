package command

import (
	"fmt"
	"os"
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
			fmt.Fprintln(os.Stderr, err)
			continue
		}
		defer addon.Clean()

		if err = addon.Fetch(); err != nil {
			fmt.Fprintln(os.Stderr, err)
			continue
		}

		if err = addon.Package(); err != nil {
			fmt.Fprintln(os.Stderr, err)
			continue
		}

		if err = addon.Extract(cfg.UserCfg.AddonDir); err != nil {
			fmt.Fprintln(os.Stderr, err)
			continue
		}

		cfg.AppCfg.Installed[strings.ToLower(arg)] = addon
		cfg.AppCfg.Latest[strings.ToLower(arg)] = addon
	}

	cfg.Save()

	return nil
}
