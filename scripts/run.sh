#!/usr/bin/env bash
set -euo pipefail
# shellcheck source=common.sh
source "$(dirname -- "$0")/common.sh"
[[ ! -e $WINEPREFIX/.lightburn-installing ]] || die 'LightBurn installation is incomplete. Run install.sh with the installer again.'
[[ -n ${DISPLAY:-} ]] || die 'DISPLAY is required to show LightBurn. Run from a graphical desktop session.'
{ prefix_initialized && [[ -f $APP ]]; } || die 'Prefix is not initialized with LightBurn. Run install.sh first.'
[[ -f $LAUNCHER && -s $LAUNCHER ]] || die 'Native launcher is missing or empty. Run install.sh without an installer to repair this prefix.'
mkdir -p -- "$LOG_DIR"
printf 'Wine stdout and stderr are appended to %s/run.log\n' "$LOG_DIR" >&2
wine_pid=
signal_status=0
interrupt() {
  signal_status=$1
  [[ -n $wine_pid ]] || return 0
  trap '' HUP INT TERM
  kill -TERM "$wine_pid" 2>/dev/null || true
  wait "$wine_pid" 2>/dev/null || true
  exit "$signal_status"
}
trap 'interrupt 129' HUP
trap 'interrupt 130' INT
trap 'interrupt 143' TERM
# The native helper owns a non-breakaway job for itself and its descendants.
wine 'C:\LightBurn\start-lightburn.exe' "$@" >> "$LOG_DIR/run.log" 2>&1 &
wine_pid=$!
# A signal can arrive between spawning Wine and recording its PID.
(( signal_status == 0 )) || interrupt "$signal_status"
wait "$wine_pid"
