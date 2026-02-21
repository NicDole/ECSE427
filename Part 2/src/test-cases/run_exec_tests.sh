#!/bin/bash
# Run 1.2.2 exec tests from test-cases directory (so P_short, P_prog1, etc. are in cwd).
# Usage: cd test-cases && ./run_exec_tests.sh

set -e
MYSH="../mysh"
TESTS="T_exec_single T_exec_two T_exec_invalid_policy T_exec_usage_few T_exec_usage_many T_exec_duplicate T_exec_notfound T_exec_policies T_FCFS"

for t in $TESTS; do
  if [ ! -f "${t}.txt" ]; then
    echo "SKIP ${t} (no ${t}.txt)"
    continue
  fi
  if [ ! -f "${t}_result.txt" ]; then
    echo "SKIP ${t} (no ${t}_result.txt)"
    continue
  fi
  out=$(mktemp)
  $MYSH < "${t}.txt" 2>/dev/null > "$out" || true
  if diff -q "$out" "${t}_result.txt" > /dev/null 2>&1; then
    echo "PASS ${t}"
  else
    echo "FAIL ${t}"
    diff -u "${t}_result.txt" "$out" || true
  fi
  rm -f "$out"
done
