package command

import (
	"fmt"
	"sort"

	"github.com/ccassise/wowpkg/internal/config"
)

func List(cfg *config.Config, args []string) error {
	// var tb terminal.Builder
	// for i := range make([]int, 256) {
	// 	tb.AddArg(terminal.FGColorID(uint8(i)))
	// 	tb.SetText(fmt.Sprintf("%d", i))
	// 	fmt.Printf("%s ", tb.String())
	// 	tb.Clear()
	// }

	names := make([]string, 0, len(cfg.AppState.Installed))
	for _, addon := range cfg.AppState.Installed {
		names = append(names, addon.Name)
	}

	sort.Strings(names)

	fmt.Println("==> Installed addons")
	for _, name := range names {
		fmt.Println(name)
	}

	return nil
}
