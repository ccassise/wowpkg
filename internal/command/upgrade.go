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
		for k := range cfg.AppState.Installed {
			names = append(names, k)
		}
	} else {
		for _, arg := range args[1:] {
			if _, ok := cfg.AppState.Installed[strings.ToLower(arg)]; ok {
				names = append(names, strings.ToLower(arg))
			} else {
				fmt.Fprintf(os.Stderr, "Error: [%s] %s\n", arg, &addon.NotFound{})
				continue
			}
		}
	}

	for _, name := range names {
		installed := cfg.AppState.Installed[name]
		latest := cfg.AppState.Latest[name]

		if installed.Version == latest.Version {
			continue
		}

		fmt.Printf("==> [%s] upgrading...\n", latest.Name)
		fmt.Printf("==> [%s] downloading %s\n", latest.Name, latest.Url)
		if err := latest.Fetch(); err != nil {
			fmt.Fprintf(os.Stderr, "Error: [%s] %s\n", latest.Name, err)
			continue
		}
		defer latest.Clean()

		fmt.Printf("==> [%s] packaging...\n", latest.Name)
		if err := latest.Package(); err != nil {
			fmt.Fprintf(os.Stderr, "Error: [%s] %s\n", latest.Name, err)
			continue
		}

		fmt.Printf("==> [%s] removing old directories\n", latest.Name)
		if err := Remove(cfg, []string{"remove", name}); err != nil {
			fmt.Fprintf(os.Stderr, "Error: [%s] %s\n", latest.Name, err)
			continue
		}

		fmt.Printf("==> [%s] extracting files to %s\n", latest.Name, cfg.UserCfg.AddonPath)
		if err := latest.Unpack(cfg.UserCfg.AddonPath); err != nil {
			fmt.Fprintf(os.Stderr, "Error: [%s] %s\n", latest.Name, err)
			continue
		}

		for _, dir := range latest.Dirs {
			fmt.Printf("moving %s to %s\n", dir, cfg.UserCfg.AddonPath)
		}

		cfg.AppState.Installed[name] = latest
		cfg.AppState.Latest[name] = latest
	}

	return nil
}
