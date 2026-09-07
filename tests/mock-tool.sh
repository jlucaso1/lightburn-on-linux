#!/usr/bin/env bash
set -euo pipefail
tool=${0##*/}
# shellcheck disable=SC2129
printf '%s' "$tool" >> "$TRACE"
printf ' <%s>' "$@" >> "$TRACE"
printf '\n' >> "$TRACE"
case "$tool" in
  xvfb-run)
    [[ $1 == -a ]] || exit 90
    [[ ${FAIL:-} != xvfb ]] || exit 35
    shift
    export DISPLAY=:123
    exec "$@"
    ;;
  wineboot)
    [[ ${FAIL:-} != wineboot ]] || exit 31
    mkdir -p "$WINEPREFIX/drive_c/windows/system32" "$WINEPREFIX/dosdevices"
    if [[ ${DELAY_SYSTEM_REG:-} != 1 ]]; then
      touch "$WINEPREFIX/system.reg"
    fi
    ;;
  wine)
    if [[ $1 == reg ]]; then
      [[ ${FAIL:-} != reg ]] || exit 32
    elif [[ $1 == --version ]]; then
      printf 'wine-mock\n'
    elif [[ $1 == 'C:\LightBurn\LightBurn.exe' || $1 == 'C:\LightBurn\start-lightburn.exe' ]]; then
      if [[ $1 == 'C:\LightBurn\start-lightburn.exe' ]]; then
        printf 'service starting\n' >> "$TRACE"
        [[ ${FAIL:-} != service ]] || exit 38
        if [[ ${SIGNAL_TEST:-} != startup ]]; then
          if [[ -e $WINEPREFIX/.mock-winmgmt-running ]]; then
            printf 'service already running\n' >> "$TRACE"
          fi
          touch "$WINEPREFIX/.mock-winmgmt-running"
          printf 'service running\napplication launched\n' >> "$TRACE"
        fi
      fi
      if [[ ${SIGNAL_TEST:-} == 1 || ${SIGNAL_TEST:-} == startup ]]; then
        trap ':' INT
        bash -c 'read -r -t 10 -u 9' &
        child=$!
        trap 'kill -TERM "$child" 2>/dev/null || true; wait "$child" 2>/dev/null || true; printf "received TERM\n"; printf "stopped\n" >&7; exit 143' TERM
        printf '%s %s\n' "$$" "$child" >&8
        wait "$child"
        exit "${RUN_STATUS:-0}"
      fi
      printf 'launcher stdout\n'
      printf 'launcher stderr\n' >&2
      exit "${RUN_STATUS:-0}"
    else
      [[ ${FAIL:-} != installer ]] || exit 33
      mkdir -p "$WINEPREFIX/drive_c/LightBurn"
      touch "$WINEPREFIX/drive_c/LightBurn/LightBurn.exe"
      [[ ${FAIL:-} != partial-installer ]] || exit 36
    fi
    ;;
  compiler|x86_64-w64-mingw32-gcc)
    if [[ $1 == -dumpmachine ]]; then
      printf '%s\n' "${TARGET:-x86_64-w64-mingw32}"
      exit 0
    fi
    [[ ${FAIL:-} != compiler ]] || exit 34
    if [[ ${FAIL:-} == launcher-compiler && $1 != -shared ]]; then exit 39; fi
    while [[ $# -gt 0 ]]; do
      if [[ $1 == -o ]]; then
        printf 'mock DLL\n' > "$2"
        exit 0
      fi
      shift
    done
    exit 91
    ;;
  *) exit 92 ;;
esac
