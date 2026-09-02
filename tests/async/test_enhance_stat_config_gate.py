from _paths import SRC
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
source = (SRC / "enhance.c").read_text()
header = (SRC / "enhance.h").read_text()
config = (ROOT / "lib/enhance.cfg").read_text()

# The donor-free superior lane fails closed unless explicitly enabled in config.
assert re.search(r"int\s+enhance_stat_enabled\s*=\s*0;", source)
assert "extern int enhance_stat_enabled;" in header
assert "else if (section_idx == 1)" in source
assert 'if (!strcmp(key, "enhance_stat.enabled"))' in source
assert "enhance_stat_enabled = atoi(val) ? 1 : 0;" in source
assert "enhance_stat.enabled=1" in config

# The enabled no-material path is isolated, so all two-argument commands retain
# legacy handling. In particular, `enhance bracer damroll` reaches the essence
# dispatcher whether the gate is enabled or disabled.
enabled_path = source.index("if (!*second && enhance_stat_enabled)")
legacy_start = source.index("/* Original 2-arg enhance */")
pouch_lookup = source.index("pouch = chaos_material_pouch_find(ch);", legacy_start)
pouch_assign = source.index("material = pouch;", legacy_start)
legacy_material_lookup = source.index("material = get_obj_in_list_vis(ch, second, ch->carrying);", legacy_start)
missing_material = source.index("if (!material)", legacy_start)
essence_dispatch = source.index("modenhance(ch, source, material);", legacy_start)
assert enabled_path < legacy_start < pouch_lookup < pouch_assign < legacy_material_lookup < missing_material < essence_dispatch
assert "if (stat_idx != -1 && enhance_stat_enabled)" not in source

# A named Chaos pouch is a virtual legacy donor: resolve it through nested/belt
# ownership, use the source's wear flags/value, and never extract the pouch.
assert "chaos_material_pouch_find(ch)" in source
assert "chaos_material_pouch_is_active(material)" in source
assert "if (!pouch_material && itemvalue(material) < minval)" in source
assert "if (!pouch_material)" in source
assert "chaos_material_pouch_is_active(pouch)" in source
legacy_success = source.index("obj_to_char(robj, ch);", source.index("void enhance"))
pouch_cleanup_guard = source.index("if (!pouch_material)", legacy_success)
physical_donor_extract = source.index("obj_from_char(material);", legacy_success)
assert pouch_cleanup_guard < physical_donor_extract

# With the lane disabled, `enhance <item>` retains its pre-feature prompt.
assert "And which object is the enhancement object?" in source
assert "Syntax: enhance <source item you want to upgrade> <upgrade material item>" in source

# act() supplies the line ending; embedding another one creates a blank line
# before the final item value. Legacy luck banners remain conditional and colored.
assert 'Your enhancement is a success! You now have &n$p&+B!\\r\\n' not in source
assert '&+YYou feel &+MEXTREMELY Lucky&+Y!\\r\\n' in source
assert '&+YYou feel &+MVery Lucky&+Y!\\r\\n' in source
assert '&+YYou feel &+MLucky&+Y!\\r\\n' in source

print("enhance stat config gate contract passed")
