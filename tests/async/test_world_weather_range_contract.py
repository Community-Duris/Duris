"""Every zone weather index must stay inside the table it selects.

`weather_setup()` in db.c reads twelve integers per zone from
`world.weather` and uses them directly as subscripts into the local
`winds`, `precip`, `humid`, and `temps` tables.  An out-of-range value is
clamped by ARR_GET, which silently substitutes the wrong climate and
prints an "ARRAY ... index N >= M" line on every boot.  Derive the table
sizes from db.c so the data and the tables cannot drift apart again.
"""

from __future__ import annotations

import re
from pathlib import Path

from _paths import SRC

ROOT = Path(__file__).resolve().parents[2]

db_c = (SRC / "db.c").read_text(encoding="utf-8")


def table_size(name: str) -> int:
    match = re.search(
        r"const\s+signed\s+char\s+" + name + r"\s*\[\s*(\d+)\s*\]", db_c
    )
    assert match, f"{name}[] declaration not found in db.c"
    return int(match.group(1))


winds = table_size("winds")
precip = table_size("precip")
humid = table_size("humid")
temps = table_size("temps")

# db.c indexes humid[] with the season_precip value, so the precip column is
# bounded by the smaller of the two tables.
limits = (winds, min(precip, humid), temps)

# weather_setup() loops `for (zon = 0; zon <= 99; zon++)`.
assert "for (zon = 0; zon <= 99; zon++)" in db_c, "zone loop bound changed"
ZONES = 100
SEASONS = 4

for rel in ("areas/world.weather", "areas_mini/world.weather"):
    path = ROOT / rel
    lines = path.read_text(encoding="utf-8").split("\n")
    rows = [line for line in lines if line.strip()]
    assert len(rows) == ZONES, f"{rel}: expected {ZONES} zone rows, found {len(rows)}"

    for row_no, row in enumerate(rows, start=1):
        values = row.split()
        assert len(values) == SEASONS * 3, (
            f"{rel}:{row_no}: expected {SEASONS * 3} values, found {len(values)}"
        )
        for season in range(SEASONS):
            for offset, size in enumerate(limits):
                value = int(values[season * 3 + offset])
                kind = ("wind", "precip", "temp")[offset]
                assert 0 <= value < size, (
                    f"{rel}:{row_no}: season {season} {kind} index {value} "
                    f"outside 0..{size - 1}"
                )
