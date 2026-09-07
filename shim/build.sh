#!/usr/bin/env bash
set -euo pipefail
[[ $# -le 1 ]] || { printf 'Usage: %s [output.dll]\n' "$0" >&2; exit 1; }
SHIM=$(cd -- "$(dirname -- "$0")" && pwd)
output=${1:-$SHIM/winrtcamstub.dll}
if [[ ! ${CC+x} ]]; then
  if command -v x86_64-w64-mingw32-gcc >/dev/null; then
    CC=x86_64-w64-mingw32-gcc
  else
    CC=/opt/llvm-mingw/bin/x86_64-w64-mingw32-gcc
  fi
fi
target=$("$CC" -dumpmachine)
case "$target" in
  x86_64-w64-mingw32|x86_64-w64-windows-gnu) ;;
  *) printf 'Compiler must target x86_64 Windows, got %s\n' "$target" >&2; exit 1 ;;
esac
"$CC" -shared -O2 -s -o "$output" "$SHIM/winrtcamstub.c" -lwindowsapp -luser32 -lkernel32
