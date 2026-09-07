#!/usr/bin/env bash
set -euo pipefail
# shellcheck source=common.sh
source "$(dirname -- "$0")/common.sh"
check_version
[[ ! -e $WINEPREFIX/.lightburn-installing ]] || die 'LightBurn installation is incomplete. Run install.sh with the installer again.'
[[ -n ${DISPLAY:-} ]] || die 'DISPLAY is required to show LightBurn. Run from a graphical desktop session.'
[[ -f $APP && -f $WINEPREFIX/system.reg ]] || die 'Prefix is not initialized with LightBurn. Run install.sh first.'
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
# /exec waits on the application and ties it to the waiter's kill-on-close job.
# Unlike /unix, it preserves the caller's working directory and output handles.
wine 'C:\windows\system32\start.exe' /exec 'C:\LightBurn\LightBurn.exe' "$@" >> "$LOG_DIR/run.log" 2>&1 &
wine_pid=$!
# A signal can arrive between spawning Wine and recording its PID.
(( signal_status == 0 )) || interrupt "$signal_status"
wait "$wine_pid"
