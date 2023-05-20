package command

import (
	"fmt"
	"os"
	"strings"
	"sync"

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

	var wg sync.WaitGroup
	wg.Add(len(names))

	addons := make(chan *addon.Addon)
	for _, name := range names {
		installed := cfg.AppState.Installed[name]
		latest := cfg.AppState.Latest[name]

		if installed.Version == latest.Version {
			wg.Done()
			continue
		}

		// This avoids need for a mutex? Would a mutex be slower or faster than
		// the copy? This is only even a concern if the user enters the same
		// addon two or more times.
		a := *latest

		go func(a *addon.Addon) {
			defer wg.Done()
			fmt.Printf("==> Upgrading %s\n", a.Name)
			fmt.Printf("==> [%s] downloading %s\n", a.Name, a.Url)
			if err := a.Fetch(); err != nil {
				fmt.Fprintf(os.Stderr, "Error: [%s] %s\n", a.Name, err)
				return
			}

			addons <- a
		}(&a)
	}

	go func() {
		wg.Wait()
		close(addons)
	}()

	for addon := range addons {
		func() {
			defer func() {
				fmt.Printf("==> [%s] cleaning up any temp files\n", addon.Name)
				addon.Clean()
			}()

			fmt.Printf("==> [%s] packaging...\n", addon.Name)
			if err := addon.Package(); err != nil {
				fmt.Fprintf(os.Stderr, "Error: [%s] %s\n", addon.Name, err)
				return
			}

			fmt.Printf("==> [%s] removing old directories\n", addon.Name)
			if err := Remove(cfg, []string{"remove", addon.Name}); err != nil {
				fmt.Fprintf(os.Stderr, "Error: [%s] %s\n", addon.Name, err)
				return
			}

			fmt.Printf("==> [%s] extracting files to %s\n", addon.Name, cfg.UserCfg.AddonPath)
			if err := addon.Unpack(cfg.UserCfg.AddonPath); err != nil {
				fmt.Fprintf(os.Stderr, "Error: [%s] %s\n", addon.Name, err)
				return
			}

			for _, dir := range addon.Dirs {
				fmt.Printf("moving %s to %s\n", dir, cfg.UserCfg.AddonPath)
			}

			cfg.AppState.Installed[strings.ToLower(addon.Name)] = addon
			cfg.AppState.Latest[strings.ToLower(addon.Name)] = addon
		}()
	}

	return nil
}
