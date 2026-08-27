#!/usr/bin/env python3
"""The launcher must not mistake a sibling checkout's service for this tree."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / "scripts/start_mud.sh").read_text()

assert "--property=WorkingDirectory" in SOURCE
assert '"$(realpath -m "$SERVICE_ROOT")" == "$ROOT"' in SOURCE
assert 'cycle_mud.sh "$@"' in SOURCE
assert "[[ $# -eq 0 ]]" in SOURCE

print("worktree-aware game launcher contract: ok")
