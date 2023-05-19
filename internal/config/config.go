package config

import (
	"encoding/json"
	"os"
	"path/filepath"

	"github.com/ccassise/wowpkg/pkg/addon"
)

type Config struct {
	AppState State
	UserCfg  UserConfig
}

type State struct {
	Installed map[string]*addon.Addon `json:"installed"`
	Latest    map[string]*addon.Addon `json:"latest"`
}

type UserConfig struct {
	AddonPath string `json:"addon_path"`
}

func CfgPath() string {
	ep, _ := os.Executable()
	return filepath.Join(ep, "..", "..", "config.json")

	// return filepath.Join("dump", "config.json")
}

func StatePath() string {
	ep, _ := os.Executable()
	return filepath.Join(ep, "..", "..", "state.json")

	// return filepath.Join("dump", "state.json")
}

// Loads config files from default config locations on disk.
func (c *Config) Load() error {
	if err := c.AppState.Load(StatePath()); err != nil {
		return err
	}

	if err := c.UserCfg.Load(CfgPath()); err != nil {
		return err
	}

	return nil
}

func (c *State) Load(path string) error {
	return loadFrom(path, c)
}

func (c *UserConfig) Load(path string) error {
	return loadFrom(path, c)
}

func loadFrom[T State | UserConfig](path string, c *T) error {
	f, err := os.Open(path)
	if err != nil {
		return err
	}
	defer f.Close()

	if err = json.NewDecoder(f).Decode(c); err != nil {
		return err
	}

	return nil
}

// Writes the config files to default config locations on disk.
func (c *Config) Save() error {
	if err := c.AppState.Save(StatePath()); err != nil {
		return err
	}

	if err := c.UserCfg.Save(CfgPath()); err != nil {
		return err
	}

	return nil
}

func (c *State) Save(path string) error {
	return saveTo(path, c)
}

func (c *UserConfig) Save(path string) error {
	return saveTo(path, c)
}

func saveTo(path string, c interface{}) error {
	f, err := os.Create(path)
	if err != nil {
		return err
	}
	defer f.Close()

	encoder := json.NewEncoder(f)
	encoder.SetIndent("", "\t")
	if err = encoder.Encode(c); err != nil {
		return err
	}

	return nil
}
