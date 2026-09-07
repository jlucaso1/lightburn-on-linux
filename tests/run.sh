#!/usr/bin/env bash
set -euo pipefail
REPO=$(cd -- "$(dirname -- "$0")/.." && pwd)
mkdir -p "$REPO/.cache"
WORK=$(mktemp -d "$REPO/.cache/shell-tests.XXXXXX")
trap 'rm -rf -- "$WORK"' EXIT
mkdir -p "$WORK/repo/shim" "$WORK/bin"
cp -R "$REPO/scripts" "$WORK/repo/"
cp "$REPO/shim/build.sh" "$WORK/repo/shim/"
for tool in wine wineboot xvfb-run compiler x86_64-w64-mingw32-gcc; do
  cp "$REPO/tests/mock-tool.sh" "$WORK/bin/$tool"
  chmod +x "$WORK/bin/$tool"
done
export PATH="$WORK/bin:$PATH" CC="$WORK/bin/compiler"
export HOME="$WORK/home" XDG_DATA_HOME="$WORK/data space" XDG_STATE_HOME="$WORK/state space"
export XDG_CACHE_HOME="$WORK/cache space" TRACE="$WORK/trace" DISPLAY=:123
unset WINEPREFIX WINRTCAMSTUB_DLL
printf 'mock installer\n' > "$WORK/installer space.exe"
hash=$(sha256sum "$WORK/installer space.exe")
cp "$REPO/versions.env" "$WORK/repo/versions.env"
export LB_WIN_SHA256=${hash%% *} LB_VERSION=2.1.04 WINEARCH=win64
unset LB_WIN_EXE
passed=0 failed=0
check() {
  local name=$1
  shift
  if "$@"; then
    printf 'PASS %s\n' "$name"
    passed=$((passed + 1))
  else
    printf 'FAIL %s\n' "$name"
    failed=$((failed + 1))
  fi
}
install() { bash "$WORK/repo/scripts/install.sh" "$WORK/installer space.exe" > "$WORK/output" 2>&1; }
fails_with() {
  local expected=$1 status=0
  shift
  "$@" || status=$?
  [[ $status == "$expected" ]]
}
selected_hash() {
  # shellcheck disable=SC2016 # Expansion belongs to the child shell.
  env -u LB_WIN_SHA256 LB_VERSION="$1" bash -c 'source "$1"; [[ $LB_WIN_SHA256 == "$2" ]]' _ "$WORK/repo/versions.env" "$2"
}
check '2.1.04 selects official checksum' selected_hash 2.1.04 1209eb5c8467a9aefa4eabbace5e982e232f8352db24797bb01f56671299b17b
check '2.1.00 selects official checksum' selected_hash 2.1.00 00b22facdd24465195a1ce8e92546a9580d216c76ee1c82e505519b7ebcbd8ce
check 'unknown version has no default checksum' selected_hash 2.99.00 ''
# shellcheck disable=SC2016 # Expansion belongs to the child shell.
check 'explicit checksum overrides known selection' bash -c 'source "$1"; [[ $LB_WIN_SHA256 == "$2" ]]' _ "$WORK/repo/versions.env" "${hash%% *}"
# shellcheck disable=SC2016 # Expansion belongs to the child shell.
check 'explicit empty checksum stays empty' env LB_WIN_SHA256= bash -c 'source "$1"; [[ -z $LB_WIN_SHA256 ]]' _ "$WORK/repo/versions.env"
export FAIL=wineboot
check 'wineboot failure is preserved' fails_with 31 install
export FAIL=reg
check 'registry failure is preserved' fails_with 32 install
check 'failed registration leaves no version stamp' test ! -e "$XDG_DATA_HOME/lightburn-on-linux/wineprefix/.lightburn-version"
unset FAIL
check 'repair needs no installer' bash "$WORK/repo/scripts/install.sh"
check 'repair after registration failure leaves version unknown' test ! -e "$XDG_DATA_HOME/lightburn-on-linux/wineprefix/.lightburn-version"
check 'successful rerun' install
check 'exact MediaFrameSourceGroup registry key' grep -Fxq 'wine <reg> <add> <HKLM\Software\Microsoft\WindowsRuntime\ActivatableClassId\Windows.Media.Capture.Frames.MediaFrameSourceGroup> </v> <DllPath> </t> <REG_SZ> </d> <C:\Windows\System32\winrtcamstub.dll> </f>' "$TRACE"
check 'exact DeviceInformation registry key' grep -Fxq 'wine <reg> <add> <HKLM\Software\Microsoft\WindowsRuntime\ActivatableClassId\Windows.Devices.Enumeration.DeviceInformation> </v> <DllPath> </t> <REG_SZ> </d> <C:\Windows\System32\winrtcamstub.dll> </f>' "$TRACE"
export WINEPREFIX="$WORK/custom prefix"
export LB_WIN_SHA256=bad
: > "$TRACE"
check 'checksum failure' fails_with 1 install
check 'checksum failure invokes no tools' test ! -s "$TRACE"
check 'checksum failure creates no prefix' test ! -e "$WINEPREFIX"
export LB_WIN_SHA256=${hash%% *} FAIL=compiler
check 'compiler failure' fails_with 34 install
check 'compiler failure creates no prefix' test ! -e "$WINEPREFIX"
unset FAIL
export TARGET=aarch64-w64-mingw32
check 'wrong compiler target' fails_with 1 install
check 'wrong compiler target creates no prefix' test ! -e "$WINEPREFIX"
unset TARGET
export FAIL=installer
check 'installer failure is preserved' fails_with 33 install
check 'installer failure leaves no stamp' test ! -e "$WINEPREFIX/.lightburn-version"
unset FAIL DISPLAY
: > "$TRACE"
check 'headless retry with spaced installer and prefix' install
check 'headless uses automatic display allocation' grep -q 'xvfb-run <-a>' "$TRACE"
check 'one Xvfb session for entire setup' test "$(grep -c '^xvfb-run ' "$TRACE")" = 1
check 'shim builds only once under Xvfb' test "$(grep -c '^compiler <-shared>' "$TRACE")" = 1
check 'new successful install stamps version' test "$(< "$WINEPREFIX/.lightburn-version")" = 2.1.04
check 'explicit prefix is honored' test -f "$WINEPREFIX/drive_c/LightBurn/LightBurn.exe"
check 'external build leaves source tree untouched' test ! -e "$WORK/repo/shim/winrtcamstub.dll"
: > "$TRACE"
check 'headless launcher is rejected' fails_with 1 bash "$WORK/repo/scripts/run.sh"
check 'headless launcher invokes no tools' test ! -s "$TRACE"
for failure in wineboot installer reg xvfb; do
  case "$failure" in
    wineboot) status=31 ;;
    installer) status=33 ;;
    reg) status=32 ;;
    xvfb) status=35 ;;
  esac
  check "headless $failure failure is preserved" fails_with "$status" env \
    WINEPREFIX="$WORK/headless-$failure" FAIL="$failure" \
    bash "$WORK/repo/scripts/install.sh" "$WORK/installer space.exe"
  check "headless $failure failure leaves no stamp" test ! -e "$WORK/headless-$failure/.lightburn-version"
