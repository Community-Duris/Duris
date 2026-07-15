#!/usr/bin/env python3
"""Contract for the read-only static material-composition report."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
report = (root / "src/material_rarity.c").read_text()
header = (root / "src/material_rarity.h").read_text()
comm = (root / "src/comm.c").read_text()
db = (root / "src/db.c").read_text()

# The report must use the game's authoritative runtime template/item-value APIs,
# not a second hand parser or a guessed recipe formula.
assert "read_object" in report
assert "itemvalue" in report
assert "get_matstart" in report
assert "extract_obj" in report

# The executable mode is offline/read-only: boot the object database, write the
# report, and exit before listening for player connections.
assert "--material-rarity-report" in comm
assert "boot_material_rarity_objects" in comm
assert "ne_init_event_pool();" in db
assert "write_material_rarity_report" in comm
assert comm.index("if (material_rarity_report_mode)") < comm.index("initialize_mysql")
assert "void write_material_rarity_report" in header

# Preserve source data for auditability and make the percentage denominator
# explicit. Unknown/unmapped material families must be reported, not silently
# folded into a default material family.
assert "material-rarity-summary.csv" in report
assert "material-rarity-recipes.csv" in report
assert "material-rarity-exclusions.csv" in report
assert "unmapped-material-family" in report
assert "total_material_units" in report
assert "share_percent" in report

print("material rarity report contract passed")
