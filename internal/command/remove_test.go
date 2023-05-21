package command

import (
	"io"
	"io/fs"
	"os"
	"path/filepath"
	"testing"

	"github.com/ccassise/wowpkg/internal/config"
	"github.com/ccassise/wowpkg/pkg/addon"
)

var testPath = filepath.Join("..", "..", "test")

func TestRemove(t *testing.T) {
	outPath, err := os.MkdirTemp(testPath, "mock_addon_dir")
	if err != nil {
		t.Fatal(err)
	}
	defer os.RemoveAll(outPath)

	err = copyTestRemoveDir(outPath)
	if err != nil {
		t.Fatal(err)
	}

	expectedFiles := []string{
		"ReallyCoolAddon",
		filepath.Join("ReallyCoolAddon", "ReallyCoolAddon.txt"),
	}

	cfg := config.Config{
		UserCfg: config.UserConfig{
			AddonPath: outPath,
		},
		AppState: config.State{
			Installed: map[string]*addon.Addon{
				"weakauras": {Dirs: []string{"WeakAuras", "WeakAurasArchive"}},
			},
			Latest: map[string]*addon.Addon{
				"weakauras": {Dirs: []string{"WeakAuras", "WeakAurasArchive"}},
			},
		},
	}

	err = Remove(&cfg, []string{"remove", "weakauras"})
	if err != nil {
		t.Fatal(err)
	}

	for _, expectedFile := range expectedFiles {
		filePath := filepath.Join(outPath, expectedFile)
		if _, err := os.Stat(filePath); err != nil {
			t.Fatalf("should find %s", filePath)
		}
	}

	var actualCount int
	err = filepath.WalkDir(outPath, func(path string, d fs.DirEntry, err error) error {
		if path == outPath {
			return nil
		}

		actualCount++

		return nil
	})
	if err != nil {
		t.Fatal(err)
	}

	if actualCount != len(expectedFiles) {
		t.Fatalf("should find %d files but found %d", len(expectedFiles), actualCount)
	}

	_, ok := cfg.AppState.Installed["weakauras"]
	if ok {
		t.Fatalf("should delete entry from install list")
	}

	_, ok = cfg.AppState.Latest["weakauras"]
	if ok {
		t.Fatalf("should delete entry from latest list")
	}
}

func TestRemoveOnlyRemovesAddonFoundInCatalog(t *testing.T) {
	outPath, err := os.MkdirTemp(testPath, "mock_addon_dir")
	if err != nil {
		t.Fatal(err)
	}
	defer os.RemoveAll(outPath)

	err = copyTestRemoveDir(outPath)
	if err != nil {
		t.Fatal(err)
	}

	expectedFiles := []string{
		"ReallyCoolAddon",
		filepath.Join("ReallyCoolAddon", "ReallyCoolAddon.txt"),
		"WeakAuras",
		filepath.Join("WeakAuras", "WeakAuras.txt"),
		filepath.Join("WeakAuras", "folder"),
		"WeakAurasArchive",
		filepath.Join("WeakAurasArchive", "WeakAurasArchive.txt"),
	}

	cfg := config.Config{
		UserCfg: config.UserConfig{
			AddonPath: outPath,
		},
		AppState: config.State{
			Installed: map[string]*addon.Addon{
				"weakauras": {Dirs: []string{"WeakAuras", "WeakAurasArchive"}},
			},
			Latest: map[string]*addon.Addon{
				"weakauras": {Dirs: []string{"WeakAuras", "WeakAurasArchive"}},
			},
		},
	}

	err = Remove(&cfg, []string{"remove", "Really"})
	if err != nil {
		t.Fatal(err)
	}

	for _, expectedFile := range expectedFiles {
		filePath := filepath.Join(outPath, expectedFile)
		if _, err := os.Stat(filePath); err != nil {
			t.Fatalf("should find %s", filePath)
		}
	}

	var actualCount int
	err = filepath.WalkDir(outPath, func(path string, d fs.DirEntry, err error) error {
		if path == outPath {
			return nil
		}

		actualCount++

		return nil
	})
	if err != nil {
		t.Fatal(err)
	}

	if actualCount != len(expectedFiles) {
		t.Fatalf("should find %d files but found %d", len(expectedFiles), actualCount)
	}

	_, ok := cfg.AppState.Installed["weakauras"]
	if !ok {
		t.Fatalf("should not delete entry from install list")
	}

	_, ok = cfg.AppState.Latest["weakauras"]
	if !ok {
		t.Fatalf("should not delete entry from latest list")
	}
}

func copyTestRemoveDir(outDir string) error {
	testRemoveDir := filepath.Join(testPath, "mock_addon_dir")

	filePaths := []string{
		"ReallyCoolAddon",
		filepath.Join("ReallyCoolAddon", "ReallyCoolAddon.txt"),
		"WeakAuras",
		filepath.Join("WeakAuras", "WeakAuras.txt"),
		filepath.Join("WeakAuras", "folder"),
		"WeakAurasArchive",
		filepath.Join("WeakAurasArchive", "WeakAurasArchive.txt"),
	}

	for _, filePath := range filePaths {
		f, err := os.Open(filepath.Join(testRemoveDir, filePath))
		if err != nil {
			return err
		}
		defer f.Close()

		fstat, err := f.Stat()
		if err != nil {
			return err
		}

		dest := filepath.Join(outDir, filePath)

		if fstat.IsDir() {
			os.MkdirAll(dest, 0755)
			continue
		}

		if err := os.MkdirAll(filepath.Dir(dest), 0755); err != nil {
			return err
		}

		destFile, err := os.Create(dest)
		if err != nil {
			return err
		}
		defer destFile.Close()

		_, err = io.Copy(destFile, f)
		if err != nil {
			return err
		}
	}

	return nil
}