done
export DISPLAY=:123
launch_status=0
"$WORK/repo/scripts/run.sh" 'file with spaces.lbrn2' '--example=a b' > "$WORK/launcher.stdout" 2> "$WORK/launcher.stderr" || launch_status=$?
check 'launcher forwards arguments by direct invocation' test "$launch_status" = 0
check 'launcher argument boundaries' grep -Fq 'wine <C:\windows\system32\start.exe> </exec> <C:\LightBurn\LightBurn.exe> <file with spaces.lbrn2> <--example=a b>' "$TRACE"
check 'launcher logs stderr' grep -q 'launcher stderr' "$XDG_STATE_HOME/lightburn-on-linux/run.log"
check 'launcher logs stdout' grep -q 'launcher stdout' "$XDG_STATE_HOME/lightburn-on-linux/run.log"
check 'launcher announces log on stderr' grep -Fq "$XDG_STATE_HOME/lightburn-on-linux/run.log" "$WORK/launcher.stderr"
check 'Wine output is not echoed to stderr' fails_with 1 grep -q 'launcher stderr' "$WORK/launcher.stderr"
check 'launcher leaves stdout empty' test ! -s "$WORK/launcher.stdout"
export RUN_STATUS=37
check 'launcher preserves failure' fails_with 37 bash "$WORK/repo/scripts/run.sh"
unset RUN_STATUS
export LB_VERSION=other
: > "$TRACE"
check 'mismatched version refuses explicit installer' fails_with 1 install
check 'mismatch invokes no tools' test ! -s "$TRACE"
check 'mismatched configured version permits launch' bash "$WORK/repo/scripts/run.sh"
export LB_VERSION=2.1.04
printf 'packaged DLL\n' > "$WORK/packaged DLL.dll"
export WINRTCAMSTUB_DLL="$WORK/packaged DLL.dll" CC=/nonexistent/compiler
check 'packaged DLL skips compiler and installer on repair' bash "$WORK/repo/scripts/install.sh"
check 'packaged DLL copied' cmp "$WINRTCAMSTUB_DLL" "$WINEPREFIX/drive_c/windows/system32/winrtcamstub.dll"
unset WINRTCAMSTUB_DLL
export CC="$WORK/bin/compiler"
check 'build accepts external output by direct invocation' "$WORK/repo/shim/build.sh" "$WORK/external DLL.dll"
check 'external output exists' test -s "$WORK/external DLL.dll"
check 'build detects compiler on PATH' env -u CC bash "$WORK/repo/shim/build.sh" "$WORK/detected.dll"
export WINEPREFIX=relative
check 'relative prefix refused' fails_with 1 install
unset WINEPREFIX
mkdir -p "$WORK/repo/.work/wineprefix/dosdevices"
touch "$WORK/repo/.work/wineprefix/system.reg"
mkdir -p "$WORK/repo/.work/wineprefix/drive_c/LightBurn"
touch "$WORK/repo/.work/wineprefix/drive_c/LightBurn/LightBurn.exe"
check 'unstamped legacy repair succeeds' install
check 'legacy version remains unknown' test ! -e "$WORK/repo/.work/wineprefix/.lightburn-version"
check 'legacy repair warns about unknown version' grep -q 'version is unknown' "$WORK/output"
check 'serial traversal is rejected' fails_with 1 bash "$WORK/repo/scripts/map-serial.sh" /dev/null ../escaped
check 'serial traversal creates nothing outside dosdevices' test ! -L "$WORK/repo/.work/wineprefix/escaped"
check 'serial direct invocation selects legacy prefix' "$WORK/repo/scripts/map-serial.sh" /dev/null com1
check 'mapping is in legacy prefix' test -L "$WORK/repo/.work/wineprefix/dosdevices/com1"
for port in com0 COM1 com01 com-1 'com1/../../escaped'; do
  check "invalid port $port" fails_with 1 bash "$WORK/repo/scripts/map-serial.sh" /dev/null "$port"
