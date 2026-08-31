"""Source contracts ensuring complete helpfiles exist for all 37 player races."""

from _paths import SRC
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HELP_DIR = ROOT / "help"
LIB_DIR = ROOT / "lib"

# Load source files
defines = (SRC / "defines.h").read_text(encoding="utf-8", errors="ignore")
common = (SRC / "common.c").read_text(encoding="utf-8", errors="ignore")
props_text = (LIB_DIR / "duris.properties").read_text(encoding="utf-8", errors="ignore")
parsed_hlp = (HELP_DIR / "duris_help_parsed.hlp").read_text(encoding="utf-8", errors="ignore")
duris_hlp = (HELP_DIR / "duris_help.hlp").read_text(encoding="utf-8", errors="ignore")

# Extract race_names_table
rnt_match = re.search(r"race_names_table\[.*?\]\s*=\s*\{(.*?)\n\};", common, re.S)
assert rnt_match, "race_names_table not found in common.c"

player_races = []
for line in rnt_match.group(1).strip().splitlines():
    line = line.strip()
    if not line or line.startswith("//"):
        continue
    m = re.search(r'\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\}', line)
    if m:
        player_races.append((m.group(1), m.group(2), m.group(3), m.group(4)))

# We have 37 player races (indices 1 to 37)
assert len(player_races) >= 38, f"Expected at least 38 entries in race_names_table, got {len(player_races)}"
races_to_test = player_races[1:38]
assert len(races_to_test) == 37, f"Expected 37 player races, got {len(races_to_test)}"

# Parse entries from parsed.hlp
parsed_entries = {}
for block in parsed_hlp.split("\n#0\n"):
    lines = block.strip().splitlines()
    if len(lines) >= 2:
        title_line = lines[1].strip()
        title = title_line.split(" - Last Edited:")[0].strip()
        parsed_entries[title.lower()] = block.strip()

# Parse entries from duris_help.hlp
duris_entries = {}
for block in re.split(r"You say 'HELPMARKER_\d+_[^']+'", duris_hlp):
    lines = block.strip().splitlines()
    for l in lines[:5]:
        m = re.search(r"([A-Za-z0-9 -]+)\s+-\s+Last Edited:", l)
        if m:
            t = m.group(1).strip()
            duris_entries[t.lower()] = block.strip()
            break

required_universal_sections = [
    "==See also==",
    "==Class list==",
    "==Racial Statistics==",
    "==Racial Traits==",
    "==Innate abilities=="
]

newly_added_races = {
    "troll", "half-elf", "illithid", "thri-kreen", "vampire",
    "death knight", "shadow beast", "storm giant", "wight", "orog",
    "githzerai", "drider", "kobold", "planetbound illithid", "wood elf",
    "firbolg", "tiefling"
}

print(f"Verifying helpfiles for all {len(races_to_test)} player races...")

for normal, nospaces, ansi, code in races_to_test:
    key = normal.lower()
    
    # 1. Must exist in parsed.hlp
    assert key in parsed_entries, f"Missing helpfile in duris_help_parsed.hlp for race '{normal}'"
    p_content = parsed_entries[key]
    
    # 2. Must exist in duris_help.hlp
    assert key in duris_entries, f"Missing helpfile in duris_help.hlp for race '{normal}'"
    
    # 3. Must have Last Edited header
    assert " - Last Edited:" in p_content, f"Helpfile for '{normal}' missing '- Last Edited:' header"
    
    # 4. Must have universal sections
    for sec in required_universal_sections:
        assert sec in p_content, f"Helpfile for '{normal}' missing section '{sec}'"
    
    # 5. Newly added races must have detailed Strengths and Weakness sections
    if key in newly_added_races:
        assert "==Strengths==" in p_content, f"Helpfile for '{normal}' missing ==Strengths=="
        assert ("==Weakness==" in p_content or "==Weaknesses==" in p_content), (
            f"Helpfile for '{normal}' missing ==Weakness=="
        )
    
    # 6. Must have stats populated
    for stat in ["Strength", "Agility", "Dexterity", "Constitution", "Power", "Intelligence", "Wisdom", "Charisma", "Luck", "Karma"]:
        assert f"{stat}" in p_content, f"Helpfile for '{normal}' missing stat '{stat}'"
    
    # 7. Must have combat and spell pulse
    assert "Combat Pulse :" in p_content, f"Helpfile for '{normal}' missing Combat Pulse"
    assert "Spell Pulse  :" in p_content, f"Helpfile for '{normal}' missing Spell Pulse"

print(f"All {len(races_to_test)} player race helpfiles verified successfully!")
