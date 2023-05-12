package zipper

import (
	"archive/zip"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"strings"
)

// Extracts .zip src file to the destination dest.
func Unzip(dest, src string) error {
	r, err := zip.OpenReader(src)
	if err != nil {
		return err
	}
	defer r.Close()

	for _, f := range r.File {
		if err = UnzipFile(dest, f); err != nil {
			return err
		}
	}

	return nil
}

// Extracts only a specific file from the .zip to the destination dest.
func UnzipFile(dest string, f *zip.File) error {
	const invalidFilePathErr = "invalid file path"
	destination, err := filepath.Abs(dest)
	if err != nil {
		return fmt.Errorf("%s: %s", invalidFilePathErr, destination)
	}

	file := filepath.Join(destination, f.Name)
	if !strings.HasPrefix(file, filepath.Clean(destination)+string(os.PathSeparator)) {
		return fmt.Errorf("%s: %s", invalidFilePathErr, file)
	}

	if f.FileInfo().IsDir() {
		if err := os.MkdirAll(file, os.ModeDir); err != nil {
			return err
		}
		return nil
	}

	if err := os.MkdirAll(filepath.Dir(file), os.ModeDir); err != nil {
		return err
	}

	destinationFile, err := os.OpenFile(file, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, f.Mode())
	if err != nil {
		return err
	}
	defer destinationFile.Close()

	zippedFile, err := f.Open()
	if err != nil {
		return err
	}
	defer zippedFile.Close()

	_, err = io.Copy(destinationFile, zippedFile)
	if err != nil {
		return err
	}

	return nil
}
