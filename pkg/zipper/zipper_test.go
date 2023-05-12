package zipper

import (
	"archive/zip"
	"os"
	"path/filepath"
	"strings"
	"testing"
)

var testDir = filepath.Join("..", "..", "test")

func TestInflate(t *testing.T) {
	outputDir := filepath.Join(testDir, "results")
	defer os.RemoveAll(outputDir)

	err := Unzip(outputDir, filepath.Join(testDir, "mock.zip"))
	if err != nil {
		t.Fatal(err)
	}

	expectedDirectories := []string{outputDir, filepath.Join(outputDir, "dir")}

	for _, expected := range expectedDirectories {
		f, err := os.Stat(expected)
		if err != nil {
			t.Fatal(err)
		}

		if !f.IsDir() {
			t.Fatalf("a directory '%s' should exist", expected)
		}
	}

	expectedFiles := []string{filepath.Join(outputDir, "test.txt"), filepath.Join(outputDir, "dir", "hello.txt")}

	for _, expected := range expectedFiles {
		f, err := os.Stat(expected)
		if err != nil {
			t.Fatal(err)
		}

		if f.IsDir() {
			t.Fatalf("expected a file '%s' but got a directory", expected)
		}
	}
}

func TestInflateFile(t *testing.T) {
	outputDirectory := filepath.Join(testDir, "results")
	defer os.RemoveAll(outputDirectory)

	r, err := zip.OpenReader(filepath.Join(testDir, "mock.zip"))
	if err != nil {
		t.Fatal(err)
	}
	defer r.Close()

	for _, f := range r.File {
		if strings.HasSuffix(f.FileInfo().Name(), "hello.txt") {
			err := UnzipFile(outputDirectory, f)
			if err != nil {
				t.Fatal(err)
			}
		}
	}

	expected := filepath.Join(outputDirectory, "dir", "hello.txt")
	f, err := os.Stat(expected)
	if err != nil {
		t.Fatal(err)
	}

	if f.IsDir() {
		t.Fatalf("expected a file '%s' but got a directory", expected)
	}
}
