package command

import (
	"fmt"
	"os"
	"strings"
	"sync"

	"github.com/ccassise/wowpkg/internal/config"
	"github.com/ccassise/wowpkg/pkg/addon"
)

func Update(cfg *config.Config, args []string) error {
	var names []string
	if len(args) <= 1 {
		for _, installed := range cfg.AppState.Installed {
			names = append(names, installed.Name)
		}
	} else {
		for _, arg := range args[1:] {
			if installed, ok := cfg.AppState.Installed[strings.ToLower(arg)]; ok {
				names = append(names, installed.Name)
			} else {
				fmt.Printf("Error: [%s] %s\n", arg, &addon.NotFound{})
				continue
			}
		}
	}

	var wg sync.WaitGroup
	wg.Add(len(names))

	addons := make(chan *addon.Addon)
	for _, name := range names {
		go func(name string) {
			defer wg.Done()
			fmt.Printf("==> Updating %s\n", name)
			fmt.Printf("fetching %s metadata...\n", name)
			addon, err := addon.New(name)
			if err != nil {
				fmt.Fprintf(os.Stderr, "Error: [%s] %s\n", name, err)
				return
			}

			fmt.Printf("done %s\n", addon.Name)

			addons <- addon
		}(name)
	}

	go func() {
		wg.Wait()
		close(addons)
	}()

	for addon := range addons {
		cfg.AppState.Latest[strings.ToLower(addon.Name)] = addon
	}

	return nil
}
