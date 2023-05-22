package command

import (
	"fmt"
	"sort"
	"strings"

	"github.com/ccassise/wowpkg/internal/config"
)

func Outdated(cfg *config.Config, args []string) error {
	names := make([]string, 0, len(cfg.AppState.Installed))
	for key, addon := range cfg.AppState.Installed {
		installed := cfg.AppState.Installed[key]
		latest := cfg.AppState.Latest[key]
		if latest.Version != installed.Version {
			names = append(names, addon.Name)
		}
	}

	sort.Strings(names)

	for _, name := range names {
		key := strings.ToLower(name)
		installed := cfg.AppState.Installed[key]
		latest := cfg.AppState.Latest[key]

		fmt.Printf("%s (%s) < %s\n", installed.Name, installed.Version, latest.Version)
	}

	return nil
}
