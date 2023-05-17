package command

type InvalidArgs struct{}

func (*InvalidArgs) Error() string {
	return "invalid args"
}