done
check 'relative device rejected' fails_with 1 bash "$WORK/repo/scripts/map-serial.sh" dev/null com1
check 'regular file rejected' fails_with 1 bash "$WORK/repo/scripts/map-serial.sh" "$WORK/installer space.exe" com1
mkdir "$WORK/target directory"
ln -s "$WORK/target directory" "$WORK/repo/.work/wineprefix/dosdevices/com2"
check 'directory symlink replaced rather than followed' bash "$WORK/repo/scripts/map-serial.sh" /dev/null com2
check 'replacement points at device' test "$(readlink "$WORK/repo/.work/wineprefix/dosdevices/com2")" = /dev/null
check 'target directory untouched' test ! -e "$WORK/target directory/null"
export WINEPREFIX="$WORK/uninitialized"
check 'uninitialized prefix rejected' fails_with 1 bash "$WORK/repo/scripts/map-serial.sh" /dev/null com1
for entry in scripts/install.sh scripts/run.sh scripts/map-serial.sh shim/build.sh; do
  check "$entry is executable" test -x "$WORK/repo/$entry"
done
export WINEPREFIX="$WORK/partial install" FAIL=partial-installer
check 'partial installer failure is preserved' fails_with 36 install
check 'partial installer wrote app' test -f "$WINEPREFIX/drive_c/LightBurn/LightBurn.exe"
check 'partial installer leaves marker' test -f "$WINEPREFIX/.lightburn-installing"
unset FAIL
: > "$TRACE"
check 'partial install retry requires installer' fails_with 1 bash "$WORK/repo/scripts/install.sh"
check 'missing retry installer invokes no tools' test ! -s "$TRACE"
check 'partial installation cannot launch' fails_with 1 "$WORK/repo/scripts/run.sh"
check 'partial launch invokes no tools' test ! -s "$TRACE"
export LB_WIN_SHA256=bad
check 'partial install retry checks checksum' fails_with 1 install
export LB_WIN_SHA256=${hash%% *}
: > "$TRACE"
check 'partial install retry succeeds by direct invocation' "$WORK/repo/scripts/install.sh" "$WORK/installer space.exe"
check 'partial install retry runs installer again' grep -Fq "wine <$WORK/installer space.exe>" "$TRACE"
check 'successful retry removes marker' test ! -e "$WINEPREFIX/.lightburn-installing"
check 'successful retry stamps version' test -f "$WINEPREFIX/.lightburn-version"
printf '2.1.00\n' > "$WINEPREFIX/.lightburn-version"
: > "$TRACE"
check '2.1.00 launches with default configuration' env -u LB_VERSION -u LB_WIN_SHA256 bash "$WORK/repo/scripts/run.sh"
check '2.1.00 repairs with default configuration' env -u LB_VERSION -u LB_WIN_SHA256 bash "$WORK/repo/scripts/install.sh"
check 'repair preserves 2.1.00 stamp' test "$(< "$WINEPREFIX/.lightburn-version")" = 2.1.00
check 'repair never runs installer' fails_with 1 grep -Fq "wine <$WORK/installer space.exe>" "$TRACE"
export WINEPREFIX="$WORK/delayed registry" DELAY_SYSTEM_REG=1
check 'fresh install succeeds before registry flush' install
check 'mock has not flushed system.reg' test ! -e "$WINEPREFIX/system.reg"
check 'fresh prefix launches before registry flush' bash "$WORK/repo/scripts/run.sh"
check 'fresh prefix maps serial before registry flush' bash "$WORK/repo/scripts/map-serial.sh" /dev/null com1
unset DELAY_SYSTEM_REG
export WINEPREFIX="$WORK/future version" LB_VERSION=2.99.00
: > "$TRACE"
check 'unknown install without checksum fails' fails_with 1 env -u LB_WIN_SHA256 bash "$WORK/repo/scripts/install.sh" "$WORK/installer space.exe" > "$WORK/output" 2>&1
check 'unknown checksum error explains override' grep -q 'LB_WIN_SHA256' "$WORK/output"
check 'unknown checksum failure invokes no tools' test ! -s "$TRACE"
check 'unknown checksum failure creates no prefix' test ! -e "$WINEPREFIX"
check 'empty checksum rejected' fails_with 1 env LB_WIN_SHA256= bash "$WORK/repo/scripts/install.sh" "$WORK/installer space.exe"
check 'empty checksum invokes no tools' test ! -s "$TRACE"
check 'empty checksum creates no prefix' test ! -e "$WINEPREFIX"
for version in '' $'2.99.00\ninjected' $'2.99.00\r' '../2.99.00' '2\99'; do
  check 'unsafe install version rejected' fails_with 1 env LB_VERSION="$version" bash "$WORK/repo/scripts/install.sh" "$WORK/installer space.exe"
