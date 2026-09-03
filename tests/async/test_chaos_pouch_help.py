#!/usr/bin/env python3
"""Verify the player-facing Chaos craft pouch help source and registrations."""

from __future__ import annotations

from _paths import ROOT, source


HELP = ROOT / "lib/information/helpchaospouch"
content = HELP.read_text(encoding="utf-8")
importer = (ROOT / "scripts/import_help_to_prod.sh").read_text(encoding="utf-8")
catalog = source("flatfile_help_catalog.c").read_text(encoding="utf-8")

assert len(content) >= 4000, "Chaos craft pouch help page is not complete"
for section in (
    "GETTING THE POUCH",
    "LOOKING AT THE SCOREBOARD",
    "COLLECTING MATERIALS",
    "CRAFTING AND FORGING",
    "ENHANCEMENT",
    "ENCRUSTING",
    "RESTRICTIONS AND TROUBLESHOOTING",
):
    assert section in content, f"missing help section: {section}"
for command in (
    "look in pouch",
    "examine pouch",
    "put <material> pouch",
    "put all pouch",
    "put all.<keyword> pouch",
    "enhance <item> pouch",
    "encrust <item> <jewel-vnum>",
):
    assert command in content, f"missing pouch command documentation: {command}"
for catalog_range in ("400000 through 400209", "400291 through 400299"):
    assert catalog_range in content, f"missing pouch catalog range: {catalog_range}"

assert '["helpchaospouch"]="chaos craft pouch"' in importer
assert '{ "lib/information/helpchaospouch", "chaos craft pouch" }' in catalog
assert '{ "lib/information/helpchaospouch", "chaos pouch" }' in catalog
print("Chaos craft pouch help source and registrations passed")
