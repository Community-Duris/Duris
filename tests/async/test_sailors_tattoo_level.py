from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
ACHIEVEMENTS = (ROOT / "src/world/achievements.c").read_text(encoding="utf-8")


assert "if (lvlachi >= 20)" in ACHIEVEMENTS
assert '"&+BGain level 20"' in ACHIEVEMENTS
assert "if (GET_LEVEL(ch) >= 20 && (!paf || paf->modifier < 20))" in ACHIEVEMENTS
assert ACHIEVEMENTS.count("paf->modifier = 20;") == 1

# The ordinary Sailor's Tattoo path should no longer use level 30 as its
# eligibility threshold or completion marker.
assert "if (lvlachi >= 30)" not in ACHIEVEMENTS
assert "if (GET_LEVEL(ch) >= 30 && (!paf || paf->modifier < 30))" not in ACHIEVEMENTS
