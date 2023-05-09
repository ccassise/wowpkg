package command

import (
	"os"
	"strings"
)

func Uninstall(cfg Config, args []string) error {
	pkgs, err := os.ReadDir(cfg.CatalogDir)
	if err != nil {
		return err
	}

	caseSensitivePkgs := []string{}
	for _, pkg := range pkgs {
		if !pkg.IsDir() {
			caseSensitivePkgs = append(caseSensitivePkgs, strings.TrimSuffix(pkg.Name(), catalogSuffix))
		}
	}

	caseSensitiveArgs := []string{}
	for _, arg := range args {
		caseSensitiveArg := caseSensitiveStr(caseSensitivePkgs, arg)
		caseSensitiveArgs = append(caseSensitiveArgs, caseSensitiveArg+catalogSuffix)
	}

	// for _, caseSensitiveArg := range caseSensitiveArgs {
	// 	err = os.RemoveAll(filepath.Join(cfg.AddonDir, caseSensitiveArg))
	// 	if err != nil {
	// 		return err
	// 	}
	// }

	return nil
}
