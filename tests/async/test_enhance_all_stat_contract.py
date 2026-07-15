from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/enhance.c").read_text()
config = (ROOT / "lib/enhance.cfg").read_text()

# The enabled lane is driven from `enhance <item>` and plans every eligible
# positive stat as one atomic enhancement, rather than taking a stat argument.
assert "static bool build_superior_enhancement_plan" in source
assert "static bool perform_superior_enhancement" in source
assert "if (!*second && enhance_stat_enabled)" in source
assert "if (stat_idx != -1 && enhance_stat_enabled)" not in source

# A plan aggregates template-derived tribute across all upgradeable stats and
# validates the entire inventory before mutating the item or consuming anything.
assert "superior_plan_add_material" in source
assert "vnum_in_inv(ch, plan->materials[i].vnum) < plan->materials[i].count" in source
assert "source->affected[plan->slots[i]].modifier++" in source
assert "vnum_from_inv(ch, plan->materials[i].vnum, plan->materials[i].count)" in source
assert source.index("vnum_in_inv(ch, plan->materials[i].vnum)") < source.index("source->affected[plan->slots[i]].modifier++")

# The `enhance <item>` preview is intentionally aggregate-only: no stat names,
# values, caps, or stat-specific syntax are revealed.
preview_start = source.index("static void show_superior_requirements")
preview_end = source.index("void do_enhance", preview_start)
preview = source[preview_start:preview_end]
assert "display_name" not in preview
assert "enhance <item> <stat>" not in preview
assert r"enhance <item>\r\n" in preview
assert "enhancement" in preview.lower()

assert "enhance <source> <stat>" not in config

print("all-stat enhancement contract passed")
