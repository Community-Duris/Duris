#!/usr/bin/env python3
"""Manual-only C14 disposable SQL journey (not automatic test discovery)."""
from __future__ import annotations

import subprocess
import sys

from pa_sync_item_state_fixture import run_sync_item_state_fixture


def main() -> int:
    try:
        print(run_sync_item_state_fixture(), flush=True)
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as exc:
        print(f"C14 fixture blocked/failed: {exc}", file=sys.stderr, flush=True)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
