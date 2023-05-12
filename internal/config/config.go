package config

import (
	"encoding/json"
	"os"
	"path/filepath"
)

type Config struct {
	AppCfg  AppConfig
	UserCfg UserConfig
}

type AppConfig struct {
	Installed map[string][]string
}

type UserConfig struct {
	AddonDir   string `json:"addon_directory"`
	CatalogDir string `json:"catalog_directory"`
}

// Loads config files from default config locations on disk.
func (c *Config) Load() error {
	if err := c.AppCfg.Load(filepath.Join("dump", "wowpkg.json")); err != nil {
		return err
	}

	if err := c.UserCfg.Load(filepath.Join("dump", "config.json")); err != nil {
		return err
	}

	return nil
}

func (c *AppConfig) Load(path string) error {
	return loadFrom(path, c)
}

func (c *UserConfig) Load(path string) error {
	return loadFrom(path, c)
}

func loadFrom[T AppConfig | UserConfig](path string, c *T) error {
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
	if err := c.AppCfg.Save(filepath.Join("dump", "wowpkg.json")); err != nil {
		return err
	}

	if err := c.UserCfg.Save(filepath.Join("dump", "config.json")); err != nil {
		return err
	}

	return nil
}

func (c *AppConfig) Save(path string) error {
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
	if err = encoder.Encode(c); err != nil {
		return err
	}

	return nil
}
