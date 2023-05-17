package command

import (
	"fmt"
	"os"
	"strings"

	"github.com/ccassise/wowpkg/internal/config"
	"github.com/ccassise/wowpkg/pkg/addon"
)

func Update(cfg *config.Config, args []string) error {
	var names []string
	if len(args) <= 1 {
		for k := range cfg.AppCfg.Installed {
			names = append(names, k)
		}
	} else {
		for _, arg := range args[1:] {
			if _, ok := cfg.AppCfg.Installed[strings.ToLower(arg)]; ok {
				names = append(names, strings.ToLower(arg))
			} else {
				err := addon.NotFound{Name: arg}
				fmt.Fprintf(os.Stderr, "%s\n", err.Error())
				continue
			}
		}
	}

	for _, name := range names {
		addon, err := addon.New(name)
		if err != nil {
			fmt.Fprintf(os.Stderr, "%s\n", err)
			continue
		}

		cfg.AppCfg.Latest[name] = addon
	}

	cfg.Save()

	return nil
}
