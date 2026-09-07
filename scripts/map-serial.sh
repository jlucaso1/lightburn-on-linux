#!/usr/bin/env bash
set -euo pipefail
# shellcheck source=common.sh
source "$(dirname -- "$0")/common.sh"
[[ $# == 2 ]] || die "Usage: $0 /dev/ttyUSB0 com1"
[[ $1 == /* && -c $1 ]] || die 'Serial device must be an absolute path to a character device.'
[[ $2 =~ ^com[1-9][0-9]*$ ]] || die 'Port must match com[1-9][0-9]*.'
prefix_initialized || die 'Wine prefix is not initialized.'
ln -sfnT -- "$1" "$WINEPREFIX/dosdevices/$2"
printf 'Mapped %s to %s\n' "$1" "$2"
