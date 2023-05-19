package zipper

import (
	"archive/zip"
	"os"
	"path/filepath"
	"strings"
	"testing"
)

var testPath = filepath.Join("..", "..", "test")

func TestInflate(t *testing.T) {
	outPath := filepath.Join(testPath, "results")
	defer os.RemoveAll(outPath)

	err := Unzip(outPath, filepath.Join(testPath, "mock.zip"))
	if err != nil {
		t.Fatal(err)
	}

	expectedDirectories := []string{outPath, filepath.Join(outPath, "dir")}

	for _, expected := range expectedDirectories {
		f, err := os.Stat(expected)
		if err != nil {
			t.Fatal(err)
		}

		if !f.IsDir() {
			t.Fatalf("a directory '%s' should exist", expected)
		}
	}

	expectedFiles := []string{filepath.Join(outPath, "test.txt"), filepath.Join(outPath, "dir", "hello.txt")}

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
	outPath := filepath.Join(testPath, "results")
	defer os.RemoveAll(outPath)

	r, err := zip.OpenReader(filepath.Join(testPath, "mock.zip"))
	if err != nil {
		t.Fatal(err)
	}
	defer r.Close()

	for _, f := range r.File {
		if strings.HasSuffix(f.FileInfo().Name(), "hello.txt") {
			err := UnzipFile(outPath, f)
			if err != nil {
				t.Fatal(err)
			}
		}
	}

	expected := filepath.Join(outPath, "dir", "hello.txt")
	f, err := os.Stat(expected)
	if err != nil {
		t.Fatal(err)
	}

	if f.IsDir() {
		t.Fatalf("expected a file '%s' but got a directory", expected)
	}
}
