#!/usr/bin/env python3
"""Source-contract tests for the master spellbook (object vnum 7).

The scribed-spell list on a spellbook is a raw bitmap stored in an extra
description, so it cannot be carried by the text object file -- it has embedded
NULs.  The prototype therefore ships empty and is filled in code at load time.
These checks pin down both halves of that arrangement, and the numeric class
mask baked into the area file.

Verifies:
1. MASTER_SPELLBOOK_VNUM defined in defines.h.
2. BOOK_CLASSES names every spellbook class and IS_BOOK_CLASS derives from it.
3. FillMasterSpellBook implemented in memorize.c and declared in prototypes.h.
4. The fill mirrors prac_all_spells()'s notion of a scribable spell.
5. AddSpellToSpellBook tolerates a NULL owner so a prototype can be filled.
6. read_object() fills the book for that vnum and no other.
7. The area prototype exists, is a spellbook, and carries the right values.
8. The vnum is claimed exactly once across the area sources.
"""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"

BOOK_CLASS_NAMES = ("SORCERER", "CONJURER", "NECROMANCER", "ILLUSIONIST",
                    "BARD", "SUMMONER", "REAVER", "THEURGIST")
TONGUE_MAGIC = 27


def check(name, ok):
    print(("OK: " if ok else "FAIL: ") + name)
    return bool(ok)


all_ok = True

defines_h = (SRC / "defines.h").read_text(encoding="utf-8", errors="replace")
utils_h = (SRC / "utils.h").read_text(encoding="utf-8", errors="replace")
memorize_c = (SRC / "memorize.c").read_text(encoding="utf-8", errors="replace")
db_c = (SRC / "db.c").read_text(encoding="utf-8", errors="replace")
proto_h = (SRC / "prototypes.h").read_text(encoding="utf-8", errors="replace")

# 1. Vnum define
m_vnum = re.search(r"#define\s+MASTER_SPELLBOOK_VNUM\s+(\d+)", defines_h)
all_ok &= check("MASTER_SPELLBOOK_VNUM defined in defines.h", m_vnum is not None)
vnum = int(m_vnum.group(1)) if m_vnum else None

# 2. Class mask
m_mask = re.search(r"#define\s+BOOK_CLASSES\s+(.*?)\n#define\s+IS_BOOK_CLASS", utils_h, re.S)
all_ok &= check("BOOK_CLASSES defined in utils.h", m_mask is not None)
if m_mask:
    body = m_mask.group(1)
    for name in BOOK_CLASS_NAMES:
        all_ok &= check(f"BOOK_CLASSES includes CLASS_{name}",
                        re.search(r"\bCLASS_" + name + r"\b", body) is not None)
all_ok &= check("IS_BOOK_CLASS derives from BOOK_CLASSES",
                "#define IS_BOOK_CLASS(cls) ((cls) & BOOK_CLASSES)" in utils_h)

# Resolve the mask numerically from the class bits, so the value baked into the
# area file cannot drift away from the code.
bits = dict(re.findall(r"#define\s+BIT_(\d+)\s+(\d+)U", defines_h))
mask = 0
for name in BOOK_CLASS_NAMES:
    m_cls = re.search(r"#define\s+CLASS_" + name + r"\s+BIT_(\d+)", defines_h)
    if not m_cls or m_cls.group(1) not in bits:
        all_ok &= check(f"CLASS_{name} resolves to a BIT_n value", False)
        continue
    mask |= int(bits[m_cls.group(1)])

# 3. Fill function
fill = re.search(r"int FillMasterSpellBook\(P_obj obj\)\s*\{(.*?)\n\}", memorize_c, re.S)
all_ok &= check("FillMasterSpellBook implemented in memorize.c", fill is not None)
all_ok &= check("FillMasterSpellBook declared in prototypes.h",
                "int FillMasterSpellBook(P_obj);" in proto_h)
if fill:
    body = fill.group(1)
    all_ok &= check("FillMasterSpellBook refuses non-spellbooks",
                    "obj->type != ITEM_SPELLBOOK" in body)
    all_ok &= check("FillMasterSpellBook stamps the book language",
                    "obj->value[0] = TONGUE_MAGIC;" in body)
    all_ok &= check("FillMasterSpellBook stamps every book class",
                    "obj->value[1] = BOOK_CLASSES;" in body)
    all_ok &= check("FillMasterSpellBook walks the whole spell table",
                    "spl = FIRST_SPELL; spl <= LAST_SPELL" in body)
    all_ok &= check("FillMasterSpellBook scribes through AddSpellToSpellBook",
                    "AddSpellToSpellBook(NULL, obj, spl)" in body)
    all_ok &= check("FillMasterSpellBook records the page count",
                    "obj->value[3] = pages;" in body)

