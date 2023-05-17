package command

import (
	"fmt"
	"os"
	"strings"

	"github.com/ccassise/wowpkg/internal/config"
)

func Search(cfg *config.Config, args []string) error {
	const catalogSuffix = ".json"

	if len(args) <= 1 {
		return &InvalidArgs{}
	}

	catalog, err := os.ReadDir("catalog")
	if err != nil {
		return err
	}

	arg := args[1]
	var found []string
	for _, item := range catalog {
		it := strings.TrimSuffix(item.Name(), catalogSuffix)
		if strings.Contains(strings.ToLower(it), strings.ToLower(arg)) {
			found = append(found, it)
		}
	}

	for _, item := range found {
		fmt.Println("\t" + item)
	}

	return nil
}
