from pathlib import Path
from contract_text import contains

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/enhance.c").read_text()
config = (ROOT / "lib/enhance.cfg").read_text()

# Administrators can repeat a numeric zone or vnum line to exclude any number
# of output templates from the legacy random-enhancement pool.
assert "[pool_exclude_zone]" in config
assert "zone=<virtual zone number>" in config
assert "[pool_exclude_vnum]" in config
assert "vnum=<object vnum>" in config

# Both config sections must be recognized and parsed into their own filters.
assert contains(source, 'else if (!strcmp(section, "pool_exclude_zone")) section_idx = 2;')
assert contains(source, 'else if (!strcmp(section, "pool_exclude_vnum")) section_idx = 3;')
assert contains(source, 'else if (section_idx == 2 && !strcmp(key, "zone"))')
assert contains(source, 'else if (section_idx == 3 && !strcmp(key, "vnum"))')

# Pool filtering is additive to affect filtering and happens before an entry is allocated.
pool_filter = source.index("is_enhance_pool_banned(obj)")
entry_alloc = source.index("malloc(sizeof(struct enhance_index_entry))", pool_filter)
assert pool_filter < entry_alloc
assert contains(source, "is_enhance_banned(item)")

# Zone matching uses the established zone-table/vnum-origin mapping, not an
# item's live location or mutable state.
assert contains(source, "zone_table[zone].real_bottom")
assert contains(source, "zone_table[zone].number")

print("enhance pool filter contract passed")
