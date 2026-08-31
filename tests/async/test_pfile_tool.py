#!/usr/bin/env python3
"""End-to-end contracts for the offline bin/tools/pfile scanner.

The scanner links only skills.c and files.c out of the server tree and fills
the rest in with pfile-stubs.c, so link stubs and `_PFILE_` macro arms rot
silently: the tool keeps compiling while producing nothing at runtime.  These
tests exercise the two paths that rot.
"""

from _paths import SRC, source
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

# ---------------------------------------------------------------------------
# The _PFILE_ macro arms must index the skill being defined, not numSkills.
# numSkills is only ever mutated by server-only setup that _PFILE_ excludes,
# so indexing by it collapses every skill onto skills[0].
# ---------------------------------------------------------------------------
skills_text = source("skills.c").read_text(encoding="utf-8")
macro_block = skills_text.split("Skill skills[MAX_AFFECT_TYPES + 1];", 1)[1].split("#else", 1)[0]
assert "skills[numSkills]" not in macro_block, (
    "the _PFILE_/_DE_ macro arms index by numSkills, which stays 0 under -D_PFILE_; "
    "index by currentSkill instead"
)
assert "skills[currentSkill].m_class" in macro_block

# The stubbed tool must format strings for real; a no-op checked_snprintf
# leaves every player path empty and the scanner opens nothing.
stubs_text = (SRC / "account" / "pfile-stubs.c").read_text(encoding="utf-8")
assert "checked_snprintf" not in stubs_text, (
    "pfile-stubs.c must not stub checked_snprintf; link core/safe_format.c instead"
)
assert "core/safe_format.o" in (SRC / "Makefile").read_text(encoding="utf-8"), (
    "the pfile link line must pull in the real checked_snprintf"
)

# ---------------------------------------------------------------------------
# Runtime: initialize_skills() under _PFILE_ populates distinct skill slots.
# ---------------------------------------------------------------------------
PROBE = r"""
#include "core/structs.h"
#include <cstdio>

extern Skill skills[MAX_AFFECT_TYPES + 1];
extern void initialize_skills();

void create_epic_skills() {}

int flag2idx(int flag)
{
    int i = 0;
    while (flag > 0)
    {
        i++;
        flag >>= 1;
    }
    return i;
}

int main()
{
    initialize_skills();

    int populated = 0;
    for (int skill = 1; skill <= MAX_AFFECT_TYPES; skill++)
        for (int cls = 0; cls < CLASS_COUNT && populated <= skill; cls++)
            for (int spec = 0; spec < MAX_SPEC + 1; spec++)
                if (skills[skill].m_class[cls].rlevel[spec])
                {
                    populated++;
                    cls = CLASS_COUNT;
                    break;
                }

    std::printf("%d\n", populated);
    return 0;
}
"""

CFLAGS = ["-std=c++20", "-w", "-D_PFILE_", "-DTEST_MUD", "-D__NO_TESTS__"]
INCLUDES = ["-I" + str(SRC), "-I/usr/include/libxml2", "-I/usr/include/mysql"]

with tempfile.TemporaryDirectory() as workspace:
    work = Path(workspace)
    probe = work / "probe.cpp"
    probe.write_text(PROBE, encoding="utf-8")
    binary = work / "probe"

    subprocess.run(
        ["g++", *CFLAGS, *INCLUDES, "-x", "c++", str(source("skills.c")),
         str(probe), "-o", str(binary)],
        check=True,
        cwd=SRC,
    )
    populated = int(subprocess.run(
        [str(binary)], check=True, capture_output=True, text=True
    ).stdout.strip())

# Every class skill collapsing onto skills[0] leaves this at zero.
assert populated > 100, f"only {populated} skill slots carry class levels under _PFILE_"

# ---------------------------------------------------------------------------
# Runtime: the built tool resolves Players/<letter>/<name> and opens the file.
# ---------------------------------------------------------------------------
subprocess.run(["make", "-s", "-C", str(SRC), "pfile"], check=True)
tool = ROOT / "bin" / "tools" / "pfile"
assert tool.is_file(), "make pfile did not produce bin/tools/pfile"

with tempfile.TemporaryDirectory() as workspace:
    work = Path(workspace)
    for letter in "abcdefghijklmnopqrstuvwxyz":
        (work / "Players" / letter).mkdir(parents=True)
    # Too short to parse, but reaching the "too short" complaint proves the
    # path was formatted and the file was opened.
    (work / "Players" / "t" / "testchar").write_bytes(b"X")

    scan = subprocess.run(
        [str(tool), "testchar", "-t"], cwd=work, capture_output=True, text=True
    )

assert "Problem restoring save file of: testchar" in scan.stderr, (
    "pfile never opened Players/t/testchar; player paths are not being formatted:\n"
    f"stdout={scan.stdout!r} stderr={scan.stderr!r}"
)

print("pfile tool contracts hold")
