#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
test_dir="$(mktemp -d "${TMPDIR:-/tmp}/dpf-vst3-time-tests.XXXXXX")"
trap 'rm -rf "$test_dir"' EXIT
"${CXX:-c++}" -std=c++11 -O2 "$root/tests/TimePositionVST3.cpp" -o "$test_dir/vst3-time-tests"
"$test_dir/vst3-time-tests"
