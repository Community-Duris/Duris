#!/usr/bin/env python3
"""Keep the two parallel flat-file journeys on distinct build outputs."""

from _paths import ROOT


SOURCE = (ROOT / "tests/async/test_flatfile_combat_journey.py").read_text()

assert 'f"flatfile-combat-{os.getpid()}"' in SOURCE
assert 'ROOT / "bin/tests/flatfile-combat"' not in SOURCE

print("parallel flat-file journey build isolation contract passed")
