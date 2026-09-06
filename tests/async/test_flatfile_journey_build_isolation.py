#!/usr/bin/env python3
"""Keep the parallel flat-file journeys on distinct build outputs."""

from _paths import ROOT


SOURCES = (
    ("tests/async/test_flatfile_combat_journey.py", "flatfile-combat"),
    ("tests/async/test_flatfile_chaos_new_character_kit.py", "flatfile-combat"),
    ("tests/async/test_account_recovery_journey.py", "flatfile-recovery"),
)

# The prefix is matched WITHOUT its closing bracket: a journey may pass further
# arguments to TemporaryDirectory and wrap the call, which the combat journey
# now does, and the contract is about the prefix rather than the call's shape.
for relative, prefix in SOURCES:
    source = (ROOT / relative).read_text()
    assert f'TemporaryDirectory(prefix=f"{prefix}-{{os.getpid()}}-"' in source, relative
    assert "build_flatfile_server(pathlib.Path(build_tmp))" in source, relative
    assert 'ROOT / "bin/tests/flatfile-combat"' not in source, relative

print("parallel flat-file journey build isolation and cleanup contract passed")
