package addon

import (
	"bufio"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"time"

	"github.com/ccassise/wowpkg/pkg/zipper"
)

type Addon struct {
	Name    string   `json:"name"`
	Desc    string   `json:"desc"`
	Version string   `json:"version"`
	Url     string   `json:"url"`
	Handler string   `json:"handler"` // Strategy to get and package addon.
	Dirs    []string `json:"dirs"`

	zipPath     string
	packagePath string
}

type NotFound struct{}

func (e *NotFound) Error() string {
	return "unable to find addon"
}

type DownloadFailed struct {
	StatusCode int
}

func (e *DownloadFailed) Error() string {
	return fmt.Sprintf("download failed for addon with status code %d", e.StatusCode)
}

type NoZip struct{}

func (e *NoZip) Error() string {
	return "no .zip for addon"
}

type GHRateLimit struct {
	ResetAtUnixSec int64
}

func (e *GHRateLimit) Error() string {
	const ghRateErrMsg = "github rate limit reached"

	d := time.Until(time.Unix(e.ResetAtUnixSec, 0))
	if d.Minutes() < 0 {
		return ghRateErrMsg
	}
	return fmt.Sprintf("%s - try again in %.0f minutes", ghRateErrMsg, d.Minutes())
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
// NOTE: This method makes a HTTP request.
func New(name string) (*Addon, error) {
	item, err := newItem(name)
	if err != nil {
		return nil, &NotFound{}
	}

	addon := Addon{
		Name:    item.Name,
		Desc:    item.Desc,
		Handler: item.Handler,
	}

	client := http.DefaultClient
	req, err := http.NewRequest(http.MethodGet, item.Url, nil)
	if err != nil {
		return nil, err
	}

	req.Header.Set("Accept", "application/vnd.github+json")
	req.Header.Set("X-GitHub-Api-Version", "2022-11-28")

	resp, err := client.Do(req)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()

	remainingHdr := resp.Header.Get("x-ratelimit-remaining")
	if resp.StatusCode == http.StatusForbidden && remainingHdr == "0" {
		resetHdr := resp.Header.Get("x-ratelimit-reset")
		resetAt, err := strconv.ParseInt(resetHdr, 10, 64)
		if err != nil {
			return nil, &GHRateLimit{0}
		}

		return nil, &GHRateLimit{resetAt}
	}

	if resp.StatusCode != http.StatusOK {
		return nil, &DownloadFailed{StatusCode: resp.StatusCode}
	}

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
		return nil, &NoZip{}
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
		return &NoZip{}
	}

	var err error
	addon.packagePath, err = os.MkdirTemp("", addon.Name+"_"+addon.Version+"_")
	if err != nil {
		return err
	}

	if err := zipper.Unzip(addon.packagePath, addon.zipPath); err != nil {
		return err
	}

	dirs, err := os.ReadDir(addon.packagePath)
	if err != nil {
		return err
	}

	if len(addon.Dirs) > 0 {
		addon.Dirs = []string{}
	}

	for _, dir := range dirs {
		if dir.IsDir() {
			addon.Dirs = append(addon.Dirs, dir.Name())
		}
	}

	return nil
}

// Extracts a packaged addon into the dest directory.
//
// NOTE: This function is not thread safe.
func (addon *Addon) Unpack(dest string) error {
	for _, dir := range addon.Dirs {
		newPath := filepath.Join(dest, dir)
		if _, err := os.Stat(newPath); err == nil {
			if err = os.RemoveAll(newPath); err != nil {
				return err
			}
		}

		if err := os.Rename(filepath.Join(addon.packagePath, dir), newPath); err != nil {
			return err
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

	catalog, err := os.ReadDir(CatalogPath())
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

	f, err := os.Open(filepath.Join(CatalogPath(), caseSensitiveName+catalogSuffix))
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

func CatalogPath() string {
	// ep, _ := os.Executable()
	// return filepath.Join(ep, "..", "..", "catalog")

	return "catalog"
}
