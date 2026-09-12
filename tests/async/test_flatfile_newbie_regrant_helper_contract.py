#!/usr/bin/env python3
"""Keep the current-base helper union compatible with dynamic test callers."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
journey = (ROOT / "tests/async/test_flatfile_combat_journey.py").read_text()
regrant = (ROOT / "tests/async/test_flatfile_newbie_regrant_journey.py").read_text()

signature = journey.split("def create_character(", 1)[1].split("entry, _", 1)[0]
assert 'class_name: str = "w"' in signature
assert "account: str = ACCOUNT" in signature
assert "character: str = CHARACTER" in signature
assert "email: str = EMAIL" in signature

# The helper's defaults are bound when the function is defined.  This caller
# temporarily changes module globals for the observer, so it must pass the
# alternate identity explicitly instead of relying on those defaults.
assert "journey.create_character(observer, expected_room=None)" not in regrant
observer_call = regrant[regrant.index("journey.create_character(\n                            observer") :]
assert 'account="Observeacct"' in observer_call
assert 'character="Valerek"' in observer_call
assert 'email="observeacct@example.invalid"' in observer_call

print("newbie regrant helper/caller compatibility contract passed")
