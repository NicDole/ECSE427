#!/bin/bash

# Run all T_*.txt test cases in batch mode the same way
# the graders do: fresh make clean; make mysh for each test,
# then ./mysh < testfile.txt, and compare output against
# *_result.txt / *_result2.txt ignoring whitespace and case.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Testcases live in src/test-cases. We must run mysh from that
# directory (so scripts like short_program, P_prog1, etc. are
# in the current working directory), exactly like the graders.
TEST_DIR="$SCRIPT_DIR/test-cases"
cd "$TEST_DIR"

TMP_OUT="$(mktemp)"

overall_pass=0
overall_fail=0
overall_skip=0

for t in T_*.txt; do
  [ -e "$t" ] || continue

  base="$(basename "$t" .txt)"
  expected1="${base}_result.txt"
  expected2="${base}_result2.txt"

  echo "===== $base ====="

  # Fresh build like graders
  make -C "$SCRIPT_DIR" clean >/dev/null 2>&1 || true
  if ! make -C "$SCRIPT_DIR" mysh >/dev/null 2>&1; then
    echo "BUILD FAIL for $base"
    overall_fail=$((overall_fail + 1))
    continue
  fi

  # Run in batch mode
  ../mysh < "$t" > "$TMP_OUT"

  if [ ! -f "$expected1" ] && [ ! -f "$expected2" ]; then
    echo "SKIP (no expected output for $base)"
    overall_skip=$((overall_skip + 1))
    continue
  fi

  pass=0

  if [ -f "$expected1" ]; then
    if diff -iw "$expected1" "$TMP_OUT" >/dev/null 2>&1; then
      pass=1
    fi
  fi

  if [ "$pass" -eq 0 ] && [ -f "$expected2" ]; then
    if diff -iw "$expected2" "$TMP_OUT" >/dev/null 2>&1; then
      pass=1
    fi
  fi

  if [ "$pass" -eq 1 ]; then
    echo "PASS $base"
    overall_pass=$((overall_pass + 1))
  else
    echo "FAIL $base"
    overall_fail=$((overall_fail + 1))
    # Show unified diff against primary expected if it exists
    if [ -f "$expected1" ]; then
      diff -u "$expected1" "$TMP_OUT" || true
    else
      diff -u "$expected2" "$TMP_OUT" || true
    fi
  fi

  echo
done

rm -f "$TMP_OUT"

echo "Summary: pass=$overall_pass fail=$overall_fail skip=$overall_skip"

exit 0

