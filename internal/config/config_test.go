package config

import (
	"os"
	"path/filepath"
	"reflect"
	"testing"

	"github.com/ccassise/wowpkg/pkg/addon"
)

var testPath = filepath.Join("..", "..", "test")

func TestSaveLoad(t *testing.T) {
	appStatePath := filepath.Join(testPath, "test_app_config.json")
	userConfigPath := filepath.Join(testPath, "test_user_config.json")

	expect := Config{
		AppState: State{
			Installed: map[string]*addon.Addon{
				"testfolder":  {Name: "testfolder", Desc: "testfolder desc", Version: "v1.0.0", Dirs: []string{"test1", "test2", "test3"}},
				"test2folder": {Name: "test2folder", Desc: "test2folder desc", Version: "v1.2.3", Dirs: []string{"test4"}},
			},
			Latest: map[string]*addon.Addon{
				"testfolder":  {Name: "testfolder", Desc: "testfolder desc", Version: "v1.0.0", Dirs: []string{"test1", "test2", "test3"}},
				"test2folder": {Name: "test2folder", Desc: "test2folder desc", Version: "v1.2.3", Dirs: []string{"test4"}},
			},
		},
		UserCfg: UserConfig{
			AddonPath: "test_addon_dir",
		},
	}

	if err := expect.AppState.Save(appStatePath); err != nil {
		t.Fatalf("%s\n", err)
	}
	defer os.Remove(appStatePath)

	if err := expect.UserCfg.Save(userConfigPath); err != nil {
		t.Fatalf("%s\n", err)
	}
	defer os.Remove(userConfigPath)

	actual := Config{
		UserCfg:  UserConfig{},
		AppState: State{},
	}

	if err := actual.AppState.Load(appStatePath); err != nil {
		t.Fatalf("%s\n", err)
	}

	if err := actual.UserCfg.Load(userConfigPath); err != nil {
		t.Fatalf("%s\n", err)
	}

	if !reflect.DeepEqual(expect.AppState, actual.AppState) {
		t.Fatalf("AppCfg: not equal\n")
	}

	if expect.UserCfg != actual.UserCfg {
		t.Fatalf("UserCfg: want %s, got %s\n", expect.UserCfg, actual.UserCfg)
	}
}
