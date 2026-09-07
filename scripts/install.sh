#!/usr/bin/env bash
set -euo pipefail
# shellcheck source=common.sh
source "$(dirname -- "$0")/common.sh"
[[ $# -le 1 ]] || die "Usage: $0 [installer.exe]"
installer=${1:-${LB_WIN_EXE:-}}
installing="$WINEPREFIX/.lightburn-installing"
new_install=0
if [[ -n $installer || ! -f $APP || -e $installing ]]; then
  check_version
fi
if [[ ! -f $APP || -e $installing ]]; then
  new_install=1
  [[ -n $LB_VERSION && $LB_VERSION != *[[:cntrl:]/\\]* ]] || die 'LB_VERSION must be nonempty and contain no control characters or path separators.'
  [[ -n $LB_WIN_SHA256 ]] || die 'No installer checksum configured. Set LB_WIN_SHA256 for this LB_VERSION.'
  [[ -n $installer && -f $installer ]] || die 'Supply an installer path as an argument or LB_WIN_EXE.'
  [[ $installer == /* ]] || installer="$PWD/$installer"
  digest=$(sha256sum -- "$installer")
  [[ ${digest%% *} == "$LB_WIN_SHA256" ]] || die 'Installer SHA-256 does not match LB_WIN_SHA256.'
fi
command -v wine >/dev/null
command -v wineboot >/dev/null
if [[ -z ${DISPLAY:-} ]]; then
  command -v xvfb-run >/dev/null
  exec xvfb-run -a bash "$REPO/scripts/install.sh" "$@"
fi
if [[ $new_install == 0 && ! -e $STAMP ]]; then
  printf 'Warning: installed LightBurn version is unknown; repair will leave it unstamped.\n' >&2
fi

build_dir=
trap 'if [[ -n $build_dir ]]; then rm -rf -- "$build_dir"; fi' EXIT
if [[ ${WINRTCAMSTUB_DLL+x} ]]; then
  [[ -f $WINRTCAMSTUB_DLL && -s $WINRTCAMSTUB_DLL ]] || die 'WINRTCAMSTUB_DLL must name a nonempty DLL file.'
  dll=$WINRTCAMSTUB_DLL
else
  cache="${XDG_CACHE_HOME:-$HOME/.cache}/lightburn-on-linux"
  [[ $cache == /* ]] || die 'XDG_CACHE_HOME must be an absolute path.'
  mkdir -p -- "$cache"
  build_dir=$(mktemp -d "$cache/build.XXXXXX")
  dll="$build_dir/winrtcamstub.dll"
  bash "$REPO/shim/build.sh" "$dll"
fi

mkdir -p -- "$LOG_DIR"
printf 'Installer output is appended to %s/install.log\n' "$LOG_DIR" >&2
exec >> "$LOG_DIR/install.log" 2>&1
mkdir -p -- "$WINEPREFIX"
wineboot --init
if [[ $new_install == 1 ]]; then
  touch -- "$installing"
  wine "$installer" /VERYSILENT /NORESTART /SUPPRESSMSGBOXES '/DIR=C:\LightBurn'
  [[ -f $APP ]] || die 'Installer completed without creating LightBurn.exe.'
  rm -- "$installing"
fi
[[ -f $APP ]] || die 'Installer completed without creating LightBurn.exe.'
cp -- "$dll" "$WINEPREFIX/drive_c/windows/system32/winrtcamstub.dll"
for class in Windows.Media.Capture.Frames.MediaFrameSourceGroup Windows.Devices.Enumeration.DeviceInformation; do
  wine reg add "HKLM\\Software\\Microsoft\\WindowsRuntime\\ActivatableClassId\\$class" \
    /v DllPath /t REG_SZ /d 'C:\Windows\System32\winrtcamstub.dll' /f
done
if [[ $new_install == 1 ]]; then
  printf '%s\n' "$LB_VERSION" > "$STAMP"
fi
printf 'Installation and shim registration completed.\n'
