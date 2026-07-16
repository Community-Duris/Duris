#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC="$ROOT/src"
TEST="$ROOT/tests/async/test_signal_handlers.cpp"
OUT="${TMPDIR:-/tmp}/test_signal_handlers"

g++ -std=c++20 -I"$SRC" "$TEST" -o "$OUT"
"$OUT"
