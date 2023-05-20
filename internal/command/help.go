package command

import (
	"fmt"

	"github.com/ccassise/wowpkg/internal/config"
)

const helpUsage = `Example usage:
  wowpkg install ADDON...
  wowpkg list
  wowpkg outdated
  wowpkg remove ADDON...
  wowpkg search TEXT
  wowpkg update [ADDON...]
  wowpkg upgrade [ADDON...]
`

func Help(cfg *config.Config, args []string) error {
	fmt.Printf("%s", helpUsage)

	return nil
}
