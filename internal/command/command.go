package command

import "strings"

const catalogSuffix = ".json"
const notFound = "unable to find addon in catalog"

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
