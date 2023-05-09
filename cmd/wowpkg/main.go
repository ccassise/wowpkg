package main

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"

	"github.com/ccassise/wowpkg/internal/command"
	// "github.com/ccassise/wowpkg/pkg/addon"
	// "github.com/ccassise/wowpkg/internal/command"
)

func main() {
	if len(os.Args) <= 1 {
		fmt.Fprintf(os.Stderr, "this should be the help/usage message\n")
		os.Exit(1)
	}

	cfg := command.Config{AddonDir: filepath.Join("dump", "out")}

	arg := os.Args[1]
	switch strings.ToLower(arg) {
	case "install":
		fmt.Println(os.Args[1:])
		command.Install(cfg, os.Args[1:])
	default:
		fmt.Fprintf(os.Stderr, "unrecognized command\n")
		os.Exit(1)
	}

	// f, err := os.Open(filepath.Join("catalog", "bigwigs.json"))
	// if err != nil {
	// 	fmt.Fprintln(os.Stderr, err)
	// 	os.Exit(1)
	// }

	// var pkgInfo addon.GHPkgInfo
	// bufReader := bufio.NewReader(f)
	// if err := json.NewDecoder(bufReader).Decode(&pkgInfo); err != nil {
	// 	fmt.Fprintln(os.Stderr, err)
	// 	os.Exit(1)
	// }

	// // fmt.Println(pkgInfo.Owner, pkgInfo.Repo, pkgInfo.Handler)

	// // zipDir := filepath.Join("dump", "zips")
	// pkg, err := addon.New(&pkgInfo)
	// if err != nil {
	// 	fmt.Println(err)
	// 	os.Exit(1)
	// }
	// defer pkg.Clean()
	// // defer pkgInfo.Clean()

	// // addonDir := "C:\\Program Files (x86)\\Battle.net\\World of Warcraft\\_retail_\\Interface\\AddOns"
	// addonDir := filepath.Join("dump", "out")

	// // if err = pkgInfo.Process(filepath.Join("dump", "out")); err != nil {
	// if err = pkg.Package(addonDir); err != nil {
	// 	fmt.Println(err)
	// 	os.Exit(1)
	// }
}
