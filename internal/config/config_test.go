package config

import (
	"os"
	"path/filepath"
	"reflect"
	"testing"
)

var testDir = filepath.Join("..", "..", "test")

func TestSaveLoad(t *testing.T) {
	appConfigPath := filepath.Join(testDir, "test_app_config.json")
	userConfigPath := filepath.Join(testDir, "test_user_config.json")

	expect := Config{
		AppCfg: AppConfig{
			Installed: map[string][]string{
				"testfolder":  {"test1", "test2", "test3"},
				"test2folder": {"test4"},
			},
		},
		UserCfg: UserConfig{
			AddonDir:   "test_addon_dir",
			CatalogDir: "test_catalog_dir",
		},
	}

	if err := expect.AppCfg.Save(appConfigPath); err != nil {
		t.Fatalf("%s\n", err)
	}
	defer os.Remove(appConfigPath)

	if err := expect.UserCfg.Save(userConfigPath); err != nil {
		t.Fatalf("%s\n", err)
	}
	defer os.Remove(userConfigPath)

	actual := Config{
		UserCfg: UserConfig{},
		AppCfg:  AppConfig{},
	}

	if err := actual.AppCfg.Load(appConfigPath); err != nil {
		t.Fatalf("%s\n", err)
	}

	if err := actual.UserCfg.Load(userConfigPath); err != nil {
		t.Fatalf("%s\n", err)
	}

	if !reflect.DeepEqual(expect.AppCfg, actual.AppCfg) {
		t.Fatalf("AppCfg: want %s, got %s\n", expect.AppCfg, actual.AppCfg)
	}

	if expect.UserCfg != actual.UserCfg {
		t.Fatalf("UserCfg: want %s, got %s\n", expect.UserCfg, actual.UserCfg)
	}
}
