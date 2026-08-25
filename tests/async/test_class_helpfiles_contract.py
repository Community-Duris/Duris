"""Source contracts for class help files coverage across Duris help systems."""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
COMMON_C = (ROOT / "src/common.c").read_text()
DEFINES_H = (ROOT / "src/defines.h").read_text()
PARSED_HELP = (ROOT / "help/duris_help_parsed.hlp").read_text()
RAW_HELP = (ROOT / "help/duris_help.hlp").read_text()
HELP_INDEX = (ROOT / "lib/information/help_index").read_text()

# 1. Extract all 30 classes from class_names_table in src/common.c
class_table_match = re.search(
    r"const struct class_names\s+class_names_table\[\]\s*=\s*\{(.*?)\n\};",
    COMMON_C,
    re.DOTALL,
)
assert class_table_match is not None, "class_names_table not found in src/common.c"

class_names = []
for line in class_table_match.group(1).split("\n"):
    m = re.search(r'\{\s*"([^"]+)"', line)
    if m and m.group(1) not in ("None", "NULL"):
        class_names.append(m.group(1))

assert len(class_names) == 30, f"Expected 30 classes, found {len(class_names)}"

# 2. Parse help/duris_help_parsed.hlp
parsed_entries = PARSED_HELP.split("\n#0\n")
parsed_map = {}
for entry in parsed_entries:
    lines = entry.strip().split("\n")
    if len(lines) >= 2:
        title_line = lines[1].strip()
        title = title_line.split(" - Last Edited:")[0].strip()
        parsed_map[title.lower()] = (title, entry)

# 3. Parse lib/information/help_index
index_entries = HELP_INDEX.split("\n#\n")
index_map = {}
for entry in index_entries:
    lines = entry.strip().split("\n")
    if lines:
        title_line = lines[0].strip()
        match = re.match(r'^"([^"]+)"', title_line)
        if match:
            t = match.group(1).strip()
        else:
            t = title_line.split("(")[0].strip()
        if t:
            index_map[t.lower()] = (t, entry)

# 4. Verify contracts for every class
for cls in class_names:
    cls_lower = cls.lower()
    skills_title = f"{cls_lower} skills"
    index_skill_title = f"skill_{cls_lower}"

    # Main class helpfile in parsed help
    assert cls_lower in parsed_map, f"Missing main class helpfile for '{cls}' in duris_help_parsed.hlp"
    _, main_entry = parsed_map[cls_lower]
    main_lower = main_entry.lower()

    # Core sections check (allowing case flexibility on legacy files)
    assert "==see also==" in main_lower, f"Missing '==See also==' in help for '{cls}'"
    assert "==allowed races==" in main_lower or "==allowable races==" in main_lower, f"Missing '==Allowed races==' in help for '{cls}'"
    assert "==innate abilities==" in main_lower, f"Missing '==Innate abilities==' in help for '{cls}'"

    # Skills helpfile in parsed help
    assert skills_title in parsed_map, f"Missing skills helpfile for '{cls}' ('{cls} Skills') in duris_help_parsed.hlp"
    _, skill_entry = parsed_map[skills_title]
    skill_lower = skill_entry.lower()
    assert "==skills==" in skill_lower, f"Missing '==Skills==' in skills help for '{cls}'"
    assert "==spells==" in skill_lower, f"Missing '==Spells==' in skills help for '{cls}'"
    assert "==see also==" in skill_lower, f"Missing '==See also==' in skills help for '{cls}'"

    # Main class and skillset in help_index
    assert cls_lower in index_map or cls.upper() in [t.upper() for t in index_map], f"Missing '{cls}' in lib/information/help_index"
    assert index_skill_title in index_map or f"skill_{cls.upper()}".lower() in index_map, f"Missing 'SKILL_{cls.upper()}' in lib/information/help_index"

    # Verify presence of HELPMARKER in help/duris_help.hlp
    marker_name = cls.replace(" ", "_").replace("-", "_")
    assert f"HELPMARKER_" in RAW_HELP and (f"_{marker_name}'" in RAW_HELP or f"_{cls.replace(' ', '_')}'" in RAW_HELP or f"_{cls.replace('-', '')}'" in RAW_HELP or f"_{cls}'" in RAW_HELP), f"Missing HELPMARKER for '{cls}' in duris_help.hlp"

print("All 30 class help files and skills contracts passed successfully!")
