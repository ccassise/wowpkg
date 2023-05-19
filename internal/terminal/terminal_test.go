package terminal

import "testing"

func TestBuilder(t *testing.T) {
	var actual Builder

	actual.AddArg(Bold).AddArg(Red).SetText("This is bold red text")
	expect := "\033[1;31mThis is bold red text\033[0m"

	if actual.String() != expect {
		t.Fatalf("want %v, got %v", expect, actual.String())
	}

	actual.Clear()
	expect = ""

	if actual.String() != expect {
		t.Fatalf("want `%v`, got `%v`", expect, actual.String())
	}

	actual.AddArg(Red).AddArg(Bold).AddArg(FGColorID(80)).SetText("Test")
	expect = "\033[31;1;38;5;80mTest\033[0m"

	if actual.String() != expect {
		t.Fatalf("want `%v`, got `%v`", expect, actual.String())
	}
}
