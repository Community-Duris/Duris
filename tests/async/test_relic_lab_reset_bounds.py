#!/usr/bin/env python3
"""Regression test for the relic pickup crash (world[NOWHERE] indexing).

Picking up an artifact relic (vnums 58/59/68) makes relic_proc() fire on the
player's next command, which calls update_relic() -> reset_lab().  reset_lab()
walked 10,000 vnums and fed every real_room() result straight into world[],
including NOWHERE (-1) for the many labyrinth rooms that do not exist in the
world table.  That out-of-bounds access crashed the server.

Verifies:
1. reset_lab() checks real_room() against NOWHERE before indexing world[].
2. reset_lab() no longer mixes up the room it stamps with the room it drains
   (the old code incremented the index between the two).
3. reset_lab() rejects unknown lab types instead of using uninitialised vnums.
4. create_lab()/connect_lab()/connect_other() guard the same lookups, so the
   immortal 'randobj map' path cannot reproduce the crash.
"""

from pathlib import Path
import re
import sys
from contract_text import contains

ROOT = Path(__file__).resolve().parents[2]
src = (ROOT / "src" / "random.zone.c").read_text(encoding="utf-8", errors="replace")

checks = []


def body(pattern):
    m = re.search(pattern, src, re.S)
    return m.group(0) if m else None


reset_lab = body(r"int reset_lab\(int type\)\s*\{.*?\n\}")
if reset_lab:
    checks.append((
        "reset_lab guards room lookups against NOWHERE",
        "== NOWHERE" in reset_lab and "world[real_room(start_room + i)]" not in reset_lab
    ))
    checks.append((
        "reset_lab resolves the entrance room once and guards it",
        "entrance_rnum = real_room(entrance_room);" in reset_lab and
        "entrance_rnum != NOWHERE" in reset_lab
    ))
    checks.append((
        "reset_lab uses one resolved room number per iteration",
        "world[room_rnum].sector_type" in reset_lab and
        "world[room_rnum].people" in reset_lab and
        "world[room_rnum].contents" in reset_lab
    ))
    checks.append((
        "reset_lab does not advance the loop index between lookups",
        not re.search(r"\bi\+\+;\s*\n\s*for \(vict", reset_lab)
    ))
    checks.append((
        "reset_lab rejects unknown lab types",
        "unknown lab type" in reset_lab
    ))
else:
    checks.append(("reset_lab function present", False))

create_lab = body(r"int create_lab\(int type\)\s*\{.*?\n\}")
if create_lab:
    checks.append((
        "create_lab aborts when the lab rooms are not in the world table",
        "real_room(start_room) == NOWHERE" in create_lab and "real_room(map_room) == NOWHERE" in create_lab
    ))
    checks.append((
        "create_lab guards the ROOM_NO_TELEPORT stamp",
        "if (real_room(current_room) != NOWHERE)" in create_lab
    ))
else:
    checks.append(("create_lab function present", False))

connect_lab = body(r"int connect_lab\(int room, int dir\)\s*\{.*?\n\}")
if connect_lab:
    checks.append((
        "connect_lab resolves both ends before indexing world[]",
        contains(connect_lab, "int here = real_room(room);") and
        contains(connect_lab, "there == NOWHERE || here == NOWHERE")
    ))
else:
    checks.append(("connect_lab function present", False))

connect_other = body(r"int connect_other\(int room\)\s*\{.*?\n\}")
if connect_other:
    checks.append((
        "connect_other guards both room lookups",
        contains(connect_other, "real_room(room) != NOWHERE") and
        contains(connect_other, "real_room(room + dir_to_num(dir)) != NOWHERE")
    ))
else:
    checks.append(("connect_other function present", False))

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")

if failed:
    print("\nFailed regression checks:")
    for name in failed:
        print(f"- {name}")
    sys.exit(1)

print("\nAll relic lab reset bounds checks passed successfully.")
