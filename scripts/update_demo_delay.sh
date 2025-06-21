#!/bin/sh

# Updates delays in the demo YAML produced by terminalizer.
#
# This gives a consistent cadence to commands being entered and allows enough
# time before and after a command has been entered for the user to acknowledge
# it. The time it takes for a command itself to be ran is preserved.

if [ $# -lt 1 ]; then
    echo "PRINT USAGE" 1>&2
    exit 1
fi

start_delay=10
prompt_delay=1000
key_delay=125
command_delay=1000
end_delay=5000

while [ $# -gt 1 ]; do
    case "$1" in
        ("-s" | "--start_delay")
            start_delay=$2
            shift 2
            ;;
        ("-p" | "--prompt_delay")
            prompt_delay=$2
            shift 2
            ;;
        ("-k" | "--key_delay")
            key_delay=$2
            shift 2
            ;;
        ("-c" | "--command_delay")
            command_delay=$2
            shift 2
            ;;
        ("-e" | "--end_delay")
            end_delay=$2
            shift 2
            ;;
        ("-h" | "--help")
            echo "PRINT HELP"
            exit 0
            ;;
        (*)
            echo "error: unrecognized argument '$1'" 1>&2
            exit 1
            ;;
    esac
done

file=$1

sed -e '# Change the shell prompt to be just a "$".' \
    -e 's/content: "\(.*\\r\\n\).*\$ "/content: "\1$ "/' \
    -e "s/content: '.*\$ '/content: \"$ \"/" \
    -e '
        # Delete the bash startup message in macOS and the delay after.
        /The default interactive shell is now zsh\./ {
            N
            d
        }
    ' \
    "$file" \
| awk \
    -v "START_DELAY=$start_delay" \
    -v "PROMPT_DELAY=$prompt_delay" \
    -v "KEY_DELAY=$key_delay" \
    -v "COMMAND_DELAY=$command_delay" \
    -v "END_DELAY=$end_delay" \
    -f update_delays.awk
