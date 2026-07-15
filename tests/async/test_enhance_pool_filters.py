from pathlib import Path

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
assert 'else if (!strcmp(section, "pool_exclude_zone")) section_idx = 2;' in source
assert 'else if (!strcmp(section, "pool_exclude_vnum")) section_idx = 3;' in source
assert 'else if (section_idx == 2 && !strcmp(key, "zone"))' in source
assert 'else if (section_idx == 3 && !strcmp(key, "vnum"))' in source

# Pool filtering is additive to affect filtering and happens before an entry is allocated.
pool_filter = source.index("is_enhance_pool_banned(obj)")
entry_alloc = source.index("malloc(sizeof(struct enhance_index_entry))", pool_filter)
assert pool_filter < entry_alloc
assert "is_enhance_banned(item)" in source

# Zone matching uses the established zone-table/vnum-origin mapping, not an
# item's live location or mutable state.
assert "zone_table[zone].real_bottom" in source
assert "zone_table[zone].number" in source

print("enhance pool filter contract passed")
