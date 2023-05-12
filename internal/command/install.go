package command

import (
	"bufio"
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"strings"

	"github.com/ccassise/wowpkg/internal/config"
	"github.com/ccassise/wowpkg/pkg/addon"
)

func Install(cfg *config.Config, args []string) error {
	// caseSensitiveArgs, err := mapToCaseSensitiveNames(cfg, args)
	// if err != nil {
	// 	return err
	// }

	// pkgs := []*addon.Pkg{}
	// for _, caseSensitiveArg := range caseSensitiveArgs {
	// 	pkg, err := downloadPackage(cfg, caseSensitiveArg)
	// 	if err != nil {
	// 		fmt.Fprintf(os.Stderr, "%s: %s\n", caseSensitiveArg, err)
	// 	} else {
	// 		pkgs = append(pkgs, pkg)
	// 	}
	// }

	type item struct {
		addon *addon.Addon
		err   error
	}

	catalog, err := os.ReadDir(cfg.UserCfg.CatalogDir)
	if err != nil {
		return err
	}

	fmt.Println(args[1:], len(args[1:]))
	ch := make(chan item, len(args[1:]))
	for _, arg := range args[1:] {
		catalogItem := arg
		for _, item := range catalog {
			addonName := strings.TrimSuffix(item.Name(), catalogSuffix)
			if strings.EqualFold(addonName, arg) {
				catalogItem = addonName
			}
		}

		go func(arg string) {
			var it item

			addonInfo, err := getAddonInfo(filepath.Join(cfg.UserCfg.CatalogDir, catalogItem+catalogSuffix))
			if err != nil {
				// fmt.Fprintf(os.Stderr, "%s: %s\n", arg, notFound)
				// continue
				it.err = fmt.Errorf("%s: %s", arg, notFound)
				ch <- it
				return
			}

			it.addon, err = addon.Download(addonInfo)
			if err != nil {
				it.err = fmt.Errorf("%s: %s", arg, err)
			}
			ch <- it
		}(arg)
	}

	addons := []*addon.Addon{}
	for range args[1:] {
		it := <-ch
		if it.err != nil {
			fmt.Fprintf(os.Stderr, "%s\n", it.err)
			continue
		}
		addons = append(addons, it.addon)
	}

OUTER:
	for _, addon := range addons {
		tmpDir, err := os.MkdirTemp("", "wowpkg_"+addon.Info.Name+"_")
		if err != nil {
			fmt.Fprintf(os.Stderr, "%s: %s\n", addon.Info.Name, err)
			continue
		}
		defer os.RemoveAll(tmpDir)

		if err = addon.Package(tmpDir); err != nil {
			// return err
			fmt.Fprintf(os.Stderr, "%s: %s\n", addon.Info.Name, err)
			continue
		}
		defer addon.Clean()

		addonFiles, err := os.ReadDir(tmpDir)
		if err != nil {
			fmt.Fprintf(os.Stderr, "%s: %s\n", addon.Info.Name, err)
			continue
		}

		folders, ok := cfg.AppCfg.Installed[addon.Info.Name]
		if ok {
			for _, folder := range folders {
				if err = os.RemoveAll(filepath.Join(cfg.UserCfg.AddonDir, folder)); err != nil {
					fmt.Fprintf(os.Stderr, "%s: %s\n", addon.Info.Name, err)
					continue OUTER
				}
			}
		}
		delete(cfg.AppCfg.Installed, addon.Info.Name)

		for _, addonFile := range addonFiles {
			fmt.Println(addonFile.Name())
			if addonFile.IsDir() {
				newPath := filepath.Join(cfg.UserCfg.AddonDir, addonFile.Name())
				_, err := os.Stat(newPath)
				if err == nil {
					// TODO: Prompt user warning that folder will be removed and replaced?
					// TODO: Instead of warning for each folder build a list and then prompt and remove all at once.
					fmt.Fprintf(os.Stderr, "WARNING: replacing %s\n", newPath)

					if err = os.RemoveAll(newPath); err != nil {
						fmt.Fprintf(os.Stderr, "%s: %s\n", addon.Info.Name, err)
						continue OUTER
					}
				}

				if err = os.Rename(filepath.Join(tmpDir, addonFile.Name()), newPath); err != nil {
					fmt.Fprintf(os.Stderr, "%s: %s\n", addon.Info.Name, err)
					continue OUTER
				}
				cfg.AppCfg.Installed[addon.Info.Name] = append(cfg.AppCfg.Installed[addon.Info.Name], addonFile.Name())
			}
		}
	}

	return nil

	// var renameMu sync.Mutex
	// var wg sync.WaitGroup
	// wg.Add(len(caseSensitiveArgs))

	//# Build list of case sensitive addon names.
	// For each addon name build a list of addon info from catalog.
	// @
	// For each addon download its packaged .zip file.
	// @
	// For each packaged .zip file extract it to a temp directory.
	// Move all the extracted files into the WoW addon directory.
	// For each addon clean up the temp directory.
	// for _, caseSensitiveArg := range caseSensitiveArgs {
	// 	go func(caseSensitiveArg string) {
	// 		defer wg.Done()

	// 		f, err := os.Open(filepath.Join(cfg.UserCfg.CatalogDir, caseSensitiveArg+catalogSuffix))
	// 		if err != nil {
	// 			fmt.Fprintf(os.Stderr, "%s: %s\n", caseSensitiveArg, notFound)
	// 			return
	// 		}
	// 		defer f.Close()

	// 		var pkgInfo addon.AddonInfo
	// 		bufReader := bufio.NewReader(f)
	// 		if err := json.NewDecoder(bufReader).Decode(&pkgInfo); err != nil {
	// 			// return err
	// 			fmt.Fprintf(os.Stderr, "%s: %s\n", caseSensitiveArg, err)
	// 			return
	// 		}

	// 		pkg, err := addon.Download(&pkgInfo)
	// 		if err != nil {
	// 			// return err
	// 			fmt.Fprintf(os.Stderr, "%s: %s\n", caseSensitiveArg, err)
	// 			return
	// 		}
	// 		defer pkg.Clean()

	// 		tmpDir, err := os.MkdirTemp("dump", "wowpkg_")
	// 		if err != nil {
	// 			fmt.Fprintf(os.Stderr, "%s: %s\n", caseSensitiveArg, err)
	// 			return
	// 		}
	// 		defer os.RemoveAll(tmpDir)

	// 		if err = pkg.Package(tmpDir); err != nil {
	// 			// return err
	// 			fmt.Fprintf(os.Stderr, "%s: %s\n", caseSensitiveArg, err)
	// 			return
	// 		}

	// 		pkgFiles, err := os.ReadDir(tmpDir)
	// 		if err != nil {
	// 			fmt.Fprintf(os.Stderr, "%s: %s\n", caseSensitiveArg, err)
	// 			return
	// 		}

	// 		folders, ok := cfg.AppCfg.Installed[caseSensitiveArg]
	// 		if ok {
	// 			for _, folder := range folders {
	// 				if err = os.RemoveAll(filepath.Join(cfg.UserCfg.AddonDir, folder)); err != nil {
	// 					fmt.Fprintf(os.Stderr, "%s: %s\n", caseSensitiveArg, err)
	// 					return
	// 				}
	// 			}
	// 		}
	// 		delete(cfg.AppCfg.Installed, caseSensitiveArg)

	// 		renameMu.Lock()
	// 		defer renameMu.Unlock()
	// 		for _, pkgFile := range pkgFiles {
	// 			if pkgFile.IsDir() {
	// 				newPath := filepath.Join(cfg.UserCfg.AddonDir, pkgFile.Name())
	// 				_, err := os.Stat(newPath)
	// 				if err == nil {
	// 					// TODO: Prompt user warning that folder will be removed and replaced?
	// 					fmt.Fprintf(os.Stderr, "WARNING: replacing %s\n", newPath)

	// 					if err = os.RemoveAll(newPath); err != nil {
	// 						fmt.Fprintf(os.Stderr, "%s: %s\n", caseSensitiveArg, err)
	// 						return

	// 					}
	// 				}

	// 				if err = os.Rename(filepath.Join(tmpDir, pkgFile.Name()), newPath); err != nil {
	// 					fmt.Fprintf(os.Stderr, "%s: %s\n", caseSensitiveArg, err)
	// 					return
	// 				}
	// 				cfg.AppCfg.Installed[caseSensitiveArg] = append(cfg.AppCfg.Installed[caseSensitiveArg], pkgFile.Name())
	// 			}
	// 		}

	// 	}(caseSensitiveArg)
	// }

	// wg.Wait()
	// fmt.Println(caseSensitiveArgs)
	// fmt.Println(cfg.AppCfg.Installed)

	// return nil
}

