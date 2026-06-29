#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
python3 test_dirty_flush_retry.py
