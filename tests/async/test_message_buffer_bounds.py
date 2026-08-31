#!/usr/bin/env python3
"""Contracts for combat-message helpers that format into a caller's buffers.

Both helpers below used to write through `struct damage_messages`, whose members
now hold immutable text.  Each formatted with MAX_STRING_LENGTH (65536) into a
caller buffer that was much smaller, so each takes the caller's real size now.
"""
from _paths import SRC
from pathlib import Path

root = Path(__file__).resolve().parents[2]
fight = (SRC / "fight.c").read_text()
reavers = (SRC / "reavers.c").read_text()
structs = (SRC / "structs.h").read_text()

failures = []


def check(name, ok):
    print(("[PASS] " if ok else "[FAIL] ") + name)
    if not ok:
        failures.append(name)


# damage_messages carries immutable text only.
decl_start = structs.index("struct damage_messages")
decl = structs[decl_start:structs.index("};", decl_start)]
check(
    "damage_messages message members are const char *",
    all(
        ("const char *" + m) in decl
        for m in ("attacker", "victim", "room", "death_attacker", "death_victim", "death_room")
    ),
)
check("damage_messages has no writable char * member", "\tchar *" not in decl)

# anatomy_strike borrows hit()'s buffers and is told how big they are.
ana_start = fight.index("int anatomy_strike(")
ana = fight[ana_start:fight.index("\nint required_weapon_skill(", ana_start)]
check("anatomy_strike takes the caller's buffers", "char *attacker_msg" in ana and
      "char *victim_msg" in ana and "char *room_msg" in ana)
check("anatomy_strike takes the caller's buffer size", "size_t msg_size" in ana)
check("anatomy_strike formats within msg_size", "MAX_STRING_LENGTH" not in ana)
check("anatomy_strike no longer writes through messages",
      "snprintf(messages->" not in ana)
check("anatomy_strike callers pass sizeof of their own buffer",
      fight.count("sizeof attacker_msg, (int)dam)") == 2)

# ilienze_sword_proc_messages likewise.
ili_start = reavers.index("void ilienze_sword_proc_messages(char *")
ili = reavers[ili_start:reavers.index("\n}", ili_start)]
check("ilienze_sword_proc_messages takes a size", "size_t size" in ili)
check("ilienze_sword_proc_messages formats within size", "MAX_STRING_LENGTH" not in ili)
check("ilienze_sword_proc_messages callers pass sizeof of their own buffer",
      reavers.count("sizeof ch_msg,") == 4)

# Nothing anywhere still formats through a damage_messages member.
for name, text in (("fight.c", fight), ("reavers.c", reavers)):
    check("no snprintf through damage_messages in " + name,
          "snprintf(messages->attacker" not in text and
          "snprintf(messages.attacker" not in text)

if failures:
    print("\nFailed regression checks:")
    for f in failures:
        print("- " + f)
    raise SystemExit(1)
print("message buffer bound contract passed")
