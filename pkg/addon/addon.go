package addon

import (
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"os"

	"github.com/ccassise/wowpkg/pkg/zipper"
)

type githubReleaseResponse struct {
	Name    string
	TagName string `json:"tag_name"`
	Assets  []githubAssetResponse
}

type githubAssetResponse struct {
	ContentType        string `json:"content_type"`
	BrowserDownloadURL string `json:"browser_download_url"`
}

// Includes GitHub repository information.
type AddonInfo struct {
	Handler string // Strategy to get and package addon.
	Name    string
	Desc    string
	Url     string
}

type Addon struct {
	CreatedAt string
	ZipFile   *os.File
	Info      *AddonInfo
}

// Creates a new package from package info. If a nil error is returned the
// returned package will contain the package's file in its properties.
// NOTE: The caller should call Clean() when done with package.

// TODO: This should change to install and do the following steps:
//  1. Download the addon's .zip
//  2. Unzip
//  3. Package the addon (if necessary)
//  4. Extract the addon to the addon dir
//
// These steps should all be thread safe.
func Download(addonInfo *AddonInfo) (*Addon, error) {
	var result Addon
	result.Info = addonInfo

	resp, err := http.Get(addonInfo.Url)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()

	var latestRelease githubReleaseResponse
	if err = json.NewDecoder(resp.Body).Decode(&latestRelease); err != nil {
		return nil, err
	}

	var releaseURL string
	for _, asset := range latestRelease.Assets {
		if asset.ContentType == "application/zip" {
			releaseURL = asset.BrowserDownloadURL
			break
		}
	}

	if releaseURL == "" {
		return nil, fmt.Errorf("no .zip found in package's latest release assets")
	}

	zipResp, err := http.Get(releaseURL)
	if err != nil {
		return nil, err
	}
	defer zipResp.Body.Close()

	zipFile, err := os.CreateTemp("", addonInfo.Name+"_"+latestRelease.Name+"_*.zip")
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
func (pkg *Addon) Package(dest string) error {
	if pkg.ZipFile == nil {
		return fmt.Errorf("no .zip file")
	}

	if err := zipper.Unzip(dest, pkg.ZipFile.Name()); err != nil {
		return err
	}

	return nil
}

// Cleans up any temp folders/files created.
func (pkg *Addon) Clean() {
	if pkg.ZipFile != nil {
		pkg.ZipFile.Close()
		os.Remove(pkg.ZipFile.Name())
		pkg.ZipFile = nil
	}
	// 	if pkg.zipPath == "" {
	// 		return nil
	// 	}

	// 	tmp := pkg.zipPath
	// 	pkg.zipPath = ""
	// 	return os.Remove(tmp)
}