func getAddonInfo(path string) (*addon.AddonInfo, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	var pkgInfo addon.AddonInfo
	bufReader := bufio.NewReader(f)
	if err := json.NewDecoder(bufReader).Decode(&pkgInfo); err != nil {
		return nil, err
	}

	return &pkgInfo, nil
}

// Converts a list of addon names to the case sensitive name found from the
// addon catalog.
// func mapToCaseSensitiveNames(cfg *config.Config, args []string) ([]string, error) {
// 	pkgs, err := os.ReadDir(cfg.UserCfg.CatalogDir)
// 	if err != nil {
// 		return nil, err
// 	}

// 	caseSensitivePkgs := []string{}
// 	for _, pkg := range pkgs {
// 		if !pkg.IsDir() {
// 			caseSensitivePkgs = append(caseSensitivePkgs, strings.TrimSuffix(pkg.Name(), catalogSuffix))
// 		}
// 	}

// 	caseSensitiveArgs := []string{}
// 	for _, arg := range args[1:] {
// 		caseSensitiveArg := caseSensitiveStr(caseSensitivePkgs, arg)
// 		caseSensitiveArgs = append(caseSensitiveArgs, caseSensitiveArg)
// 	}

// 	return caseSensitiveArgs, nil
// }

// func downloadPackage(cfg *config.Config, addonName string) (*addon.Addon, error) {
// 	f, err := os.Open(filepath.Join(cfg.UserCfg.CatalogDir, addonName+catalogSuffix))
// 	if err != nil {
// 		return nil, err
// 		// fmt.Fprintf(os.Stderr, "%s: %s\n", caseSensitiveArg, notFound)
// 		// return
// 	}
// 	defer f.Close()

// 	var pkgInfo addon.AddonInfo
// 	bufReader := bufio.NewReader(f)
// 	if err := json.NewDecoder(bufReader).Decode(&pkgInfo); err != nil {
// 		return nil, err
// 		// fmt.Fprintf(os.Stderr, "%s: %s\n", caseSensitiveArg, err)
// 		// return
// 	}

// 	pkg, err := addon.Download(&pkgInfo)
// 	if err != nil {
// 		return nil, err
// 		// fmt.Fprintf(os.Stderr, "%s: %s\n", caseSensitiveArg, err)
// 		// return
// 	}
// 	// defer pkg.Clean()
// 	return pkg, nil
// }