done
check 'unsafe versions invoke no tools' test ! -s "$TRACE"
check 'unsafe versions create no prefix' test ! -e "$WINEPREFIX"
check 'future install accepts explicit checksum and LB_WIN_EXE' env LB_WIN_EXE="$WORK/installer space.exe" bash "$WORK/repo/scripts/install.sh"
check 'future install stamps configured version' test "$(< "$WINEPREFIX/.lightburn-version")" = 2.99.00
: > "$TRACE"
check 'future installed app launches with defaults' env -u LB_VERSION -u LB_WIN_SHA256 bash "$WORK/repo/scripts/run.sh"
check 'future installed app repairs with defaults' env -u LB_VERSION -u LB_WIN_SHA256 bash "$WORK/repo/scripts/install.sh"
check 'unknown configured version repairs without checksum' env -u LB_WIN_SHA256 bash "$WORK/repo/scripts/install.sh"
check 'future repair preserves stamp' test "$(< "$WINEPREFIX/.lightburn-version")" = 2.99.00
check 'future repair does not execute installer' fails_with 1 grep -Fq "wine <$WORK/installer space.exe>" "$TRACE"
export LB_VERSION=2.1.04
: > "$TRACE"
check 'future app refuses mismatched explicit installer' fails_with 1 install
check 'future app refuses mismatched LB_WIN_EXE' fails_with 1 env LB_WIN_EXE="$WORK/installer space.exe" bash "$WORK/repo/scripts/install.sh"
touch "$WINEPREFIX/.lightburn-installing"
check 'interrupted install refuses mismatch without installer' fails_with 1 bash "$WORK/repo/scripts/install.sh" > "$WORK/output" 2>&1
check 'interrupted mismatch explains separate prefix' grep -q 'separate WINEPREFIX' "$WORK/output"
rm "$WINEPREFIX/.lightburn-installing" "$WINEPREFIX/drive_c/LightBurn/LightBurn.exe"
check 'missing app refuses mismatch without installer' fails_with 1 bash "$WORK/repo/scripts/install.sh" > "$WORK/output" 2>&1
check 'missing app mismatch explains separate prefix' grep -q 'separate WINEPREFIX' "$WORK/output"
check 'all mismatched installs invoke no tools' test ! -s "$TRACE"
export WINEPREFIX="$WORK/uninitialized"
check 'uninitialized launcher rejected' fails_with 1 bash "$WORK/repo/scripts/run.sh"
mkdir -p "$WINEPREFIX/drive_c/LightBurn"
touch "$WINEPREFIX/system.reg" "$WINEPREFIX/drive_c/LightBurn/LightBurn.exe"
check 'registry and app without dosdevices cannot launch' fails_with 1 bash "$WORK/repo/scripts/run.sh"
check 'registry without dosdevices cannot map' fails_with 1 bash "$WORK/repo/scripts/map-serial.sh" /dev/null com1
rm -r "$WINEPREFIX/drive_c"
mkdir "$WINEPREFIX/dosdevices"
check 'registry and dosdevices without drive_c cannot map' fails_with 1 bash "$WORK/repo/scripts/map-serial.sh" /dev/null com1
check 'uninitialized operations invoke no tools' test ! -s "$TRACE"
export WINEPREFIX="$WORK/partial install"
printf '2.1.04\n' > "$WINEPREFIX/.lightburn-version"
mkfifo "$WORK/ready" "$WORK/block" "$WORK/stopped"
exec 7<> "$WORK/stopped" 8<> "$WORK/ready" 9<> "$WORK/block"
set -m
bash -c 'read -r -t 30 -u 9' &
unrelated_pid=$!
for signal in INT TERM HUP; do
  case "$signal" in INT) status=130 ;; TERM) status=143 ;; HUP) status=129 ;; esac
  SIGNAL_TEST=1 "$WORK/repo/scripts/run.sh" &
  launcher_pid=$!
  mock_pid=
  child_pid=
  check "$signal mock Wine reports readiness" read -r -t 5 -u 8 mock_pid child_pid
  check "$signal can be sent to launcher PID" kill -"$signal" "$launcher_pid"
  if ! read -r -t 2 -u 7; then
    check "$signal tears down Wine without a second signal" false
    kill -TERM "$mock_pid" 2>/dev/null || true
    read -r -t 2 -u 7 || true
  else
    check "$signal tears down Wine without a second signal" true
  fi
  check "$signal launcher exit status" fails_with "$status" wait "$launcher_pid"
  check "$signal leaves no mock application child" fails_with 1 kill -0 "$child_pid"
  check "$signal leaves unrelated process alive" kill -0 "$unrelated_pid"
done
kill -TERM "$unrelated_pid"
wait "$unrelated_pid" 2>/dev/null || true
SIGNAL_TEST=1 RUN_STATUS=37 "$WORK/repo/scripts/run.sh" &
launcher_pid=$!
check 'close mock Wine reports readiness' read -r -t 5 -u 8 mock_pid child_pid
check 'launcher waits while application is open' kill -0 "$launcher_pid"
printf 'close\n' >&9
check 'application close preserves exit status' fails_with 37 wait "$launcher_pid"
check 'close leaves no mock application child' fails_with 1 kill -0 "$child_pid"
set +m
exec 7>&- 8>&- 9>&-
printf '%s passed, %s failed\n' "$passed" "$failed"
[[ $failed == 0 ]]
