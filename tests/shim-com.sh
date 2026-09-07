#!/usr/bin/env bash
set -euo pipefail

# Requires MinGW, Wine, Bubblewrap, Xvfb and xauth. No existing prefix is mounted.
repo=$(cd -- "$(dirname -- "$0")/.." && pwd)
cc=${CC:-x86_64-w64-mingw32-gcc}
wine=${WINE_BIN:-$(command -v wine || true)}
[[ -n $wine ]] || wine=/usr/lib/wine/wine64
[[ -x $wine ]] || { printf 'Wine executable not found\n' >&2; exit 1; }
for tool in bwrap xvfb-run xauth timeout; do command -v "$tool" >/dev/null; done
mkdir -p "$repo/.cache"
work=$(mktemp -d "$repo/.cache/shim-com.XXXXXXXX")
printf 'Shim test artifacts in %s\n' "$work"
mkdir -p "$work/home" "$work/tmp" "$work/runtime"
chmod 700 "$work/runtime"
CC="$cc" bash "$repo/shim/build.sh" "$work/winrtcamstub.dll"
"$cc" -Wall -Wextra -Werror -O2 -o "$work/shim-com.exe" "$repo/tests/shim-com.c" \
  -lwindowsapp -luser32 -lkernel32

mounts=(--ro-bind /usr /usr --symlink usr/bin /bin --ro-bind /lib /lib)
[[ ! -d /lib64 ]] || mounts+=(--ro-bind /lib64 /lib64)
for path in /etc/fonts /etc/ld.so.cache /etc/passwd /etc/group /etc/alternatives; do
  [[ ! -e $path ]] || mounts+=(--ro-bind "$path" "$path")
done
# shellcheck disable=SC2016
exec bwrap --unshare-all --die-with-parent --new-session --cap-drop ALL \
  "${mounts[@]}" --proc /proc --dev /dev \
  --bind "$work" /validation --bind "$work/tmp" /tmp \
  --clearenv --setenv PATH /usr/bin --setenv HOME /validation/home \
  --setenv LANG C.UTF-8 --setenv WINEPREFIX /validation/prefix --setenv WINEARCH win64 \
  --setenv XDG_RUNTIME_DIR /validation/runtime --setenv XDG_CACHE_HOME /validation/home/cache \
  --setenv XDG_CONFIG_HOME /validation/home/config --setenv XDG_DATA_HOME /validation/home/data \
  --setenv WINEDLLOVERRIDES 'mscoree,mshtml=' --setenv WINEDEBUG -all \
  --chdir /validation -- timeout --kill-after=10s 180s \
  xvfb-run -a -s '-screen 0 1280x900x24 -nolisten tcp -extension GLX' \
  bash -c 'set -euo pipefail
    [[ ! -e /sys && ! -e /dev/dri && ! -e /dev/bus/usb && ! -e /home ]]
    "$1" --version
    "$1" /validation/shim-com.exe | tee /validation/source.log
    "$1" /validation/shim-com.exe "Z:\validation\winrtcamstub.dll" | tee /validation/dll.log
  ' shim-tests "$wine"
