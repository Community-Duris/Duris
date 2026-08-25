"""Guards the humanoid coin-pickup loop in event_mob_mundane().

do_get() takes the first object matching "coins" in the room, which is not
necessarily the pile the loop is looking at, and picking a pile up extracts and
frees it.  Caching obj->next_content across that call therefore leaves a
dangling pointer, and the next iteration dereferences it -- a SIGSEGV that took
the server down whenever a humanoid mob met more than one coin pile (e.g. after
a player death dropped loot near town guards).
"""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
mobact = (ROOT / "src/mobact.c").read_text()

block = mobact.split("if (IS_HUMANOID(ch) && !IS_PATROL(ch))", 1)[1]
block = block.split("if (IS_SET(ch->specials.act, ACT_SCAVENGER)", 1)[0]

assert 'do_get(ch, Gbuf1, CMD_GET);' in block, "coin pickup call went missing"

# The loop must restart from the room list head after each pickup ...
assert "world[ch->in_room].contents" in block
assert re.search(r"\bdo\b\s*\{", block), "expected a rescan loop around the walk"
assert re.search(r"\}\s*while\s*\(", block), "expected a rescan loop around the walk"

# ... and must leave the walk immediately after do_get() rather than carrying a
# cached next pointer into another iteration.
after_get = block.split("do_get(ch, Gbuf1, CMD_GET);", 1)[1]
before_loop_end = after_get.split("}\n\t\t} while", 1)[0]
assert "break;" in before_loop_end, "do_get() must be followed by a break"

# The rescan must be bounded so a pile do_get() refuses cannot spin forever.
assert re.search(r"while\s*\(\s*\w+\s*&&\s*\+\+\w+\s*<\s*\d+\s*\)", block), (
    "rescan loop needs a bound in case do_get() declines the pile"
)

print("mobact coin pickup rescan contract passed")
