#!/usr/bin/env python3
"""Source-contract and regression tests for the spell abort command (`abort`).

Verifies:
1. Command constants defined in interp.h (CMD_ABORT) and config.h (MAX_CMD).
2. Command registered in interp.c command array matching index.
3. Command whitelisted during casting in interp.c (AFF2_CASTING check).
4. Command pointer registered in assign_command_pointers() with correct position/flags.
5. comm.c allows dequeuing input when character is casting (AFF2_CASTING).
6. Prototype declared in prototypes.h.
7. do_abort implemented in sparser.c handling StopCasting, wait reset, PULSE_VIOLENCE lag, and camp abort.
8. command_attributes.txt coverage for abort.
9. help_index contains ABORT command documentation.
10. Default-off spell-abort policy and casting-blocked hint behavior.
"""

from _paths import SRC
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
def check(name, ok):
    if not ok:
        print(f"FAIL: {name}")
    else:
        print(f"OK: {name}")
    return bool(ok)

all_ok = True

# 1. Verify config.h and interp.h definitions
config_h = (SRC / "config.h").read_text(encoding="utf-8", errors="replace")
interp_h = (SRC / "interp.h").read_text(encoding="utf-8", errors="replace")
interp_c = (SRC / "interp.c").read_text(encoding="utf-8", errors="replace")

m_cmd = re.search(r"#define\s+CMD_ABORT\s+(\d+)", interp_h)
all_ok &= check("CMD_ABORT defined in interp.h", m_cmd is not None)
cmd_abort_num = int(m_cmd.group(1)) if m_cmd else None

m_max = re.search(r"#define\s+MAX_CMD\s+(\d+)", config_h)
all_ok &= check("MAX_CMD defined in config.h", m_max is not None)
max_cmd_num = int(m_max.group(1)) if m_max else None

# 2. Verify command array alignment in interp.c
cmd_block = re.search(r"const char \*command\[MAX_CMD\] = \{(.*?)\n\};", interp_c, re.S)
all_ok &= check("command array present in interp.c", cmd_block is not None)
if cmd_block:
    cmds = re.findall(r'"([^"]+)"', cmd_block.group(1))
    all_ok &= check("command array includes 'abort'", "abort" in cmds)
    if "abort" in cmds:
        abort_idx = cmds.index("abort") + 1  # 1-indexed
        all_ok &= check(f"CMD_ABORT ({cmd_abort_num}) matches position in command[] ({abort_idx})",
                        cmd_abort_num == abort_idx)
    all_ok &= check("MAX_CMD matches size of command array", len(cmds) == max_cmd_num)

# 3. Verify the per-player casting whitelist in interp.c
whitelist = re.search(r"bool cmd_allowed_while_casting\(P_char ch, int cmd\)\s*\{(.*?)\n\}", interp_c, re.S)
all_ok &= check("cmd_allowed_while_casting() defined in interp.c", whitelist is not None)
if whitelist:
    for name in ("CMD_PETITION", "CMD_RETURN", "CMD_ABORT", "PLR3_ABORT_CASTING"):
        all_ok &= check(f"cmd_allowed_while_casting() mentions {name}", name in whitelist.group(1))

all_ok &= check("casting guard in interp.c uses the character-aware whitelist",
                "IS_AFFECTED2(ch, AFF2_CASTING) && !cmd_allowed_while_casting(ch, cmd)" in interp_c)
all_ok &= check("casting-blocked command hints lead with abort when the player toggle is on",
                interp_c.count("Try 'abort' to stop casting.") == 2)
all_ok &= check("casting-blocked hints honor the per-player abort toggle",
                interp_c.count("PLR3_FLAGGED(ch, PLR3_ABORT_CASTING)") >= 3 and
                "Try 'return' or petition other gods for help if you're stuck." in interp_c and
                "If you think you're stuck, you can still petition." in interp_c)

