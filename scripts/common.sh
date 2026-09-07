#!/usr/bin/env bash

REPO=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
# shellcheck source=../versions.env
source "$REPO/versions.env"

die() { printf '%s\n' "$*" >&2; exit 1; }

if [[ ! ${WINEPREFIX+x} ]]; then
  if [[ -d "$REPO/.work/wineprefix" ]]; then
    WINEPREFIX="$REPO/.work/wineprefix"
  else
    WINEPREFIX="${XDG_DATA_HOME:-$HOME/.local/share}/lightburn-on-linux/wineprefix"
  fi
fi
[[ $WINEPREFIX == /* ]] || die 'WINEPREFIX must be an absolute path.'
[[ $WINEARCH == win64 ]] || die 'Only WINEARCH=win64 is supported.'
LOG_DIR="${XDG_STATE_HOME:-$HOME/.local/state}/lightburn-on-linux"
[[ $LOG_DIR == /* ]] || die 'XDG_STATE_HOME must be an absolute path.'
export WINEPREFIX WINEARCH
# shellcheck disable=SC2034 # Used by install.sh and run.sh.
APP="$WINEPREFIX/drive_c/LightBurn/LightBurn.exe"
STAMP="$WINEPREFIX/.lightburn-version"

prefix_initialized() {
  [[ -d $WINEPREFIX/drive_c && -d $WINEPREFIX/dosdevices ]]
}

check_version() {
  if [[ -e $STAMP ]]; then
    local installed
    installed=$(< "$STAMP")
    [[ $installed == "$LB_VERSION" ]] || die "Prefix version '$installed' differs from '$LB_VERSION'. Use a separate WINEPREFIX."
  fi
}
