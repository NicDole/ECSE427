#!/bin/bash

cd /media/sf_ECSE427/Part1/test-cases

echo "=== Testing all cases (ignoring whitespace as per grading) ==="
echo ""

passed=0
failed=0

for test in *.txt; do
    if [[ $test != *_result.txt ]]; then
        echo -n "Testing $test... "
        ../mysh < "$test" > "my_${test%.txt}_output.txt" 2>&1
        
        # Use diff -w to ignore whitespace differences (as per assignment grading)
        if diff -w "my_${test%.txt}_output.txt" "${test%.txt}_result.txt" > /dev/null 2>&1; then
            echo "✓ PASSED"
            ((passed++))
        else
            echo "✗ FAILED"
            ((failed++))
            echo "  Differences (whitespace ignored):"
            diff -w "my_${test%.txt}_output.txt" "${test%.txt}_result.txt" | head -5
        fi
    fi
done

echo ""
echo "=== Summary ==="
echo "Passed: $passed"
echo "Failed: $failed"
echo ""
echo "Note: Tests use 'diff -w' which ignores whitespace differences,"
echo "matching the grading criteria: 'we ignore differences in capitalization and whitespace'"
