#!/usr/bin/env python3
"""Regression contract for temporary dread-blade spell-save adjustment."""

from _paths import SRC
from _source_contract import function_body


source = (SRC / "classes/paladins.c").read_text(encoding="utf-8", errors="replace")
body = function_body(source, r"\bbool\s+dread_blade_proc\s*\(")
assert body is not None, "dread_blade_proc definition is missing"

adjustment = "victim->specials.apply_saving_throw[SAVING_SPELL] += 15;"
spell_call = "spell_func(number(1, GET_LEVEL(ch)), ch, 0, 0, victim, 0);"
restore = "victim->specials.apply_saving_throw[SAVING_SPELL] = save;"

adjustment_at = body.index(adjustment)
spell_call_at = body.index(spell_call, adjustment_at)
restore_at = body.index(restore, spell_call_at)
return_at = body.index("return !attacker_in_room || !victim_in_room;", restore_at)

checks = {
    "runtime identities are captured before the temporary adjustment": body.index(
        "const uint64_t victim_runtime_id = victim->runtime_id;"
    )
    < adjustment_at,
    "the proc still invokes the selected spell while the adjustment is active": adjustment_at
    < spell_call_at,
    "the restore is reachable after the spell callback": spell_call_at < restore_at,
    "the restore is executed before the proc returns": restore_at < return_at,
    "victim cleanup is guarded by the original runtime identity": "if (live_victim == victim)"
    in body[restore_at - 220 : restore_at + 180],
    "room status is checked only for identities that remain registered": "live_victim == victim && is_char_in_room(victim, room)"
    in body,
    "attacker removal ends the proc without dereferencing an unregistered pointer": "find_character_by_runtime_id(attacker_runtime_id) == ch"
    in body,
}

for name, passed in checks.items():
    print(f"[{'PASS' if passed else 'FAIL'}] {name}")

failed = [name for name, passed in checks.items() if not passed]
if failed:
    raise SystemExit("\n".join(failed))

print("\nDread-blade save restoration contract passed")