# 4. Scribable predicate matches prac_all_spells(): a circle AND a learnable cap.
circle_fn = re.search(r"static int book_class_spell_circle\(int spl\)\s*\{(.*?)\n\}",
                      memorize_c, re.S)
all_ok &= check("book_class_spell_circle() defined in memorize.c", circle_fn is not None)
if circle_fn:
    body = circle_fn.group(1)
    all_ok &= check("book_class_spell_circle() only considers spellbook classes",
                    "IS_BOOK_CLASS(1U << idx)" in body)
    all_ok &= check("book_class_spell_circle() spans every specialization",
                    "spec <= MAX_SPEC" in body)
    all_ok &= check("book_class_spell_circle() requires a real circle",
                    "rlevel < 1 || rlevel > MAX_CIRCLE" in body)
    all_ok &= check("book_class_spell_circle() requires a learnable cap",
                    "maxlearn[spec]" in body)

# 5. NULL owner tolerated
all_ok &= check("AddSpellToSpellBook tolerates a NULL owner",
                "obj->value[1] = ch ? ch->player.m_class : 0;" in memorize_c)

# 6. Load-time hook, keyed on the vnum and the item type
hook = re.search(r"if \(obj->type == ITEM_SPELLBOOK &&\s*"
                 r"obj_index\[nr\]\.virtual_number == MASTER_SPELLBOOK_VNUM\)\s*"
                 r"FillMasterSpellBook\(obj\);", db_c)
all_ok &= check("read_object() fills the master spellbook on load", hook is not None)
if hook:
    all_ok &= check("the fill runs before convertObj()",
                    db_c.index("FillMasterSpellBook(obj);") < db_c.rindex("convertObj(obj);"))

# 7/8. Area prototype.  Only areas listed in areas/AREA are concatenated into
#      world.obj by make_obj, so orphaned .obj files do not claim vnums.
areas_obj = ROOT / "areas" / "obj"
built = []
for line in (ROOT / "areas" / "AREA").read_text(encoding="utf-8", errors="replace").splitlines():
    if not line.strip() or line.startswith("*"):
        continue
    built.append(line.split()[0])

owners = []
for name in built:
    path = areas_obj / (name + ".obj")
    if not path.is_file():
        continue
    if re.search(r"(?m)^#%d$" % vnum, path.read_text(encoding="utf-8", errors="replace")):
        owners.append(path)

all_ok &= check(f"exactly one built area defines object #{vnum}", len(owners) == 1)
if len(owners) != 1:
    print(f"       claimed by: {[str(p.relative_to(ROOT)) for p in owners]}")
if len(owners) == 1:
    text = owners[0].read_text(encoding="utf-8", errors="replace")
    entry = re.search(r"(?ms)^#%d$\n(.*?)(?=^#\d)" % vnum, text)
    all_ok &= check(f"object #{vnum} entry is parseable", entry is not None)
    if entry:
        lines = [l for l in entry.group(1).split("\n")]
        # name~ short~ long~ action~ then the numeric block
        nums = [l for l in lines if re.fullmatch(r"[-0-9 ]+", l.strip()) and l.strip()]
        all_ok &= check("prototype has the three numeric lines", len(nums) >= 3)
        if len(nums) >= 3:
            flags = nums[0].split()
            values = nums[1].split()
            all_ok &= check("prototype is an ITEM_SPELLBOOK (type 33)", flags[0] == "33")
            all_ok &= check("prototype is takeable and holdable (wear flags 16385)",
                            flags[7] == "16385")
            all_ok &= check(f"value[0] is TONGUE_MAGIC ({TONGUE_MAGIC})",
                            values[0] == str(TONGUE_MAGIC))
            all_ok &= check(f"value[1] is the BOOK_CLASSES mask ({mask})",
                            values[1] == str(mask))
            all_ok &= check("value[2] leaves room to scribe more", int(values[2]) > 0)
            all_ok &= check("value[3] starts empty (the fill sets it)", values[3] == "0")

if all_ok:
    print("\nAll master spellbook contract checks passed.")
    sys.exit(0)
print("\nFAILURES DETECTED in master spellbook contract tests.")
sys.exit(1)
