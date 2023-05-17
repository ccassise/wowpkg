package addon

import (
	"bufio"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"strings"

	"github.com/ccassise/wowpkg/pkg/zipper"
)

type Addon struct {
	Name    string
	Desc    string
	Version string
	Url     string
	Handler string // Strategy to get and package addon.
	Folders []string

	zipPath     string
	packagePath string
}

type NotFound struct {
	Name string
}

func (nf *NotFound) Error() string {
	return nf.Name + ": unable to find addon"
}

type catalogItem struct {
	Handler string // Strategy to get and package addon.
	Name    string
	Desc    string
	Url     string
}

type githubReleaseResponse struct {
	Name    string
	TagName string `json:"tag_name"`
	Assets  []githubAssetResponse
}

type githubAssetResponse struct {
	ContentType        string `json:"content_type"`
	BrowserDownloadURL string `json:"browser_download_url"`
}

// Creates a new Addon from the given addon name. The name is case-insenstive
// but should match exactly the name of the addon in the catalog.
//
// NOTE: This function makes a HTTP request.
func New(name string) (*Addon, error) {
	item, err := newItem(name)
	if err != nil {
		return nil, &NotFound{name}
	}

	addon := Addon{
		Name:    item.Name,
		Desc:    item.Desc,
		Handler: item.Handler,
	}

	resp, err := http.Get(item.Url)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()

	var latestRelease githubReleaseResponse
	if err = json.NewDecoder(resp.Body).Decode(&latestRelease); err != nil {
		return nil, err
	}

	addon.Version = latestRelease.TagName

	for _, asset := range latestRelease.Assets {
		if asset.ContentType == "application/zip" {
			addon.Url = asset.BrowserDownloadURL
			break
		}
	}

	if addon.Url == "" {
		return nil, fmt.Errorf(addon.Name + ": no .zip found in package's latest release assets")
	}

	return &addon, nil
}

// Fetch's the addon's .zip.
//
// NOTE: This method may make a HTTP request.
func (addon *Addon) Fetch() error {
	zipResp, err := http.Get(addon.Url)
	if err != nil {
		return err
	}
	defer zipResp.Body.Close()

	zipFile, err := os.CreateTemp("", addon.Name+"_"+addon.Version+"_*.zip")
	if err != nil {
		return err
	}
	defer zipFile.Close()

	_, err = io.Copy(zipFile, zipResp.Body)
	if err != nil {
		return err
	}

	addon.zipPath = zipFile.Name()

	return nil
}

// Packages the addon based on the strategy found in Addon.Handler.
func (addon *Addon) Package() error {
	if addon.zipPath == "" {
		return fmt.Errorf(addon.Name + ": no .zip file")
	}

	var err error
	addon.packagePath, err = os.MkdirTemp("", addon.Name+"_"+addon.Version+"_")
	if err != nil {
		return err
	}

	if err := zipper.Unzip(addon.packagePath, addon.zipPath); err != nil {
		return err
	}

	return nil
}

// Extracts a packaged addon into the dest directory.
//
// NOTE: This function is not thread safe.
func (addon *Addon) Extract(dest string) error {
	folders, err := os.ReadDir(addon.packagePath)
	if err != nil {
		return err
	}

	if len(addon.Folders) > 0 {
		addon.Folders = []string{}
	}

	for _, folder := range folders {
		if folder.IsDir() {
			newPath := filepath.Join(dest, folder.Name())
			_, err := os.Stat(newPath)
			if err == nil {
				// TODO: Prompt user warning that folder will be removed and replaced?
				// TODO: Instead of warning for each folder build a list and then prompt and remove all at once?
				fmt.Fprintf(os.Stderr, "WARNING: replacing %s\n", newPath)

				if err = os.RemoveAll(newPath); err != nil {
					fmt.Fprintf(os.Stderr, "%s: %s\n", addon.Name, err)
					continue
				}
			}

			if err = os.Rename(filepath.Join(addon.packagePath, folder.Name()), newPath); err != nil {
				fmt.Fprintf(os.Stderr, "%s: %s\n", addon.Name, err)
				continue
			}

			addon.Folders = append(addon.Folders, folder.Name())
		}
	}

	return nil
}

func (addon *Addon) Clean() {
	if addon.zipPath != "" {
		os.Remove(addon.zipPath)
	}

	if addon.packagePath != "" {
		os.RemoveAll(addon.packagePath)
	}
}

func newItem(name string) (*catalogItem, error) {
	const catalogSuffix = ".json"

	catalog, err := os.ReadDir("catalog")
	if err != nil {
		return nil, err
	}

	var caseSensitiveName string
	for _, item := range catalog {
		itemName := strings.TrimSuffix(item.Name(), catalogSuffix)
		if strings.EqualFold(itemName, name) {
			caseSensitiveName = itemName
			break
		}
	}

	f, err := os.Open(filepath.Join("catalog", caseSensitiveName+catalogSuffix))
	if err != nil {
		return nil, err
	}
	defer f.Close()

	var item catalogItem
	bufReader := bufio.NewReader(f)
	if err := json.NewDecoder(bufReader).Decode(&item); err != nil {
		return nil, err
	}

	return &item, nil
}