# 3b. The casting guard must run before the falling / water-current checks, which
#     have side effects (falling_char, do_move) and would otherwise fire on a
#     command that is about to be rejected -- including moving a casting player.
gate_pos = interp_c.find("IS_AFFECTED2(ch, AFF2_CASTING) && !cmd_allowed_while_casting(ch, cmd)")
fall_pos = interp_c.find("if (world[ch->in_room].chance_fall")
current_pos = interp_c.find("The current sweeps you away!")
all_ok &= check("casting guard precedes the falling check",
                -1 < gate_pos < fall_pos)
all_ok &= check("casting guard precedes the water-current sweep",
                -1 < gate_pos < current_pos)

# 4. Verify assign_command_pointers binding
all_ok &= check("assign_command_pointers registers CMD_ABORT",
                "CMD_Y(CMD_ABORT, STAT_RESTING + POS_PRONE, do_abort, 0, TRUE);" in interp_c)

# 5. Verify comm.c keeps the selective casting queue active for the complete
#    AFF2_CASTING lifetime, including after PLR2_WAIT clears.
comm_c = (SRC / "comm.c").read_text(encoding="utf-8", errors="replace")
casting_predicate = re.search(
    r"static bool casting_input_for_descriptor\(P_desc descriptor, P_char character\)\s*\{(.*?)\n\}",
    comm_c,
    re.S,
)
all_ok &= check("comm.c defines a casting-state queue predicate", casting_predicate is not None)
if casting_predicate:
    body = casting_predicate.group(1)
    all_ok &= check("casting predicate is driven by AFF2_CASTING", "IS_AFFECTED2(character, AFF2_CASTING)" in body)
    all_ok &= check("casting predicate is independent of CAN_ACT", "CAN_ACT" not in body)
    all_ok &= check("casting predicate limits itself to playing descriptors",
                    "descriptor->connected == CON_PLAYING" in body and
                    "!descriptor->showstr_count" in body and
                    "!descriptor->str" in body)
all_ok &= check("comm.c assigns casting_input from the casting-state predicate",
                "casting_input = casting_input_for_descriptor(point, t_ch);" in comm_c)
all_ok &= check("comm.c still admits the casting path when CAN_ACT is true",
                "(CAN_ACT(t_ch) || casting_input)" in comm_c)
all_ok &= check("comm.c only reads a casting character's queue through the character-aware casting path",
                re.search(r"casting_input\s*\? get_casting_cmd_from_q\(t_ch, &point->input, comm\)",
                          comm_c) is not None)
# Type-ahead must survive: only a command the casting gate will actually run is
# dequeued, so everything else stays queued instead of being drained and rejected.
q = re.search(r"int get_casting_cmd_from_q\(P_char ch, struct txt_q \*queue, char \*dest\)\s*\{(.*?)\n\}", comm_c, re.S)
all_ok &= check("get_casting_cmd_from_q() defined in comm.c", q is not None)
if q:
    body = q.group(1)
    all_ok &= check("get_casting_cmd_from_q() applies the player-aware casting filter",
                    "input_allowed_while_casting(ch, tmp->text)" in body)
filtered = re.search(
    r"static int get_filtered_cmd_from_q\(.*?\)\s*\{(.*?)\n\}", comm_c, re.S)
all_ok &= check("filtered queue extraction helper is defined in comm.c", filtered is not None)
if filtered:
    body = filtered.group(1)
    # An 'abort' typed after other type-ahead must not sit behind it, and
    # unlinking a middle/tail entry must keep queue->tail valid for write_to_q().
    all_ok &= check("filtered queue helper extracts out of order",
                    "prev->next = tmp->next;" in body and "queue->head = tmp->next;" in body)
    all_ok &= check("filtered queue helper keeps queue->tail valid",
                    "queue->tail == tmp" in body and "queue->tail = prev;" in body)
all_ok &= check("get_casting_cmd_from_q declared in prototypes.h",
                "int get_casting_cmd_from_q(P_char, struct txt_q *, char *);" in
                (SRC / "prototypes.h").read_text(encoding="utf-8", errors="replace"))
