#!/usr/bin/env python3
"""Source integration contract for controlled cleric divine refusal."""

from pathlib import Path

from _paths import ROOT, extract_function, source
from contract_text import before, contains, count, index


actoff = source("actoff.c").read_text(encoding="utf-8")
interp = source("interp.c").read_text(encoding="utf-8")
interp_h = source("interp.h").read_text(encoding="utf-8")
structs = source("structs.h").read_text(encoding="utf-8")
makefile = (ROOT / "src" / "Makefile").read_text(encoding="utf-8")
properties = (ROOT / "lib" / "duris.properties").read_text(encoding="utf-8")

assert "cmd/divine_refusal_policy.o" in makefile
assert "unsigned long long divine_refusal_until_pulse;" in structs
assert "int ordered_command_number(const char *input);" in interp_h
assert "static int input_command_number(const char *input)" in interp
parser = extract_function("interp.c", "int ordered_command_number(")
assert contains(parser, "return input_command_number(input);")
assert contains(parser, "is_retired_command_spelling(word, len)")

for setting in (
    "pets.divine_refusal.enabled=0",
    "pets.divine_refusal.summoner_only=1",
    "pets.divine_refusal.percent=10",
    "pets.divine_refusal.retry_lock_seconds=4",
):
    assert setting in properties

eligibility = extract_function("actoff.c", "static bool divine_refusal_eligible(")
for condition in (
    "IS_PC(master)",
    "IS_ALIVE(master)",
    "IS_NPC(pet)",
    "GET_MASTER(pet) == master",
    "IS_AFFECTED(pet, AFF_CHARM)",
    "GET_CLASS(pet, CLASS_CLERIC)",
    "GET_CLASS(master, CLASS_SUMMONER)",
):
    assert contains(eligibility, condition)

decision = extract_function("actoff.c", "static bool divine_refusal_blocks_order(")
assert contains(decision, "const bool exempt = cmd == CMD_ABORT || cmd == CMD_FLEE;")
assert contains(decision, "divine_refusal_command_blocked(pet, cmd)")
assert contains(decision, "ne_event_tick")
assert contains(decision, "&pet->specials.divine_refusal_until_pulse")
assert contains(decision, "number")

display = extract_function("actoff.c", "static void show_new_divine_refusal(")
assert "My deity has warned me against completing that action." in display
assert "refuses your order with a solemn shake" in display
for forbidden in ("do_say(", "mobsay(", "do_emote("):
    assert forbidden not in display
assert "ACT_SILENCEABLE" in display
assert "is_silent(pet, false)" in display
assert "CAN_SPEAK(pet)" in display
assert contains(display, "master->specials.z_cord == pet->specials.z_cord")

order = extract_function("actoff.c", "void do_order(")
assert count(order, "divine_refusal_blocks_order(") == 2
assert count(order, "ordered_command_number(message)") == 1
assert count(order, "current_divine_refusal_config()") == 1
followers_start = index(order, 'else\n\t\t{ /* This is order "followers" */')
named = order[:followers_start]
followers = order[followers_start:]

assert before(named, "GET_MASTER(victim) != ch", "divine_refusal_blocks_order(")
assert before(named, "if (CAN_ACT(victim))", "divine_refusal_blocks_order(")
assert before(named, "divine_refusal_blocks_order(", "AFF5_ORDERING")
assert contains(named, "CharWait(ch, new_refusal ? PULSE_VIOLENCE : 2);")

assert before(followers, "GET_MASTER(k) == ch", "divine_refusal_blocks_order(")
assert before(followers, "IS_AFFECTED(k, AFF_CHARM)", "divine_refusal_blocks_order(")
assert before(followers, "if (!refusal_can_apply)", "!CAN_ACT(k) || IS_IMMOBILE(k)")
assert before(followers, "!CAN_ACT(k) || IS_IMMOBILE(k)",
              "divine_refusal_blocks_order(")
busy = followers[index(followers, "if (!CAN_ACT(k) || IS_IMMOBILE(k))"):
                 index(followers, "else", index(followers, "if (!CAN_ACT(k) || IS_IMMOBILE(k))"))]
assert before(busy, 'send_to_char("Ok.\\n", ch)',
              'act("$N seems a bit busy at the moment, try later."')
assert before(followers, "divine_refusal_blocks_order(", "AFF5_ORDERING")
assert contains(followers, "if (new_refusal) l_delay = TRUE;")
assert contains(followers, 'if (!acknowledged && !refused) send_to_char("Ok.\\n", ch);')
assert "CharWait(k," not in followers

docs = ROOT / "docs" / "reference" / "DIVINE_REFUSAL.md"
assert docs.exists()
assert "default-off" in docs.read_text(encoding="utf-8").lower()

print("Divine refusal order integration contract passed.")
