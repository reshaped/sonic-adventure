#!/bin/sh
# CI's native compiler and assembler keep each target's final output in its usual log.
set -u
log=$1
admitted=$2
shift 2
monitor=$log.monitor
first=$log.timeout.${2##*/}
trap 'rm -f "$monitor"' EXIT

attempt() {
    # Only timeout writes the monitor file. A child can print the same text or
    # exit 124/137 itself without triggering a retry.
    LC_ALL=C timeout --verbose -k "$GRACE" "$HUNG" sh -c 'exec "$@" 2>&1' sh "$@" >"$log" 2>"$monitor"
    status=$?
    if [ -s "$monitor" ]; then cat "$monitor" >>"$log"; fi
}

attempt "$@"
case $status in 124|137) ;; *) exit "$status";; esac
if ! grep -q '^timeout: sending signal TERM to command ' "$monitor"; then exit "$status"; fi
# A tool can print a fatal diagnostic before hanging. Preserve the normal
# admitted-warning check when deciding whether its incomplete attempt is eligible.
if ! awk -v admitted="$admitted" '/ \([EWF]\) / { if (admitted != "" && index($0, admitted) && !seen++) next; exit 1 }' "$log"; then
    exit "$status"
fi

cp "$log" "$first" || exit 1
cat "$first" >&2
echo "$log: wibo timed out running $2, retrying once (first output in $first)" >&2
attempt "$@"
echo "$log: wibo retry exited with status $status" >&2
exit "$status"
