package terminal

import (
	"fmt"
	"strings"
)

type Builder struct {
	text string
	args []string
}

const (
	Bold          = "1"
	Italics       = "3"
	Underlined    = "4"
	Strikethrough = "9"
)

const (
	Black   = "30"
	Red     = "31"
	Green   = "32"
	Yellow  = "33"
	Blue    = "34"
	Magenta = "35"
	Cyan    = "36"
	White   = "37"
	Default = "39"
)

func (b *Builder) SetText(s string) *Builder {
	b.text = s
	return b
}

func (b *Builder) AddArg(arg string) *Builder {
	b.args = append(b.args, arg)
	return b
}

func FGColorID(color uint8) string {
	return fmt.Sprintf("38;5;%d", color)
}

func (b *Builder) Clear() *Builder {
	b.text = ""
	b.args = []string{}

	return b
}

func (b *Builder) String() string {
	var sb strings.Builder

	if len(b.args) > 0 {
		sb.WriteString("\033[")
		sb.WriteString(strings.Join(b.args, ";"))
		sb.WriteString("m")
	}

	sb.WriteString(b.text)

	if len(b.args) > 0 {
		sb.WriteString("\033[0m")
	}

	return sb.String()
}
