package config

import (
	"os"
	"path/filepath"
	"reflect"
	"testing"

	"github.com/ccassise/wowpkg/pkg/addon"
)

var testDir = filepath.Join("..", "..", "test")

func TestSaveLoad(t *testing.T) {
	appConfigPath := filepath.Join(testDir, "test_app_config.json")
	userConfigPath := filepath.Join(testDir, "test_user_config.json")

	expect := Config{
		AppCfg: AppConfig{
			Installed: map[string]*addon.Addon{
				"testfolder":  {Name: "testfolder", Desc: "testfolder desc", Version: "v1.0.0", Folders: []string{"test1", "test2", "test3"}},
				"test2folder": {Name: "test2folder", Desc: "test2folder desc", Version: "v1.2.3", Folders: []string{"test4"}},
			},
			Latest: map[string]*addon.Addon{
				"testfolder":  {Name: "testfolder", Desc: "testfolder desc", Version: "v1.0.0", Folders: []string{"test1", "test2", "test3"}},
				"test2folder": {Name: "test2folder", Desc: "test2folder desc", Version: "v1.2.3", Folders: []string{"test4"}},
			},
		},
		UserCfg: UserConfig{
			AddonDir: "test_addon_dir",
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
		t.Fatalf("AppCfg: not equal\n")
	}

	if expect.UserCfg != actual.UserCfg {
		t.Fatalf("UserCfg: want %s, got %s\n", expect.UserCfg, actual.UserCfg)
	}
}
