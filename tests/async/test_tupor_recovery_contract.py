#!/usr/bin/env python3
"""Source contracts for tupor recovery and interruption behavior."""
import re
from pathlib import Path

from _paths import SRC

ROOT = Path(__file__).resolve().parents[2]
INTERP = (SRC / "cmd" / "interp.c").read_text()
MEMORIZE = (SRC / "classes" / "memorize.c").read_text()
ACTMOVE = (SRC / "cmd" / "actmove.c").read_text()
ETHERMANCER = (SRC / "classes" / "ethermancer.c").read_text()
SKILLS = (SRC / "classes" / "skills.c").read_text()
PROTOTYPES = (SRC / "core" / "prototypes.h").read_text()
PROPERTIES = (ROOT / "lib" / "duris.properties").read_text()
HELP_INDEX = (ROOT / "lib" / "information" / "help_index").read_text()


def function_body(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


# The player-facing command must use the slot-recovery implementation and must
# not trigger the generic room-aggro pass. The separate offensive spell keeps
# its explicit TAR_AGGRO target flag.
active_tupor_lines = [
    line.strip()
    for line in INTERP.splitlines()
    if "CMD_Y(CMD_TUPOR" in line and not line.lstrip().startswith("//")
]
assert active_tupor_lines == [
    "CMD_Y(CMD_TUPOR, STAT_SLEEPING + POS_PRONE, do_assimilate, 0, FALSE);"
]
assert "TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO, spell_induce_tupor" in SKILLS

# Tupor is an active recovery command that should preserve meditation and
# concealment just like the other passive recovery commands.
assert "(cmd != CMD_TUPOR)" in INTERP
assert "cmd != CMD_DEFOREST && cmd != CMD_ARTIFACTS &&\n\t\t\t\t\t\t cmd != CMD_TUPOR)" in INTERP

# The first event must be based on the highest incomplete circle, not the
# character's maximum circle when the top circle is already full.
assert "schedule_memorize(ch, get_circle_memtime(ch, need_mem) / 2)" in MEMORIZE
assert "You are already recovering your spell power." in MEMORIZE

# Voluntary posture changes end recovery without the old shock wait. Hostile
# interruption remains explicit and data-driven.
assert "enum class memorization_stop_reason : uint8_t" in PROTOTYPES
assert "stop_memorizing(P_char, memorization_stop_reason = memorization_stop_reason::disrupted);" in PROTOTYPES
assert ACTMOVE.count("stop_memorizing(ch, memorization_stop_reason::voluntary);") == 7
assert "memorize.interrupt.tupor.wait=1.000" in PROPERTIES
assert "get_property(\"memorize.interrupt.tupor.wait\", 1.0)" in MEMORIZE
assert "Allows a Harpy or Ethermancer to recover spell slots." in HELP_INDEX
assert "Waking or changing posture voluntarily ends the recovery without a shock," in HELP_INDEX

induce_tupor = function_body(ETHERMANCER, "void spell_induce_tupor(")
assert "stop_memorizing(victim, memorization_stop_reason::disrupted);" in induce_tupor

# Keep the behavior split intentional: the legacy AFF4-only innate helper is
# not allowed to become the player-facing recovery implementation by accident.
assert re.search(
    r"CMD_Y\(CMD_TUPOR, STAT_SLEEPING \+ POS_PRONE, do_assimilate, 0, FALSE\);",
    INTERP,
)

print("tupor recovery source contract passed")
