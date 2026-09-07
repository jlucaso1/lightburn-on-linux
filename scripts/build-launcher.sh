#!/usr/bin/env bash
set -euo pipefail
[[ $# == 1 ]] || { printf 'Usage: %s output.exe\n' "$0" >&2; exit 1; }
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
exec "$CC" -Wall -Wextra -Werror -O2 -o "$1" "$(dirname -- "$0")/start-lightburn.c" -ladvapi32