all_ok &= check("input_allowed_while_casting() is character-aware",
                "bool input_allowed_while_casting(P_char, const char *);" in
                (SRC / "prototypes.h").read_text(encoding="utf-8", errors="replace"))
all_ok &= check("cmd_allowed_while_casting() is character-aware",
                "bool cmd_allowed_while_casting(P_char, int);" in
                (SRC / "prototypes.h").read_text(encoding="utf-8", errors="replace"))
all_ok &= check("input_allowed_while_casting() defined in interp.c",
                "bool input_allowed_while_casting(P_char ch, const char *input)" in interp_c)

# 6. Verify prototypes.h declaration
proto_h = (SRC / "prototypes.h").read_text(encoding="utf-8", errors="replace")
all_ok &= check("do_abort declared in prototypes.h",
                "void do_abort(P_char, char *, int);" in proto_h)

# 7. Verify sparser.c do_abort implementation
sparser_c = (SRC / "sparser.c").read_text(encoding="utf-8", errors="replace")
do_abort_m = re.search(r"void do_abort\(P_char ch,.*?\)\s*\{(.*?)\n\}", sparser_c, re.S)
all_ok &= check("do_abort implemented in sparser.c", do_abort_m is not None)
if do_abort_m:
    body = do_abort_m.group(1)
    all_ok &= check("do_abort checks IS_ALIVE(ch)", "IS_ALIVE(ch)" in body)
    all_ok &= check("do_abort checks AFF2_CASTING", "IS_AFFECTED2(ch, AFF2_CASTING)" in body)
    all_ok &= check("do_abort calls StopCasting(ch)", "StopCasting(ch);" in body)
    all_ok &= check("do_abort disarms event_wait", "disarm_char_nevents(ch, event_wait);" in body)
    all_ok &= check("do_abort clears PLR2_WAIT", "REMOVE_BIT(ch->specials.act2, PLR2_WAIT);" in body)
    all_ok &= check("do_abort applies PULSE_VIOLENCE lag via CharWait", "CharWait(ch, PULSE_VIOLENCE);" in body)
    all_ok &= check("do_abort supports camping abort fallback", "AFF_CAMPING" in body and "TAG_CAMP" in body)
    all_ok &= check("do_abort reports non-casting feedback", "You are not casting a spell to abort!" in body)

# 8. Verify the player-facing `tog abort` policy and persisted bit.
cmd_attrs = (ROOT / "docs/lib/information/command_attributes.txt").read_text(encoding="utf-8", errors="replace")
all_ok &= check("command_attributes.txt includes abort entry", "abort\n~" in cmd_attrs)
structs_h = (SRC / "structs.h").read_text(encoding="utf-8", errors="replace")
all_ok &= check("PLR3_ABORT_CASTING is a persisted player bit", "#define PLR3_ABORT_CASTING BIT_16" in structs_h)
actoth_c = (SRC / "actoth.c").read_text(encoding="utf-8", errors="replace")
all_ok &= check("toggles list exposes abort", '"abort", // 66' in actoth_c)
all_ok &= check("tog abort toggles PLR3_ABORT_CASTING",
                "case 66" in actoth_c and "PLR3_TOG(PLR3_ABORT_CASTING)" in actoth_c)
all_ok &= check("tog abort has player-facing on/off messages",
                "Spell abort is disabled." in actoth_c and "Spell abort is enabled." in actoth_c)
all_ok &= check("toggle status displays abort state", "PLR3_FLAGGED(ch, PLR3_ABORT_CASTING)" in actoth_c)

# 9. Verify help_index
help_idx = (ROOT / "lib/information/help_index").read_text(encoding="utf-8", errors="replace")
all_ok &= check("help_index includes ABORT command entry", '"ABORT" (Command)' in help_idx and "Syntax:         abort" in help_idx)

if all_ok:
    print("\nAll spell abort command contract checks passed.")
    sys.exit(0)
else:
    print("\nFAILURES DETECTED in spell abort command contract tests.")
    sys.exit(1)
