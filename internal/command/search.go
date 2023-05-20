package command

import (
	"fmt"
	"os"
	"strings"

	"github.com/ccassise/wowpkg/internal/config"
	"github.com/ccassise/wowpkg/pkg/addon"
)

func Search(cfg *config.Config, args []string) error {
	const catalogSuffix = ".json"

	if len(args) <= 1 {
		return &InvalidArgs{}
	}

	catalog, err := os.ReadDir(addon.CatalogPath())
	if err != nil {
		return err
	}

	fmt.Println("==> Addons")
	arg := args[1]
	var found []string
	for _, item := range catalog {
		if item.IsDir() {
			continue
		}

		it := strings.TrimSuffix(item.Name(), catalogSuffix)
		if strings.Contains(strings.ToLower(it), strings.ToLower(arg)) {
			found = append(found, it)
		}
	}

	for _, item := range found {
		fmt.Printf("%s\n", item)
	}

	return nil
}
