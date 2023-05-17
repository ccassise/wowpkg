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

var testDir = filepath.Join("..", "..", "test")

func TestUninstall(t *testing.T) {
	outDir, err := os.MkdirTemp(testDir, "mock_addon_dir")
	if err != nil {
		t.Fatal(err)
	}
	defer os.RemoveAll(outDir)

	err = copyTestUninstallDir(outDir)
	if err != nil {
		t.Fatal(err)
	}

	expectedFiles := []string{
		"ReallyCoolAddon",
		filepath.Join("ReallyCoolAddon", "ReallyCoolAddon.txt"),
	}

	cfg := config.Config{
		UserCfg: config.UserConfig{
			AddonDir: outDir,
		},
		AppCfg: config.AppConfig{
			Installed: map[string]*addon.Addon{
				"weakauras": {Folders: []string{"WeakAuras", "WeakAurasArchive"}},
			},
			Latest: map[string]*addon.Addon{
				"weakauras": {Folders: []string{"WeakAuras", "WeakAurasArchive"}},
			},
		},
	}

	err = Uninstall(&cfg, []string{"uninstall", "weakauras"})
	if err != nil {
		t.Fatal(err)
	}

	for _, expectedFile := range expectedFiles {
		filePath := filepath.Join(outDir, expectedFile)
		if _, err := os.Stat(filePath); err != nil {
			t.Fatalf("should find %s", filePath)
		}
	}

	var actualCount int
	err = filepath.WalkDir(outDir, func(path string, d fs.DirEntry, err error) error {
		if path == outDir {
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

	_, ok := cfg.AppCfg.Installed["weakauras"]
	if ok {
		t.Fatalf("should delete entry from install list")
	}

	_, ok = cfg.AppCfg.Latest["weakauras"]
	if ok {
		t.Fatalf("should delete entry from latest list")
	}
}

func TestUninstallOnlyRemovesAddonFoundInCatalog(t *testing.T) {
	outDir, err := os.MkdirTemp(testDir, "mock_addon_dir")
	if err != nil {
		t.Fatal(err)
	}
	defer os.RemoveAll(outDir)

	err = copyTestUninstallDir(outDir)
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
			AddonDir: outDir,
		},
		AppCfg: config.AppConfig{
			Installed: map[string]*addon.Addon{
				"weakauras": {Folders: []string{"WeakAuras", "WeakAurasArchive"}},
			},
			Latest: map[string]*addon.Addon{
				"weakauras": {Folders: []string{"WeakAuras", "WeakAurasArchive"}},
			},
		},
	}

	err = Uninstall(&cfg, []string{"uninstall", "Really"})
	if err != nil {
		t.Fatal(err)
	}

	for _, expectedFile := range expectedFiles {
		filePath := filepath.Join(outDir, expectedFile)
		if _, err := os.Stat(filePath); err != nil {
			t.Fatalf("should find %s", filePath)
		}
	}

	var actualCount int
	err = filepath.WalkDir(outDir, func(path string, d fs.DirEntry, err error) error {
		if path == outDir {
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

	_, ok := cfg.AppCfg.Installed["weakauras"]
	if !ok {
		t.Fatalf("should not delete entry from install list")
	}

	_, ok = cfg.AppCfg.Latest["weakauras"]
	if !ok {
		t.Fatalf("should not delete entry from latest list")
	}
}

func copyTestUninstallDir(outDir string) error {
	testUninstallDir := filepath.Join(testDir, "mock_addon_dir")

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
		f, err := os.Open(filepath.Join(testUninstallDir, filePath))
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
			os.MkdirAll(dest, os.ModeDir)
			continue
		}

		if err := os.MkdirAll(filepath.Dir(dest), os.ModeDir); err != nil {
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
