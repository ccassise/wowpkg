package command

import (
	"fmt"

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

	fmt.Println("==> Installed addons")
	for _, addon := range cfg.AppState.Installed {
		fmt.Println(addon.Name)
	}

	return nil
}
