package command

import "strings"

type Config struct {
	AddonDir   string
	CatalogDir string
}

const catalogSuffix = ".json"

// Searches caseSensitiveStrs for a string that matches the case-insensitive s
// and returns it. If no match was found then s is returned.
func caseSensitiveStr(caseSensitiveStrs []string, s string) string {
	for _, str := range caseSensitiveStrs {
		if strings.EqualFold(str, s) {
			return str
		}
	}
	return s
}
