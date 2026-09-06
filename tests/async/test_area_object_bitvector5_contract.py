#!/usr/bin/env python3
"""Contract for backward-compatible AFF5 masks in static area objects."""
from _paths import SRC
from pathlib import Path

from contract_text import contains, index

ROOT = Path(__file__).resolve().parents[2]
db = (SRC / "db.c").read_text()
docs = (ROOT / "docs/content/AREA_OBJECT_FORMAT.md").read_text()

loader_start = index(db, "object_template parse_object_template(int nr)")
numeric_start = index(db, "/* *** numeric data *** */", loader_start)
extras_start = index(db, "/* *** extra descriptions *** */", numeric_start)
loader = db[numeric_start:extras_start]

for field in (
	"obj->bitvector = utmp;",
	"obj->bitvector2 = utmp;",
	"obj->bitvector3 = utmp;",
	"obj->bitvector4 = utmp;",
):
	assert contains(loader, field)

marker = index(loader, 'if (!strcmp(chk, "B5"))')
assignment = index(loader, "obj->bitvector5 = utmp;", marker)
assert marker < assignment
assert contains(loader, 'logit(LOG_STATUS, "Object %d has an invalid B5 affect mask."')

# The next section token is read before checking B5. This is what keeps old
# E/A/T/# records aligned while allowing the explicit extension.
token_read = index(loader, 'fscanf(obj_f, " %s \\n", chk)')
assert token_read < marker

assert "B5 <mask>" in docs
assert "Do not add a bare fifth number" in docs

print("area object bitvector5 compatibility contract passed")
