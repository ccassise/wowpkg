package command

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"
)

func Install(cfg Config, args []string) error {
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
	for _, arg := range args[1:] {
		caseSensitiveArg := caseSensitiveStr(caseSensitivePkgs, arg)
		caseSensitiveArgs = append(caseSensitiveArgs, caseSensitiveArg+catalogSuffix)
	}

	for _, caseSensitiveArg := range caseSensitiveArgs {
		_, err := os.Open(filepath.Join(cfg.CatalogDir, caseSensitiveArg))
		if err != nil {
			fmt.Fprintf(os.Stderr, "unable to find addon '%s'\n", strings.TrimSuffix(caseSensitiveArg, catalogSuffix))
			continue
		}

		// var pkgInfo addon.GHPkgInfo
		// bufReader := bufio.NewReader(f)
		// if err := json.NewDecoder(bufReader).Decode(&pkgInfo); err != nil {
		// 	return err
		// }

		// // zipDir := filepath.Join("dump", "zips")
		// pkg, err := addon.New(&pkgInfo)
		// if err != nil {
		// 	return err
		// }
		// defer pkg.Clean()

		// // outDir := "C:\\Program Files (x86)\\Battle.net\\World of Warcraft\\_retail_\\Interface\\AddOns"
		// outDir := filepath.Join("dump", "out")

		// if err = pkg.Package(cfg.AddonDir); err != nil {
		// 	return err
		// }
	}

	fmt.Println(caseSensitiveArgs)

	return nil
}
