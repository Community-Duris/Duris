#!/usr/bin/env python3
"""Contract for the opt-in NPC reset material fallback."""
from _paths import SRC
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
db = (SRC / "db.c").read_text()
enhance = (SRC / "enhance.c").read_text()
header = (SRC / "enhance.h").read_text()
config = (ROOT / "lib/enhance.cfg").read_text()

# The feature is a separate fail-closed gate in the enhancement-stat config.
assert "enhance_stat.npc_material_fallback.enabled=0" in config
assert "extern int enhance_stat_npc_material_fallback_enabled;" in header
assert "int enhance_stat_npc_material_fallback_enabled = 0;" in enhance
assert "enhance_stat_npc_material_fallback_enabled = 0;" in enhance
assert '"enhance_stat.npc_material_fallback.enabled"' in enhance

# A failed NPC G/E load produces exactly one highest-quality material based on
# the missing template's material family.  The enhancement module owns policy;
# db.c only reports its reset-specific chance failure.  The material stays in
# inventory even for an E command and does not replace an equipment slot.
assert "enhance_on_npc_item_reset_skipped(P_char mob, P_obj missing_item)" in header
fallback = enhance[enhance.index("void enhance_on_npc_item_reset_skipped"):]
assert "enhance_stat_npc_material_fallback_enabled" in fallback
assert "get_matstart(missing_item) + 4" in fallback
assert "read_object(high_vnum, VIRTUAL)" in fallback
assert "obj_to_char(material, mob)" in fallback
assert "load_npc_missing_item_material" not in db

# Only the two chance-rejected NPC item commands invoke it. Successful loads,
# artifact suppression, item limits, and non-NPC reset commands are unchanged.
assert db.count("enhance_on_npc_item_reset_skipped(mob, obj);") == 2
for marker in ("case 'G': /* obj_to_char */", "case 'E': /* object to equipment list */"):
    block = db[db.index(marker):]
    rejected = block.index("!ITEM_LOAD_CHECK(obj, ival, ZCMD.arg4)")
    window = block[rejected:rejected + 500]
    fallback_call = window.index("enhance_on_npc_item_reset_skipped(mob, obj);")
    extract = window.index("extract_obj(obj);")
    assert fallback_call < extract

print("NPC missing-item material fallback contract passed")
