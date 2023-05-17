package command

import (
	"fmt"
	"os"
	"strings"

	"github.com/ccassise/wowpkg/internal/config"
	"github.com/ccassise/wowpkg/pkg/addon"
)

func Upgrade(cfg *config.Config, args []string) error {
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
		installed := cfg.AppCfg.Installed[name]
		latest := cfg.AppCfg.Latest[name]

		if installed.Version == latest.Version {
			continue
		}

		fmt.Printf("==> %s: removing old folders\n", latest.Name)
		if err := Uninstall(cfg, []string{"uninstall", name}); err != nil {
			fmt.Fprintf(os.Stderr, "%s\n", err.Error())
			continue
		}

		fmt.Printf("==> %s: downloading %s\n", latest.Name, latest.Url)
		if err := latest.Fetch(); err != nil {
			fmt.Fprintf(os.Stderr, "%s\n", err.Error())
			continue
		}
		defer latest.Clean()

		fmt.Printf("==> %s: packaging...\n", latest.Name)
		if err := latest.Package(); err != nil {
			fmt.Fprintf(os.Stderr, "%s\n", err.Error())
			continue
		}

		fmt.Printf("==> %s: extracting files to %s\n", latest.Name, cfg.UserCfg.AddonDir)
		if err := latest.Extract(cfg.UserCfg.AddonDir); err != nil {
			fmt.Fprintf(os.Stderr, "%s\n", err.Error())
			continue
		}

		cfg.AppCfg.Installed[name] = latest
		cfg.AppCfg.Latest[name] = latest
	}

	cfg.Save()

	return nil
}
