#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
cp "$ROOT/lib/artifacts/catalog.json" "$TMP/catalog.json"
g++ -std=c++20 -Wall -Wextra -Werror -I"$ROOT/src" \
  "$ROOT/tests/async/artifact_control_harness.cpp" \
  "$ROOT/src/artifact/artifact_control.c" -lcjson -o "$TMP/artifact_control_harness"
"$TMP/artifact_control_harness" "$TMP/catalog.json"
echo "artifact control C++ model and draft/publish harness: ok"
