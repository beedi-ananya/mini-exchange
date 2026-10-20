#!/usr/bin/env bash
# Builds normally and with sanitizers, then runs every test program.
set -euo pipefail
cd "$(dirname "$0")/.."

run_all() {
    local dir="$1"
    local count=0
    for t in "$dir"/test_*; do
        if [ -f "$t" ] && [ -x "$t" ]; then
            if out=$("$t" 2>&1); then
                echo "-- $t: $(echo "$out" | tail -n 1)"
                count=$((count + 1))
            else
                echo "FAILED: $t"
                echo "$out" | tail -n 8
                exit 1
            fi
        fi
    done
    echo "   ($count test programs passed in $dir)"
}

echo "== normal build =="
cmake -S . -B build > /dev/null
cmake --build build > /dev/null
run_all build

echo "== sanitizer build (address + undefined) =="
cmake -S . -B build-asan -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g" > /dev/null
cmake --build build-asan > /dev/null
run_all build-asan

echo "ALL CHECKS PASSED"
