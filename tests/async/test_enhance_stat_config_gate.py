from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/enhance.c").read_text()
header = (ROOT / "src/enhance.h").read_text()
config = (ROOT / "lib/enhance.cfg").read_text()

# The stat-enhance path must fail closed unless explicitly enabled in config.
assert re.search(r"int\s+enhance_stat_enabled\s*=\s*0;", source)
assert "extern int enhance_stat_enabled;" in header
assert "else if (section_idx == 1)" in source
assert 'if (!strcmp(key, "enhance_stat.enabled"))' in source
assert "enhance_stat_enabled = atoi(val) ? 1 : 0;" in source
assert "enhance_stat.enabled=1" in config

# A stat word must be allowed to fall through to legacy material handling when
# the donor-free stat lane is disabled: e.g. `enhance bracer damroll` consumes
# a carried damroll essence instead of being rejected by the config gate.
stat_path = source.index("if (stat_idx != -1 && enhance_stat_enabled)")
source_lookup = source.index("get_obj_in_list_vis(ch, first, ch->carrying)", stat_path)
charge = source.index("SUB_MONEY(ch, cost, 0);", stat_path)
mutation = source.index("source->affected[source_slot].modifier = new_mod;", stat_path)
assert "if (stat_idx != -1 && enhance_stat_enabled)" in source
assert "if (!enhance_stat_enabled)" not in source[stat_path:source_lookup]
legacy_start = source.index("/* Original 2-arg enhance */")
legacy_material_lookup = source.index("get_obj_in_list_vis(ch, second, ch->carrying)", legacy_start)
essence_dispatch = source.index("modenhance(ch, source, material);", legacy_start)
assert stat_path < legacy_start < legacy_material_lookup < essence_dispatch

# The donor-free lane still protects its lookup, money, and mutation while enabled.
enabled_stat_path = source.index("if (stat_idx != -1 && enhance_stat_enabled)")
assert enabled_stat_path < source_lookup < charge < mutation

# With the lane disabled, the non-stat command paths retain their pre-stat-enhance text.
assert "And which object is the enhancement object?" in source
assert "Syntax: enhance <source item you want to upgrade> <upgrade material item>" in source
help_start = source.index("static void show_enhance_help")
help_gate = source.index("if (!enhance_stat_enabled)", help_start)
help_legacy = source.index("And which object is the enhancement object?", help_start)
help_details = source.index("Superior improvements available", help_start)
assert help_gate < help_legacy < help_details

print("enhance stat config gate contract passed")
