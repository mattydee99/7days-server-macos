#!/bin/bash
# Run a server console command over the container-local telnet port and print
# its output. Telnet is not published outside the container.
#
#   docker exec 7days console.sh "castle survey 520 316 564 354"
#   docker exec 7days console.sh -w 10 "castle survey ..."   # wait longer for output
set -e

WAIT=3
if [ "$1" = "-w" ]; then
    WAIT="$2"
    shift 2
fi
if [ -z "$1" ]; then
    echo "usage: console.sh [-w seconds] \"<command>\"" >&2
    exit 1
fi

exec 3<>/dev/tcp/127.0.0.1/8081
printf '%s\r\n' "$TELNET_PASSWORD" >&3
sleep 1
printf '%s\r\n' "$1" >&3
sleep "$WAIT"
printf 'exit\r\n' >&3
# Drop the login banner: print from the echoed command onwards.
timeout 5 cat <&3 | tr -d '\r' | awk -v c="Executing command '$1'" 'index($0, c) { p = 1 } p'
exec 3<&-
