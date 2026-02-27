#!/bin/bash
# Run all tests in this directory.
# A "test" is any input `*.txt` file that has one or more matching expected output files:
#   <name>_result*.txt
#
# You can run this script from anywhere; it will `cd` into `test-cases/` automatically.

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

MYSH="../mysh"

shopt -s nullglob

pass=0
fail=0
skip=0
total=0

for input in *.txt; do
  case "$input" in
    *_result*.txt) continue ;;
    *README*.txt) continue ;;
  esac

  t="${input%.txt}"
  expected_files=( "${t}_result"*.txt )

  if [ "${#expected_files[@]}" -eq 0 ]; then
    echo "SKIP ${t} (no ${t}_result*.txt)"
    skip=$((skip + 1))
    continue
  fi

  total=$((total + 1))

  out="$(mktemp)"
  "$MYSH" < "$input" 2>/dev/null > "$out" || true

  matched=0
  for expected in "${expected_files[@]}"; do
    if diff -q "$out" "$expected" > /dev/null 2>&1; then
      matched=1
      break
    fi
  done

  if [ "$matched" -eq 1 ]; then
    echo "PASS ${t}"
    pass=$((pass + 1))
  else
    echo "FAIL ${t}"
    diff -u "${expected_files[0]}" "$out" || true
    fail=$((fail + 1))
  fi

  rm -f "$out"
done

echo
echo "Summary: total=${total} pass=${pass} fail=${fail} skip=${skip}"

if [ "$fail" -ne 0 ]; then
  exit 1
fi
