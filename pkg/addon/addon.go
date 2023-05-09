package addon

import (
	"context"
	"fmt"
	"io"
	"net/http"
	"os"

	"github.com/ccassise/wowpkg/pkg/unzip"
	"github.com/google/go-github/github"
)

// Includes GitHub repository information.
type GHPkgInfo struct {
	Handler string // Strategy to get and package addon.
	Owner   string
	Repo    string
}

type Pkg struct {
	CreatedAt string
	ZipFile   *os.File
}

// Creates a new package from package info. If a nil error is returned the
// returned package will contain the package's file in its properties.
// NOTE: The caller should call Clean() when done with package.
func New(pkgInfo *GHPkgInfo) (*Pkg, error) {
	var result Pkg
	ctx := context.Background()
	client := github.NewClient(nil)

	rel, _, err := client.Repositories.GetLatestRelease(ctx, pkgInfo.Owner, pkgInfo.Repo)
	if err != nil {
		return nil, err
	}

	var releaseURL string
	for _, asset := range rel.Assets {
		if *asset.ContentType == "application/zip" {
			releaseURL = *asset.BrowserDownloadURL
		}
	}

	if releaseURL == "" {
		return nil, fmt.Errorf("no release .zip found")
	}

	zipResp, err := http.Get(releaseURL)
	if err != nil {
		return nil, err
	}
	defer zipResp.Body.Close()

	zipFile, err := os.CreateTemp("", pkgInfo.Repo+"*.zip")
	if err != nil {
		return nil, err
	}
	result.ZipFile = zipFile

	_, err = io.Copy(zipFile, zipResp.Body)
	if err != nil {
		return nil, err
	}

	return &result, nil
}

// Prepares the package so that it may be placed into the WoW addons folder. All
// produced files will be placed in dest directory.
func (pkg *Pkg) Package(dest string) error {
	if pkg.ZipFile == nil {
		return fmt.Errorf("no file to package")
	}

	if err := unzip.Inflate(dest, pkg.ZipFile.Name()); err != nil {
		return err
	}

	return nil
}

// Cleans up any temp folders/files created.
func (pkg *Pkg) Clean() {
	if pkg.ZipFile != nil {
		pkg.ZipFile.Close()
		os.Remove(pkg.ZipFile.Name())
	}
	// 	if pkg.zipPath == "" {
	// 		return nil
	// 	}

	// 	tmp := pkg.zipPath
	// 	pkg.zipPath = ""
	// 	return os.Remove(tmp)
}
