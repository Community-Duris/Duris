#!/usr/bin/env python3
"""Keep the two parallel flat-file journeys on distinct build outputs."""

from _paths import ROOT


SOURCES = (
    (ROOT / "tests/async/test_flatfile_combat_journey.py").read_text(),
    (ROOT / "tests/async/test_flatfile_chaos_new_character_kit.py").read_text(),
)

for source in SOURCES:
    assert 'TemporaryDirectory(prefix=f"flatfile-combat-{os.getpid()}-")' in source
    assert "build_flatfile_server(pathlib.Path(build_tmp))" in source
    assert 'ROOT / "bin/tests/flatfile-combat"' not in source

print("parallel flat-file journey build isolation and cleanup contract passed")
