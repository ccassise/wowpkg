package command

import (
	"fmt"
	"sort"

	"github.com/ccassise/wowpkg/internal/config"
	"github.com/ccassise/wowpkg/pkg/addon"
)

func List(cfg *config.Config, args []string) error {
	// var tb terminal.Builder
	// for i := range make([]int, 256) {
	// 	tb.AddArg(terminal.FGColorID(uint8(i)))
	// 	tb.SetText(fmt.Sprintf("%d", i))
	// 	fmt.Printf("%s ", tb.String())
	// 	tb.Clear()
	// }

	addons := make([]*addon.Addon, 0, len(cfg.AppState.Installed))
	for _, a := range cfg.AppState.Installed {
		addons = append(addons, a)
	}

	sort.Slice(addons[:], func(i, j int) bool {
		return addons[i].Name < addons[j].Name
	})

	for _, a := range addons {
		fmt.Printf("%s (%s)\n", a.Name, a.Version)
	}

	return nil
}
