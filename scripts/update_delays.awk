BEGIN {
    # How long to wait at the start before inputting commands.
    if (START_DELAY == "") {
        START_DELAY = 10
    }

    # How long to wait after a command has ended to start entering the next
    # command.
    if (PROMPT_DELAY == "") {
        PROMPT_DELAY = 1000
    }

    # How long to wait after each user input key.
    if (KEY_DELAY == "") {
        KEY_DELAY = 125
    }

    # How long to wait for the command to start.
    if (COMMAND_DELAY == "") {
        COMMAND_DELAY = 1000
    }

    # How long to wait at the end before the gif will loop.
    if (END_DELAY == "") {
        END_DELAY = 5000
    }

}

NR == 1 {
    prev = $0
    next
}

/records:/,/^$/ {
    is_last = 0;

    # Update the first delay before any commands have been typed.
    if (match(prev, /records:/)) {
        sub(/[[:digit:]][[:digit:]]*/, START_DELAY)
    }

    # Update delay for how long the shell prompt will be shown before entering a
    # command.
    if (match(prev, /content: ".*\$ "/)) {
        sub(/[[:digit:]][[:digit:]]*/, PROMPT_DELAY)
    }

    # Update key press delay.
    if (match(prev, /content: '[[:space:]]'/) || match(prev, /content: [[:alnum:]]/)) {
        sub(/[[:digit:]][[:digit:]]*/, KEY_DELAY)
    }

    # Update the delays after a full command has been entered.
    if (match($0, /content: "\\r\\n"/)) {
        sub(/[[:digit:]][[:digit:]]*/, COMMAND_DELAY, prev)
    }

    # Update the delay of the last recorded record.
    if (match($0, /content: "logout\\r\\n"/)) {
        sub(/[[:digit:]][[:digit:]]*/, END_DELAY, prev)
        sub(/logout/, "")
        is_last = 1
    }
}

{
    print prev
    prev = $0
}

END { print }
